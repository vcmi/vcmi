@echo off
title VCMI Installer Builder
setlocal enabledelayedexpansion
cls

REM Define variables dynamically relative to the normalized base directory
set AppVersion=1.6.0
set AppBuild=ec25eb5
set InstallerArch=x64

REM Define Inno Setup version
set InnoSetupVer=6

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

REM Call Inno Setup Compiler
"%ProgFiles%\Inno Setup %InnoSetupVer%\ISCC.exe" "%InstallerScript%" ^
    /DAppVersion="%AppVersion%" ^
    /DAppBuild="%AppBuild%" ^
    /DInstallerArch="%InstallerArch%" ^
    /DSourceFilesPath="%SourceFilesPath%" ^
    /DLangPath="%LangPath%" ^
    /DLicenseFile="%LicenseFile%" ^
    /DIconFile="%IconFile%" ^
    /DSmallLogo="%SmallLogo%" ^
    /DWizardLogo="%WizardLogo%"

pause