@echo off
title VCMI Installer Builder
setlocal enabledelayedexpansion
cls

REM Define variables dynamically relative to the normalized base directory
set "AppVersion=1.7.0"
set "AppBuild=1122334455A"
set "InstallerArch=x64compatible"
set "VCMIFolder=VCMI"
set "InstallerName=VCMI-Windows"

REM Override defaults with optional parameters
if not "%~1"=="" set "AppVersion=%~1"
if not "%~2"=="" set "AppBuild=%~2"
if not "%~3"=="" set "InstallerArch=%~3"
if not "%~4"=="" set "VCMIFolder=%~4"
if not "%~5"=="" set "InstallerName=%~5"
if not "%~6"=="" set "SourceFilesPath=%~6"
if not "%~7"=="" set "UCRTFilesPath=%~7"

if not "%InstallerArch%" == "arm64" (
    set "AllowedArch=%InstallerArch%compatible"
) else (
    set "AllowedArch=%InstallerArch%"
)

REM Define Inno Setup version
set InnoSetupVer=6

REM Uncomment this line and set custom UCRT source path, otherwise latest installed Windows 10 SDK will be used
REM set "UCRTFilesPath=%ProgFiles%\Windows Kits\10\Redist\10.0.22621.0\ucrt\DLLs"

REM Normally, there is no need to modify anything below this line.

REM Determine the base directory two levels up from the installer location
set "ScriptDir=%~dp0"
set "BaseDir=%ScriptDir%..\..\"

REM Normalize the base directory
for %%i in ("%BaseDir%") do set "BaseDir=%%~fi"

REM Define specific subdirectories relative to the base directory
if not defined SourceFilesPath set "SourceFilesPath=%BaseDir%bin\Release"
set "LangPath=%BaseDir%CI\wininstaller\lang"
set "LicenseFile=%BaseDir%license.txt"
set "IconFile=%BaseDir%clientapp\icons\vcmi.ico"
set "SmallLogo=%BaseDir%CI\wininstaller\vcmismalllogo.bmp"
set "WizardLogo=%BaseDir%CI\wininstaller\vcmilogo.bmp"
set "InstallerScript=%BaseDir%CI\wininstaller\installer.iss"

REM Determine Program Files directory based on system architecture
if exist "%WinDir%\SysWow64" (
    set "ProgFiles=%programfiles(x86)%"
) else (
    set "ProgFiles=%programfiles%"
)

set "ISCC=%ProgFiles%\Inno Setup %InnoSetupVer%\ISCC.exe"

REM Github should have it installed in different location
if not exist "%ISCC%" (
    set "ISCC=%SystemDrive%\ProgramData\Chocolatey\bin\ISCC.exe"
)

REM Dynamically locate the UCRT path if not defined
if not defined UCRTFilesPath (
	set "UCRTBasePath=!ProgFiles!\Windows Kits\10\Redist"
	set "UCRTFilesPath="
	for /f "delims=" %%d in ('dir /b /ad /on "!UCRTBasePath!"') do (
		if exist "!UCRTBasePath!\%%d\ucrt\DLLs" (
			set "UCRTFilesPath=!UCRTBasePath!\%%d\ucrt\DLLs"
		)
	)
)

REM Verify Inno Setup is installed
if not exist "%ISCC%" (
    echo.
    echo ERROR: Inno Setup !InnoSetupVer! was not found in !ProgFiles!.
    echo Please install it or specify the correct path.
    echo.
    pause
    goto :eof
)

REM Verify critical paths
if not exist "%InstallerScript%" (
    echo ERROR: Installer script not found: !InstallerScript!
    pause
    goto :eof
)
if not exist "%SourceFilesPath%" (
    echo ERROR: Source files path not found: !SourceFilesPath!
    pause
    goto :eof
)
if not exist "%UCRTFilesPath%" (
    echo ERROR: UCRT files path not found: !UCRTFilesPath!
    pause
    goto :eof
)

REM Print out installer settings
echo.
echo AppVersion:        %AppVersion%
echo AppBuild:          %AppBuild%
echo InstallerArch:     %InstallerArch%
echo AllowedArch:       %AllowedArch%
echo VCMIFolder:        %VCMIFolder%
echo InstallerName:     %InstallerName%
echo SourceFilesPath:   %SourceFilesPath%
echo UCRTFilesPath:     %UCRTFilesPath%
echo InstallerScript:   %InstallerScript%
echo.

REM Call Inno Setup Compiler
"%ISCC%" "%InstallerScript%" ^
    /DAppVersion="%AppVersion%" ^
    /DAppBuild="%AppBuild%" ^
    /DInstallerArch="%InstallerArch%" ^
    /DAllowedArch="%AllowedArch%" ^
    /DVCMIFolder="%VCMIFolder%" ^
    /DInstallerName="%InstallerName%" ^
    /DSourceFilesPath="%SourceFilesPath%" ^
    /DUCRTFilesPath="%UCRTFilesPath%" ^
    /DLangPath="%LangPath%" ^
    /DLicenseFile="%LicenseFile%" ^
    /DIconFile="%IconFile%" ^
    /DSmallLogo="%SmallLogo%" ^
    /DWizardLogo="%WizardLogo%"

goto :eof
