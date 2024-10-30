@echo off
setlocal enabledelayedexpansion

REM ensure working location is at batch file level:
cd /d "%~dp0"

REM extract version:
set VERSION_FILE=..\version.h
for /f "tokens=3 delims= " %%A in ('findstr /r /c:"#define[ \t]VERSION[ \t]" %VERSION_FILE%') do (
    set VERSION=%%A
)

REM target file name:
set OUTPUT=Starship_%VERSION%

REM create temp folder:
set TEMP=temp
mkdir %TEMP%

REM copy useful files to temp folder:
copy "..\readme.md" "%TEMP%\readme.md" >nul
copy "..\.runtime\Release\resource.dat" "%TEMP%\resource.dat" >nul
copy "..\.runtime\Release\starship.exe" "%TEMP%\%OUTPUT%.exe" >nul

REM zip files:
set ZIP=%OUTPUT%.zip
tar -caf "%ZIP%" -C "%TEMP%" *

REM remove temp folder:
rmdir /S /Q "%TEMP%"

REM summary:
echo Successfully packaged '%ZIP%' archive.