@echo off
setlocal
cd /d "%~dp0"
if exist "Play Bloodborne.exe" (
  start "" "Play Bloodborne.exe"
  exit /b 0
)
if exist "run.py" (
  py -3 run.py
  exit /b 0
)
echo Copy this overlay onto a BBPort 1.8.2 folder first (Play Bloodborne.exe + bin\).
pause
