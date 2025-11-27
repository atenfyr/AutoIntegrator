@echo off
WHERE repak >nul 2>nul
IF %ERRORLEVEL% NEQ 0 (
    ECHO repak.exe not found in PATH or current working directory; cannot proceed
    goto :done
)

del /S /Q "./pak/UE4SS"
xcopy /s /e /y "./out/mod" "./pak/UE4SS"

move /y ".\pak\UE4SS\config.txt" ".\pak\UE4SS\config2.txt"

>".\pak\UE4SS\config.txt" (
  for /f "usebackq delims=" %%A in (".\pak\UE4SS\config2.txt") do (
    if "%%A" equ "LogicMods" (echo default) else (echo %%A)
  )
)

repak pack --version V4 --compression Zlib "./pak" "./out/PAK-FOR-AML-CLASSIC/000-AutoIntegratorForAML-1.0.1_P.pak"
echo.
echo All done!

:done
pause
