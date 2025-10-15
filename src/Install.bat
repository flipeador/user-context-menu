@echo off
setlocal

set SCRIPT_DIR=%~dp0
set PS_SCRIPT=%SCRIPT_DIR%Install.ps1

powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%PS_SCRIPT%"

endlocal
pause
