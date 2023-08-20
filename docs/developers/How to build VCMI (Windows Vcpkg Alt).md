# Preparations
Windows builds can be made in more than one way and with more than one tool. This guide focuses on the simplest building process using Microsoft Visual Studio 2022
## Prerequisites
Windows Vista or newer.  
Microsoft Visual Studio 2022 [download](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&channel=Release&version=VS2022&source=VSLandingPage&cid=2030&passive=false)  
Git or git GUI, for example: SourceTree [download](https://www.sourcetreeapp.com/download), or GitKraken [download](https://www.gitkraken.com/download)  
CMake [download](https://cmake.org/download/). During install after accepting license agreement make sure to check _"Add CMake to the system PATH for all users"_.  
[7-zip](https://www.7-zip.org/download.html) For unpacking pre-build Vcpkg  

## Choose an installation directory
Create a directory for VCMI development, eg. `C:\VCMI` We will call this directory `%VCMI_DIR%`

> Warning! Replace `%VCMI_DIR%` with path you've chosen for VCMI installation in the following commands.

> It is recommended to avoid non-ascii characters in the path to your working folders. The folder should not be write-protected by system.   
Good locations:  
`C:\VCMI`  
Bad locations:  
`C:\Users\Michał\VCMI (non-ascii character)`  
`C:\Program Files (x86)\VCMI (write protection)`  

## Install VCMI dependencies
We strongly recommend to use pre-built vcpkg.

### Download and unpack vcpkg archive
Vcpkg Archives are available at our GitHub: https://github.com/vcmi/vcmi-deps-windows/releases

* Download latest version available.
EG: v1.6 assets - [vcpkg-export-x64-windows-v143.7z](https://github.com/vcmi/vcmi-deps-windows/releases/download/v1.6/vcpkg-export-x64-windows-v143.7z)  
* Extract archive by right clicking on it and choosing "7-zip -> Extract Here".

### Move dependencies to target directory
Once extracted, a `vcpkg` directory will appear with `installed` and `scripts` subfolders inside.
Move extracted `vcpkg` directory into your `%VCMI_DIR%`

# Build VCMI
## Clone VCMI
#### From GIT GUI
* Open SourceTree
* File -> Clone
* select `https://github.com/vcmi/vcmi/` as source
* select `%VCMI_DIR%/source` as destination
* expand Advanced Options and change Checkout Branch to `develop`
* tick `Recursive submodules`
* click Clone  
#### From command line  
* `git clone --recursive https://github.com/vcmi/vcmi.git %VCMI_DIR%/source`  

## Generate solution for VCMI  
* Create `%VCMI_DIR%/build` folder  
* Open a command line prompt at `%VCMI_DIR%/build`  
* Execute `cd %VCMI_DIR%/build`    
* Create solution (Visual Studio 2022 64-bit) `cmake %VCMI_DIR%/source -DCMAKE_TOOLCHAIN_FILE=%VCMI_DIR%/vcpkg/scripts/buildsystems/vcpkg.cmake -G "Visual Studio 17 2022" -A x64`

## Compile VCMI with Visual Studio
Open `%VCMI_DIR%/build/VCMI.sln` in Visual Studio
Select `Release` build type in combobox
Right click on `BUILD_ALL` project. This `BUILD_ALL` project should be in `CMakePredefinedTargets` tree in Solution Explorer.
VCMI will be built in `%VCMI_DIR%/build/bin` folder!

# Notes
## Build is successful but can not start new game
Make sure you have:
* Installed Heroes III from disk or using GOG installer
* Copied `Data`, `Maps` and `Mp3` folders from Heroes III to: `%USERPROFILE%\Documents\My Games\vcmi\`

## Debug build is very slow
Debug builds with MSVC are generally extremely slow since it's not just VCMI binaries are built as debug, but every single dependency too and this usually means no optimizations at all. Debug information that available for release builds is often sufficient so just avoid full debug builds unless absolutely necessary. Instead use RelWithDebInfo configuration. Also Debug configuration might have some compilation issues because it is not checked via CI for now.

## Further notes:
For installing / uninstalling mods you need to launch `VCMI_launcher.exe`  
For Map Editor you need to launch `VCMI_mapeditor.exe`  
