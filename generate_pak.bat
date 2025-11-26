@echo off
WHERE repak >nul 2>nul
IF %ERRORLEVEL% NEQ 0 (
    ECHO repak.exe not found in PATH or current working directory; cannot proceed
    goto :done
)

del /S /Q "./pak/UE4SS"
xcopy /s /e /y "./out/mod" "./pak/UE4SS"
repak pack --version V4 --compression Zlib "./pak" "./out/PAK-FOR-AML-CLASSIC/000-AutoIntegratorForAML-1.0.0_P.pak"
echo.
echo All done!

:done
pause
