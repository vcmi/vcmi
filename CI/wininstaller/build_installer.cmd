@echo off
title VCMI Installer Builder
setlocal enabledelayedexpansion
cls

REM Define variables dynamically relative to the normalized base directory
set "AppVersion=1.6.1"
set "AppBuild=1122334455A"
set "InstallerArch=x64"
set "VCMIFolder=VCMI"

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
set "SourceFilesPath=%BaseDir%bin\Release"
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
if not exist "%ProgFiles%\Inno Setup %InnoSetupVer%\ISCC.exe" (
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

REM Call Inno Setup Compiler
"%ProgFiles%\Inno Setup %InnoSetupVer%\ISCC.exe" "%InstallerScript%" ^
    /DAppVersion="%AppVersion%" ^
    /DAppBuild="%AppBuild%" ^
    /DInstallerArch="%InstallerArch%" ^
    /DSourceFilesPath="%SourceFilesPath%" ^
    /DUCRTFilesPath="%UCRTFilesPath%" ^
    /DVCMIFolder="%VCMIFolder%" ^
    /DLangPath="%LangPath%" ^
    /DLicenseFile="%LicenseFile%" ^
    /DIconFile="%IconFile%" ^
    /DSmallLogo="%SmallLogo%" ^
    /DWizardLogo="%WizardLogo%"

pause