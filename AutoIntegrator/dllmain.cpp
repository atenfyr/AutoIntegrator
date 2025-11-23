#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "cpp-httplib/httplib.h"
#include <stdio.h>
#include <fstream>
#include <UE4SSProgram.hpp>
#include <Mod/CppUserModBase.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <windows.h>

using namespace RC;

// BEGIN NON-MIT LICENSED SECTION //

// THE FOLLOWING METHODS ARE ADAPTED FROM STACK OVERFLOW ANSWERS!
// THEY ARE NOT LICENSED UNDER MIT. SEE COMMENTS FOR FURTHER INFORMATION

// CC BY-SA 2.5 (https://creativecommons.org/licenses/by-sa/2.5/se/deed.en)
// This code is copyrighted by StackOverflow user "waqas" https://stackoverflow.com/users/58008/waqas
// Minor changes were made to this source code from the original. No warranties are given. See the original license text for more information.
// https://stackoverflow.com/a/478960
std::string AutoIntegrator_exec(std::string cmd_cpp) {
    const char* cmd = cmd_cpp.c_str();
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

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
        (LPCWSTR)&AutoIntegrator_exec, &hm) == 0)
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
    try
    {
        if (success1 && latest_ver != ver) // did we successfully get the latest version, and do we actually need to update?
        {
            Output::send<LogLevel::Verbose>(L"Updating AstroModIntegrator Classic...\n");
            if (auto res = cli.Get("/atenfyr/AstroModLoader-Classic/releases/download/v" + latest_ver + "/ModIntegrator-win-x64.exe", headers))
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
                    std::ofstream fs(folder_path + "/ModIntegrator.exe", std::ios::out | std::ios::binary);
                    fs.write((res->body).data(), (res->body).size());
                    fs.close();
                    Output::send<LogLevel::Normal>(L"Successfully downloaded ModIntegrator.exe\n");
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

void AutoIntegrator_integrate(std::string paksPath1, std::string paksPath2, std::string folder_path, std::string outPath)
{
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

    std::string finalCmd = folder_path + "/ModIntegrator.exe [ " + paksPath1 + " " + paksPath2 + " ] " + game_exec_dir_narrow + "/../../Content/Paks";
    if (!outPath.empty() && outPath != "default") finalCmd += " " + outPath;
    std::wstring finalCmd_wide = converter.from_bytes(finalCmd) + L"\n";
    Output::send<LogLevel::Verbose>(finalCmd_wide);

    std::string integrator_out = AutoIntegrator_exec(finalCmd.c_str());
    AutoIntegrator_rtrim(integrator_out);
    integrator_out += "\n";
    std::wstring integrator_out_wide = converter.from_bytes(integrator_out);
    Output::send<LogLevel::Normal>(integrator_out_wide);
}

class AutoIntegrator : public RC::CppUserModBase
{
public:
    std::string ver;
    std::string folder_path;

    AutoIntegrator() : CppUserModBase()
    {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

        // init
        folder_path = AutoIntegrator_get_dll_path() + "/../..";
        ver = AutoIntegrator_exec(folder_path + "/ModIntegrator.exe version");
        AutoIntegrator_rtrim(ver);

        // pull config
        std::string paksPath;
        std::string outPath;
        std::string autoUpdateStr;
        std::ifstream in(folder_path + "/config.txt", std::ios_base::in);
        std::getline(in, paksPath);
        std::getline(in, outPath);
        std::getline(in, autoUpdateStr);
        in.close();

        AutoIntegrator_rtrim(paksPath);
        AutoIntegrator_rtrim(outPath);
        AutoIntegrator_rtrim(autoUpdateStr);
        bool autoUpdate = !(autoUpdateStr == "false");

        if (autoUpdate)
        {
            AutoIntegrator_download_exe(folder_path, ver);

            // re-fetch version in case we auto-updated
            ver = AutoIntegrator_exec(folder_path + "/ModIntegrator.exe version");
            AutoIntegrator_rtrim(ver);
        }

        std::wstring ver_wide_cpp = converter.from_bytes(ver);
        const wchar_t* ver_wide = ver_wide_cpp.c_str();

        ModName = STR("AutoIntegrator");
        ModVersion = ver_wide;
        ModDescription = STR("atenfyr's AutoIntegrator, for loading classic AstroModLoader mods through UE4SS");
        ModAuthors = STR("atenfyr");
        // Do not change this unless you want to target a UE4SS version
        // other than the one you're currently building with somehow.
        //ModIntendedSDKVersion = STR("2.6");

        std::wstring log_out_start = L"Initializing AutoIntegrator v";
        log_out_start += ver_wide_cpp;
        log_out_start += L" by atenfyr\n";
        Output::send<LogLevel::Normal>(log_out_start);

        std::wstring log_out_2 = L"Folder path: ";
        log_out_2 += converter.from_bytes(folder_path);
        log_out_2 += L"\n";
        Output::send<LogLevel::Verbose>(log_out_2);

        std::wstring logicMods_dir_wide = UE4SSProgram::get_program().get_game_executable_directory();
        std::string logicMods_dir = converter.to_bytes(logicMods_dir_wide);
        logicMods_dir += "/../../Content/Paks/LogicMods";
        AutoIntegrator_rtrim(logicMods_dir);

        if (outPath == "LogicMods") outPath = logicMods_dir;

        AutoIntegrator_integrate(paksPath, logicMods_dir, folder_path, outPath);
    }

    ~AutoIntegrator() override
    {
    }

    auto on_program_start() -> void override
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
