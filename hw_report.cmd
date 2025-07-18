@echo off
chcp 65001 > nul
title hw_report
net session >nul 2>&1
if %errorlevel% neq 0 (
    %WINDIR%\System32\WindowsPowerShell\v1.0\powershell.exe -Command "Start-Process -FilePath '%~f0' -ArgumentList 'am_admin' -Verb RunAs"
    exit /b
)

pushd "%CD%"
CD /D "%~dp0"

if not exist ".\hw_report.ps1" (
    echo The file hw_report.ps1 was not found!
    echo Make sure it is in the same folder as this script.
    pause
    exit /b 1
)

powershell -NoProfile -ExecutionPolicy Bypass -File ".\hw_report.ps1"
