#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "cpp-httplib/httplib.h"
#include <stdio.h>
#include <fstream>
#include <algorithm>
#include <UE4SSProgram.hpp>
#include <Mod/CppUserModBase.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <windows.h>

using namespace RC;

// BEGIN NON-MIT LICENSED SECTION //

// THE FOLLOWING METHODS ARE ADAPTED FROM STACK OVERFLOW ANSWERS!
// THEY ARE NOT LICENSED UNDER MIT. SEE COMMENTS FOR FURTHER INFORMATION

// CC BY-SA 2.5 (https://creativecommons.org/licenses/by-sa/2.5/se/deed.en)
// This code is copyrighted by StackOverflow user "Evan Teran" https://stackoverflow.com/users/13430/evan-teran
// Minor changes were made to this source code from the original. No warranties are given. See the original license text for more information.
// https://stackoverflow.com/a/217605
inline void AutoIntegrator_rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
        }).base(), s.end());
}

// CC BY-SA 3.0 (https://creativecommons.org/licenses/by-sa/3.0/deed.en)
// This code is copyrighted by StackOverflow user "mkaes" https://stackoverflow.com/users/264338/mkaes
// Minor changes were made to this source code from the original. No warranties are given. See the original license text for more information.
// https://stackoverflow.com/a/6924332
std::string AutoIntegrator_get_dll_path()
{
    wchar_t path[32768]; // not MAX_PATH just in case... but MAX_PATH is probably reasonable here
    HMODULE hm = NULL;

    if (GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)&AutoIntegrator_rtrim, &hm) == 0)
    {
        int ret = GetLastError();
        fprintf(stderr, "GetModuleHandle failed, error = %d\n", ret);
        return "."; // unlikely to work
    }
    if (GetModuleFileNameW(hm, path, sizeof(path)) == 0)
    {
        int ret = GetLastError();
        fprintf(stderr, "GetModuleFileName failed, error = %d\n", ret);
        return "."; // unlikely to work
    }

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring out_str = path;
    std::string out_str_narrow = converter.to_bytes(out_str);
    return out_str_narrow;
}

// END NON-MIT LICENSED SECTION //

// ALL CODE FROM THIS POINT ON IS MIT LICENSED BY ATENFYR
// SEE THE "LICENSE" FILE FOR MORE INFORMATION

std::string AutoIntegrator_exec(std::string cmd_cpp, std::string cwd) {
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    HANDLE hReadPipe, hWritePipe;
    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) throw std::runtime_error("Failed to create pipe");

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));

    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.wShowWindow = SW_HIDE;
    si.hStdOutput = hWritePipe;
    si.hStdError = hWritePipe;

    char* cmd = _strdup(cmd_cpp.c_str());

    std::string output;

    try
    {
        if (!CreateProcessA(NULL, cmd, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, cwd.empty() ? NULL : cwd.c_str(), &si, &pi)) throw std::runtime_error("Failed to create process");

        CloseHandle(hWritePipe);

        try
        {
            char buffer[4096];
            DWORD bytesRead;

            while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0)
            {
                buffer[bytesRead] = '\0';
                output += buffer;
            }

            WaitForSingleObject(pi.hProcess, INFINITE);
        }
        catch(...)
        {
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            CloseHandle(hReadPipe);
            free(cmd);
            throw;
        }

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(hReadPipe);
        free(cmd);
    }
    catch (...)
    {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        free(cmd);
        throw;
    }

    return output;
}

bool AutoIntegrator_check_linux()
{
    // always run the integrator through wine
    return false;
}

bool AutoIntegrator_download_exe(std::string folder_path, std::string ver)
{
    Output::send<LogLevel::Normal>(L"Checking for updates to AstroModIntegrator Classic...\n");

    httplib::Headers headers = {
        { "User-Agent", ("atenfyr.com/" + ver) }
    };

#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
    httplib::SSLClient cli("github.com");
    cli.set_ca_cert_path(folder_path + "/ca-bundle.crt");
    cli.enable_server_certificate_verification(true);
#else
    httplib::Client cli("github.com");
#endif

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    bool success1 = false;
    std::string latest_ver;
    try
    {
        if (auto res = cli.Get("/atenfyr/AstroModLoader-Classic/releases/latest", headers))
        {
            //Output::send<LogLevel::Normal>(std::to_wstring(res->status));

            if (res->status >= 300 && res->status < 400)
            {
                // redirect
                std::string latest_link = res->get_header_value("location");
                //Output::send<LogLevel::Verbose>(converter.from_bytes(latest_link) + L"\n");
                if (latest_link.back() == '/') latest_link = latest_link.substr(0, latest_link.size() - 1);
                latest_ver = latest_link.substr(latest_link.find_last_of('/') + 1);
                if (latest_ver.front() == 'v') latest_ver = latest_ver.substr(1);
                success1 = true;
            }
        }
    }
    catch (...)
    {
        Output::send<LogLevel::Error>(L"Failed to retrieve the latest version of AstroModIntegrator Classic\n");
    }

    if (success1) Output::send<LogLevel::Verbose>(L"Latest version of AstroModIntegrator Classic: v" + converter.from_bytes(latest_ver) + L"\n");

    bool success2 = false;
    bool isLinux = AutoIntegrator_check_linux();
    try
    {
        if (success1 && latest_ver != ver) // did we successfully get the latest version, and do we actually need to update?
        {
            Output::send<LogLevel::Verbose>(L"Updating AstroModIntegrator Classic...\n");
            if (auto res = cli.Get("/atenfyr/AstroModLoader-Classic/releases/download/v" + latest_ver + (isLinux ? "/ModIntegrator-linux-x64" : "/ModIntegrator-win-x64.exe"), headers))
            {
                // follow redirects
                while (res->status >= 300 && res->status < 400)
                {
                    std::string newLoc = res->get_header_value("location");
                    if (newLoc.substr(0, std::string{ "http://" }.size()) == "http://") newLoc = newLoc.substr(std::string{ "http://" }.size());
                    if (newLoc.substr(0, std::string{ "https://" }.size()) == "https://") newLoc = newLoc.substr(std::string{ "https://" }.size());
                    std::string domain = newLoc.substr(0, newLoc.find_first_of('/'));
                    newLoc = newLoc.substr(newLoc.find_first_of('/') + 1);

                    Output::send<LogLevel::Verbose>(L"Redirect: " + converter.from_bytes(domain) + L", " + converter.from_bytes(newLoc) + L"\n");
#ifdef CPPHTTPLIB_OPENSSL_SUPPORT
                    httplib::SSLClient cli2(domain);
                    cli.set_ca_cert_path(folder_path + "/ca-bundle.crt");
                    cli.enable_server_certificate_verification(true);
#else
                    httplib::Client cli2(domain);
#endif
                    res = cli2.Get(newLoc, headers);
                }

                // download file
                if (res->status == httplib::StatusCode::OK_200)
                {
                    std::string newExePath = folder_path + (isLinux ? "/ModIntegrator" : "/ModIntegrator.exe");
                    
                    std::ofstream fs(newExePath, std::ios::out | std::ios::binary);
                    fs.write((res->body).data(), (res->body).size());
                    fs.close();

                    if (isLinux)
                    {
                        std::filesystem::permissions(newExePath, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write | std::filesystem::perms::owner_exec |
                            std::filesystem::perms::group_read | std::filesystem::perms::group_exec |
                            std::filesystem::perms::others_read | std::filesystem::perms::others_exec, std::filesystem::perm_options::replace);
                    }

                    Output::send<LogLevel::Normal>(L"Successfully downloaded ModIntegrator\n");
                    success2 = true;
                }
                else
                {
                    throw std::runtime_error("Invalid HTTP status code: " + std::to_string(res->status));
                }
            }
            else
            {
                throw std::runtime_error("HTTP request failed: " + httplib::to_string(res.error()));
            }
        }
    }
    catch (const std::runtime_error& err)
    {
        const char* exceptionMsg = err.what();
        std::string exceptionMsgCpp = exceptionMsg;
        Output::send<LogLevel::Error>(L"Failed to update the local copy of AstroModIntegrator Classic. " + converter.from_bytes(exceptionMsgCpp) + L"\n");
    }
    catch (...)
    {
        Output::send<LogLevel::Error>(L"Failed to update the local copy of AstroModIntegrator Classic.\n");
    }

    return success2;
}

std::wstring AutoIntegrator_GetFinalPathFromHandle(HANDLE hFile)
{
    DWORD size = GetFinalPathNameByHandleW(hFile, nullptr, 0, FILE_NAME_NORMALIZED);

    if (size == 0)
    {
        return L"";
    }

    std::vector<wchar_t> buffer(size);
    DWORD result = GetFinalPathNameByHandleW(
        hFile,
        buffer.data(),
        size,
        FILE_NAME_NORMALIZED
    );

    if (result == 0 || result >= size)
    {
        return L"";
    }

    std::wstring path = buffer.data();

    if (path.substr(0, 4) == L"\\\\?\\")
    {
        if (path.substr(4, 4) == L"UNC\\")
        {
            path = L"\\\\" + path.substr(8);
        }
        else
        {
            path = path.substr(4);
        }
    }

    return path;
}

std::wstring AutoIntegrator_PassPathThroughShimloader(std::wstring oldPath)
{
    HANDLE hFile = CreateFileW(
        oldPath.c_str(),
        0,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS,
        nullptr
    );

    if (hFile != INVALID_HANDLE_VALUE && hFile != nullptr)
    {
        std::wstring outVal = AutoIntegrator_GetFinalPathFromHandle(hFile);
        CloseHandle(hFile);
        if (outVal.empty()) outVal = oldPath;

        //Output::send<LogLevel::Verbose>(oldPath + L" => " + outVal + L"\n");
        return outVal;
    }

    return oldPath;
}

std::string AutoIntegrator_PassPathThroughShimloader(std::string oldPath)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(AutoIntegrator_PassPathThroughShimloader(converter.from_bytes(oldPath)));
}

std::string AutoIntegrator_GetExecutablePathForCmd(std::string folderPath)
{
    return AutoIntegrator_PassPathThroughShimloader(folderPath + "/ModIntegrator" + (AutoIntegrator_check_linux() ? "" : ".exe"));
}

bool AutoIntegrator_IsSecondSubDirOfFirst(const std::filesystem::path& first, const std::filesystem::path& second) {
    try
    {
        std::filesystem::path firstCanon = std::filesystem::canonical(first);
        std::filesystem::path secondCanon = std::filesystem::canonical(second);
        return std::mismatch(secondCanon.begin(), secondCanon.end(), firstCanon.begin(), firstCanon.end()).second == firstCanon.end();
    }
    catch (...) {}
    return false;
}

bool AutoIntegrator_modsChanged = 0;
bool AutoIntegrator_extractedLua = 0;
// returns UE4SS extracted mods path
bool AutoIntegrator_integrate(std::string paksPath1, std::string paksPath2, std::string folder_path, std::string outPath, std::string& out_ue4ssExtractionPath, std::wstring& out_IntegratorOutput)
{
    AutoIntegrator_extractedLua = 0;

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

    Output::send<LogLevel::Normal>(L"Performing integration...\n");

    if (paksPath1.empty() || paksPath1 == "default")
    {
        // default
        char* localappdata = getenv("LOCALAPPDATA");
        std::string localappdata2 = localappdata;
        paksPath1 = localappdata2 + "/Astro/Saved/Paks";
    }

    std::wstring game_exec_dir = UE4SSProgram::get_program().get_game_executable_directory();
    std::string game_exec_dir_narrow = converter.to_bytes(game_exec_dir);

    std::string cwd = AutoIntegrator_PassPathThroughShimloader(folder_path);

    std::string finalCmd = AutoIntegrator_GetExecutablePathForCmd(folder_path) + " -i \"" + AutoIntegrator_PassPathThroughShimloader(paksPath1) + "\" \"" + AutoIntegrator_PassPathThroughShimloader(paksPath2) + "\" -g \"" + (game_exec_dir_narrow + "/../../Content/Paks") + "\"";
    
    // outputFolder
    if (outPath.empty() || outPath == "default") outPath = paksPath1;
    outPath = AutoIntegrator_PassPathThroughShimloader(outPath);
    finalCmd += " -o \"" + outPath + "\"";

    std::string integratorPakPath = outPath + "/999-AstroModIntegrator_P.pak";
    // delete existing pak if we can to avoid success false positives
    try
    {
        std::filesystem::remove(integratorPakPath);
    }
    catch (...) {}

    // extractLua
    // we don't extract lua if cwd is a subdirectory of outPath
    // (that would mean that we are ourselves are one of the UE4SS mods that we would be trying to extract)
    if (!AutoIntegrator_IsSecondSubDirOfFirst(outPath, cwd))
    {
        finalCmd += " --extract_lua";
        // cleanLua is enabled by default

        AutoIntegrator_extractedLua = 1;
    }

    // enableCustomRoutines
    // security of custom routines is not much of a concern here because C++ UE4SS mods already can execute arbitrary code
    // so it's OK to always enable custom routines
    finalCmd += " --enable_custom_routines";

    // verbose
    finalCmd += " -v";

    // UNCOMMENT THIS TO INTENTIONALLY CRASH THE INTEGRATOR
    //finalCmd += " --pak_to_named_pipe AutoIntegrator_dummy";

    // check if mods.txt existed before
    bool modsTxtExistedBefore = 1;
    std::string modsTxtBefore = "";
    try
    {
        std::ifstream in(outPath + "/UE4SS/mods.txt", std::ios_base::in);
        std::stringstream buffer;
        buffer << in.rdbuf();
        in.close();
        modsTxtBefore = buffer.str();
    }
    catch(...)
    {
        modsTxtExistedBefore = 0;
    }
    try
    {
        if (!std::filesystem::exists(outPath + "/UE4SS/mods.txt")) modsTxtExistedBefore = 0;
    }
    catch (...)
    {
        modsTxtExistedBefore = 0;
    }

    // execute the integrator
    std::wstring finalCmd_wide = converter.from_bytes(finalCmd) + L"\n";
    Output::send<LogLevel::Verbose>(finalCmd_wide);

    std::string integrator_out = AutoIntegrator_exec(finalCmd.c_str(), cwd);
    AutoIntegrator_rtrim(integrator_out);
    integrator_out += "\n";
    std::wstring integrator_out_wide = converter.from_bytes(integrator_out);
    Output::send<LogLevel::Normal>(integrator_out_wide);

    out_IntegratorOutput = integrator_out_wide;

    // check after
    bool modsTxtExistedAfter = 1;
    std::string modsTxtAfter = "";
    try
    {
        std::ifstream in(outPath + "/UE4SS/mods.txt", std::ios_base::in);
        std::stringstream buffer;
        buffer << in.rdbuf();
        in.close();
        modsTxtAfter = buffer.str();
    }
    catch (...)
    {
        modsTxtExistedAfter = 0;
    }
    try
    {
        if (!std::filesystem::exists(outPath + "/UE4SS/mods.txt")) modsTxtExistedAfter = 0;
    }
    catch (...)
    {
        modsTxtExistedAfter = 0;
    }

    if (modsTxtExistedBefore && modsTxtExistedAfter && modsTxtBefore != modsTxtAfter) AutoIntegrator_modsChanged = 1; // if the mods.txt text changed (a new mod was added), we need to restart
    else if (modsTxtExistedBefore && !modsTxtExistedAfter) AutoIntegrator_modsChanged = 1; // if before it existed, and now it doesn't, we need to restart
    else if (!modsTxtExistedBefore && modsTxtExistedAfter) AutoIntegrator_modsChanged = 1; // if before it didn't exist, and now it does, we need to restart
    else if (!modsTxtExistedBefore && !modsTxtExistedAfter) AutoIntegrator_modsChanged = 0; // if didn't exist before or after, then no need to restart

    out_ue4ssExtractionPath = outPath + "/UE4SS";
    return std::filesystem::exists(integratorPakPath) || integrator_out_wide.contains(L"Finished integrating");
}

class AutoIntegrator : public RC::CppUserModBase
{
public:
    std::string ver;
    std::string folder_path;
    bool overrideEngineVersionAndSignatures;

    AutoIntegrator() : CppUserModBase()
    {
        ModName = STR("AutoIntegrator");
        ModVersion = STR("1.0.8");
        ModDescription = STR("atenfyr's AutoIntegrator, for loading classic AstroModLoader mods through UE4SS");
        ModAuthors = STR("atenfyr");

        // if unreal is already initialized (i.e., hot reload) then don't integrate again
        if (Unreal::UnrealInitializer::StaticStorage::bIsInitialized)
        {
            Output::send<LogLevel::Normal>(L"Hot reload detected, skipping integration\n");
            return;
        }

        UE4SSProgram& ue4ssProgram = UE4SSProgram::get_program();

        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

        folder_path = AutoIntegrator_get_dll_path() + "/../..";

        // pull config
        std::string paksPath;
        std::string outPath;
        std::string autoUpdateStr;
        std::string overrideEngineVersionAndSignaturesStr;
        std::ifstream in(folder_path + "/config.txt", std::ios_base::in);
        std::getline(in, paksPath);
        std::getline(in, outPath);
        std::getline(in, autoUpdateStr);
        std::getline(in, overrideEngineVersionAndSignaturesStr);
        in.close();

        AutoIntegrator_rtrim(paksPath);
        AutoIntegrator_rtrim(outPath);
        AutoIntegrator_rtrim(autoUpdateStr);
        AutoIntegrator_rtrim(overrideEngineVersionAndSignaturesStr);
        bool autoUpdate = !(autoUpdateStr == "false");
        overrideEngineVersionAndSignatures = !(overrideEngineVersionAndSignaturesStr == "false");

        // init
        try
        {
            ver = AutoIntegrator_exec(AutoIntegrator_GetExecutablePathForCmd(folder_path) + " version", "");
            AutoIntegrator_rtrim(ver);
        }
        catch (const std::runtime_error& err)
        {
            // force an auto-update and try again
            ver = "INVALID";

            // unless we can't auto-update...
            if (!autoUpdate)
            {
                Output::send<LogLevel::Error>(L"Failed to execute ModIntegrator: " + converter.from_bytes(err.what()) + L"\n");
                throw;
            }
        }
        catch (...)
        {
            // force an auto-update and try again
            ver = "INVALID";

            // unless we can't auto-update...
            if (!autoUpdate)
            {
                Output::send<LogLevel::Error>(L"Failed to execute ModIntegrator for an unknown reason\n");
                throw;
            }
        }

        if (autoUpdate)
        {
            AutoIntegrator_download_exe(folder_path, ver);

            // re-fetch version in case we auto-updated
            try
            {
                ver = AutoIntegrator_exec(AutoIntegrator_GetExecutablePathForCmd(folder_path) + " version", "");
                AutoIntegrator_rtrim(ver);
            }
            catch (const std::runtime_error& err)
            {
                Output::send<LogLevel::Error>(L"Failed to execute ModIntegrator: " + converter.from_bytes(err.what()) + L"\n");
                throw;
            }
            catch (...)
            {
                Output::send<LogLevel::Error>(L"Failed to execute ModIntegrator for an unknown reason\n");
                throw;
            }
        }

        std::wstring ver_wide_cpp = converter.from_bytes(ver);
        const wchar_t* ver_wide = ver_wide_cpp.c_str();

        std::wstring log_out_start = L"Initializing AutoIntegrator v" + ModVersion + L" (AstroModIntegrator Classic v";
        log_out_start += ver_wide_cpp;
        log_out_start += L") by atenfyr\n";
        Output::send<LogLevel::Normal>(log_out_start);

        std::wstring log_out_2 = L"Folder path: ";
        log_out_2 += converter.from_bytes(folder_path);
        log_out_2 += L"\n";
        Output::send<LogLevel::Verbose>(log_out_2);

        std::wstring logicMods_dir_wide = ue4ssProgram.get_game_executable_directory();
        std::string logicMods_dir = converter.to_bytes(logicMods_dir_wide);
        logicMods_dir += "/../../Content/Paks/LogicMods";
        AutoIntegrator_rtrim(logicMods_dir);
        CreateDirectoryW(converter.from_bytes(logicMods_dir).c_str(), NULL); // ignore any error from CreateDirectoryW

        if (outPath == "LogicMods") outPath = logicMods_dir;

        // some setup for popup window and restart logic
        bool restartGame = 0;
        bool ignoreCommandLineParametersRegardingRestarts = 0;
        wchar_t exe_path_buffer[1024];
        GetModuleFileNameW(GetModuleHandle(nullptr), exe_path_buffer, 1023);
        std::wstring game_exe_path = exe_path_buffer;

        // integrate
        std::string newModsDirectory = "";
        std::wstring out_IntegratorOutput = L"";
        bool success = AutoIntegrator_integrate(paksPath, logicMods_dir, folder_path, outPath, newModsDirectory, out_IntegratorOutput);

        // if something went wrong, we should display a pop-up warning, unless on server
        if (!success)
        {
            AutoIntegrator_extractedLua = 0;
            Output::send<LogLevel::Error>(L"!!! Integration failed !!!\n");
        }
        if (!success && !game_exe_path.contains(L"AstroServer"))
        {
            std::wstring popupMessage = L"AstroModIntegrator Classic " + ver_wide_cpp + L" was not able to successfully integrate your mods.\nWould you like to close the game, try again, or continue without mods?\n\n" + out_IntegratorOutput;
            int msgboxID = MessageBoxW(NULL, popupMessage.c_str(), TEXT("AutoIntegrator"), MB_CANCELTRYCONTINUE);
            switch (msgboxID)
            {
                case IDTRYAGAIN:
                    Output::send<LogLevel::Verbose>(L"User elected to try again; restarting game\n");
                    restartGame = 1;
                    ignoreCommandLineParametersRegardingRestarts = 1;
                    break;
                case IDCONTINUE:
                    Output::send<LogLevel::Verbose>(L"User elected to continue; doing nothing\n");
                    break;
                case IDCANCEL:
                default:
                    Output::send<LogLevel::Verbose>(L"User elected to cancel; closing game\n");
                    std::exit(0);
                    break;
            }
        }

        // delete all folders named "shared" except for the one under newModsDirectory
        // unless: newModsDirectory doesn't exist, then we keep the current shared folder unchanged
        // (also, don't do this unless we actually extracted lua mods)
        if (AutoIntegrator_extractedLua && !newModsDirectory.empty() && std::filesystem::is_directory(newModsDirectory))
        {
            std::filesystem::path beforeDir = AutoIntegrator_PassPathThroughShimloader(converter.from_bytes(newModsDirectory));
            std::filesystem::path beforeDirWithShared = beforeDir / "shared";

            for (std::filesystem::path this_mods_directory : ue4ssProgram.get_mods_directories())
            {
                std::filesystem::path pathToDelete = AutoIntegrator_PassPathThroughShimloader(this_mods_directory);
                if (pathToDelete == beforeDir) continue;
                
                pathToDelete = pathToDelete / "shared";

                std::error_code errorCode;
                std::filesystem::remove_all(pathToDelete, errorCode);
                if (!static_cast<bool>(errorCode) && !errorCode.message().contains("successfully")) // seems to always want to print even when succeeded... only print Normal not Error just in case
                {
                    Output::send<LogLevel::Normal>(L"Failed to delete directory \"" + pathToDelete.wstring() + L"\": " + converter.from_bytes(errorCode.message()) + L"\n");
                }

                // copy mods_directory shared folder here
                std::filesystem::copy(beforeDirWithShared, pathToDelete, std::filesystem::copy_options::overwrite_existing | std::filesystem::copy_options::recursive, errorCode);
                if (!static_cast<bool>(errorCode) && !errorCode.message().contains("successfully")) // seems to always want to print even when succeeded... only print Normal not Error just in case
                {
                    Output::send<LogLevel::Normal>(L"Failed to rename directory \"" + AutoIntegrator_PassPathThroughShimloader(beforeDirWithShared.wstring()) + L"\" to \"" + AutoIntegrator_PassPathThroughShimloader(pathToDelete.wstring()) + L"\": " + converter.from_bytes(errorCode.message()) + L"\n");
                }
            }
        }

        // update this code whenever any game update breaks UE4SS
        if (overrideEngineVersionAndSignatures && !ue4ssProgram.m_has_game_specific_config)
        {
            Output::send<LogLevel::Normal>(L"Overriding engine version and signatures for Astroneer\n");

            // get settings path
            std::wstring workingDirectory = ue4ssProgram.get_working_directory();
            std::filesystem::path finalSettingsPath = workingDirectory;
            finalSettingsPath.append(ue4ssProgram.m_settings_file_name);

            try
            {
                // edit settings file
                std::ifstream in(finalSettingsPath, std::ios_base::in);
                std::stringstream buffer;
                buffer << in.rdbuf();
                in.close();

                std::string outText = buffer.str();
                outText = std::regex_replace(outText, std::regex("MajorVersion\\s*=.*\\n"), "MajorVersion = 4\n");
                outText = std::regex_replace(outText, std::regex("MinorVersion\\s*=.*\\n"), "MinorVersion = 27\n");

                std::string outTextBefore = outText;
                if (AutoIntegrator_extractedLua) // AutoIntegrator_extractedLua is set after integration
                {
                    if (outText.contains("; AMLC"))
                    {
                        outText = std::regex_replace(outText, std::regex("; AMLC\\s*?\\n\\+ModsFolderPaths.*?\\n"), "; AMLC\n+ModsFolderPaths = " + newModsDirectory + "\n");
                    }
                    else
                    {
                        outText = std::regex_replace(outText, std::regex("\\[General\\]\\s*?\\n"), "; AMLC\n+ModsFolderPaths = " + newModsDirectory + "\n\n[General]\n");
                    }
                }
                if (outText != outTextBefore) restartGame = 1;

                std::ofstream out(finalSettingsPath, std::ios_base::out);
                out.write(outText.c_str(), outText.length());
                out.close();
            }
            catch (const std::runtime_error& err)
            {
                Output::send<LogLevel::Error>(L"Failed to edit settings file with updated engine version: " + converter.from_bytes(err.what()) + L"\n");
                throw;
            }
            catch (...)
            {
                Output::send<LogLevel::Error>(L"Failed to edit settings file with updated engine version for an unknown reason\n");
                throw;
            }

            try
            {
                // reload settings
                ue4ssProgram.settings_manager.deserialize(finalSettingsPath);
            }
            catch (const std::runtime_error& err)
            {
                Output::send<LogLevel::Error>(L"Failed to reload settings file: " + converter.from_bytes(err.what()) + L"\n");
                throw;
            }
            catch (...)
            {
                Output::send<LogLevel::Error>(L"Failed to reload settings file for an unknown reason\n");
                throw;
            }

            // provide signature
            // this relies on UE4SS commit d0479fd or later
            try
            {
                std::string signatureValue = "-- Signature produced by CorporalWill123\nfunction Register()\n    return \"FF 50 08 90 48 8B C7 83 4F 10 12 4C 8D 5C 24 70 49 8B 5B 28\"\nend\n\nfunction OnMatchFound(MatchAddress)\n    return MatchAddress - 0x173\nend\n\n";

                CreateDirectoryW((ue4ssProgram.get_working_directory() + L"/UE4SS_Signatures").c_str(), NULL); // ignore any error from CreateDirectoryW
                std::ofstream fs(ue4ssProgram.get_working_directory() + L"/UE4SS_Signatures/FText_Constructor.lua", std::ios::out);
                fs.write(signatureValue.c_str(), signatureValue.length());
                fs.close();
            }
            catch (const std::runtime_error& err)
            {
                Output::send<LogLevel::Error>(L"Failed to create FText_Constructor.lua: " + converter.from_bytes(err.what()) + L"\n");
                throw;
            }
            catch (...)
            {
                Output::send<LogLevel::Error>(L"Failed to create FText_Constructor.lua for an unknown reason\n");
                throw;
            }
        }

        // restart game if need
        if (AutoIntegrator_modsChanged) restartGame = 1;

        int nArgs = 0;
        std::wstring all_args_as_string = L"";
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &nArgs);
        for (int i = 0; i < nArgs; i++)
        {
            std::wstring argument = argv[i];
            if (argument == L"--AutoIntegratorReboot")
            {
                if (!ignoreCommandLineParametersRegardingRestarts)
                {
                    Output::send<LogLevel::Normal>(L"Forcing restartGame = 1\n");
                    restartGame = 1;
                }
            }
            else if (argument == L"--AutoIntegratorNoReboot")
            {
                if (!ignoreCommandLineParametersRegardingRestarts)
                {
                    Output::send<LogLevel::Normal>(L"Forcing restartGame = 0\n");
                    if (restartGame) Output::send<LogLevel::Warning>(L"restartGame was 1 before!\n");
                    restartGame = 0;
                }
            }
            else
            {
                if (argument.find(' ') != std::string::npos)
                {
                    all_args_as_string += L"\"" + argument + L"\" ";
                }
                else
                {
                    all_args_as_string += argument + L" ";
                }
            }
        }
        LocalFree(argv);

        if (restartGame)
        {
            Output::send<LogLevel::Normal>(L"Restarting game\n");

            try
            {
                auto const errorCode = reinterpret_cast<INT_PTR>(ShellExecuteW(
                    NULL,
                    L"open",
                    game_exe_path.c_str(),
                    (all_args_as_string + L"--AutoIntegratorNoReboot").c_str(),
                    NULL,
                    SW_SHOWNORMAL
                ));
                if (errorCode <= 32) throw std::runtime_error(std::to_string(errorCode));
            }
            catch (const std::runtime_error& err)
            {
                Output::send<LogLevel::Error>(L"Failed to launch new instance of the game: " + converter.from_bytes(err.what()) + L"\n");
                Output::send<LogLevel::Error>(L"Please restart the game manually\n");
            }
            catch (...)
            {
                Output::send<LogLevel::Error>(L"Failed to launch new instance of the game for an unknown reason\n");
                Output::send<LogLevel::Error>(L"Please restart the game manually\n");
            }

            // close game
            std::exit(0);
        }
    }

    ~AutoIntegrator() override
    {
    }

    auto on_update() -> void override
    {

    }
};

#define AUTO_INTEGRATOR_API __declspec(dllexport)
extern "C"
{
    AUTO_INTEGRATOR_API RC::CppUserModBase* start_mod()
    {
        return new AutoIntegrator();
    }

    AUTO_INTEGRATOR_API void uninstall_mod(RC::CppUserModBase* mod)
    {
        delete mod;
    }
}
