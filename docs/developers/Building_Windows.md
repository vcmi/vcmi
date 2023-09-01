# Preparations
Windows builds can be made in more than one way and with more than one tool. This guide focuses on the simplest building process using Microsoft Visual Studio 2022

# Prerequisites

- Windows Vista or newer.
- [Microsoft Visual Studio](https://visualstudio.microsoft.com/downloads/)
- Git or git GUI, for example, SourceTree [download](http://www.sourcetreeapp.com/download)
- CMake [download](https://cmake.org/download/). During install after accepting license agreement make sure to check "Add CMake to the system PATH for all users".
- To unpack pre-build Vcpkg: [7-zip](http://www.7-zip.org/download.html)
- Optionally, to create installer: [NSIS](http://nsis.sourceforge.net/Main_Page)

## Choose an installation directory

Create a directory for VCMI development, eg. `C:\VCMI` We will call this directory `%VCMI_DIR%`

Warning! Replace `%VCMI_DIR%` with path you've chosen for VCMI installation in the following commands.

It is recommended to avoid non-ascii characters in the path to your working folders. The folder should not be write-protected by system.   
Good locations:
- `C:\VCMI`
Bad locations:
- `C:\Users\Michał\VCMI (non-ascii character)`
- `C:\Program Files (x86)\VCMI (write protection)`

# Install VCMI dependencies

You have two options: to use pre-built libraries or build your own. We strongly recommend start with using pre-built ones.

## Option A. Use pre-built Vcpkg

### Download and unpack archive

Vcpkg Archives are available at our GitHub: https://github.com/vcmi/vcmi-deps-windows/releases

- Download latest version available.
EG: v1.6 assets - [vcpkg-export-x64-windows-v143.7z](https://github.com/vcmi/vcmi-deps-windows/releases/download/v1.6/vcpkg-export-x64-windows-v143.7z)  
- Extract archive by right clicking on it and choosing "7-zip -> Extract Here".

### Move dependencies to target directory
Once extracted, a `vcpkg` directory will appear with `installed` and `scripts` subfolders inside.
Move extracted `vcpkg` directory into your `%VCMI_DIR%`

## Option B. Build Vcpkg on your own

Please be aware that if you're running 32-bit Windows version, then this is impossible due to <https://github.com/microsoft/vcpkg/issues/26036>
Be aware that building Vcpkg might take a lot of time depend on your CPU model and 10-20GB of disk space.

### Create initial directory

### Clone vcpkg

1.  open SourceTree
2.  File -\> Clone
3.  select **<https://github.com/microsoft/vcpkg/>** as source
4.  select **%VCMI_DIR%/vcpkg** as destination
5.  click **Clone**

From command line use:

    git clone https://github.com/microsoft/vcpkg.git %VCMI_DIR%/vcpkg

### Build vcpkg and dependencies

- Run `%VCMI_DIR%/vcpkg/bootstrap-vcpkg.bat`
- For 32-bit build run: `%VCMI_DIR%/vcpkg/vcpkg.exe install tbb:x64-windows fuzzylite:x64-windows sdl2:x64-windows sdl2-image:x64-windows sdl2-ttf:x64-windows sdl2-mixer[mpg123]:x64-windows boost:x64-windows qt5-base:x64-windows ffmpeg:x64-windows luajit:x64-windows`
- For 64-bit build run: `%VCMI_DIR%/vcpkg/vcpkg.exe install install tbb:x86-windows fuzzylite:x86-windows sdl2:x86-windows sdl2-image:x86-windows sdl2-ttf:x86-windows sdl2-mixer[mpg123]:x86-windows boost:x86-windows qt5-base:x86-windows ffmpeg:x86-windows luajit:x86-windows`

For the list of the packages used you can also consult [vcmi-deps-windows readme](https://github.com/vcmi/vcmi-deps-windows) in case this article gets outdated a bit.

# Build VCMI

#### From GIT GUI
- Open SourceTree
- File -> Clone
- select `https://github.com/vcmi/vcmi/` as source
- select `%VCMI_DIR%/source` as destination
- expand Advanced Options and change Checkout Branch to `develop`
- tick `Recursive submodules`
- click Clone

#### From command line  
- `git clone --recursive https://github.com/vcmi/vcmi.git %VCMI_DIR%/source`  

## Generate solution for VCMI  
- Create `%VCMI_DIR%/build` folder  
- Open a command line prompt at `%VCMI_DIR%/build`  
- Execute `cd %VCMI_DIR%/build`    
- Create solution (Visual Studio 2022 64-bit) `cmake %VCMI_DIR%/source -DCMAKE_TOOLCHAIN_FILE=%VCMI_DIR%/vcpkg/scripts/buildsystems/vcpkg.cmake -G "Visual Studio 17 2022" -A x64`

## Compile VCMI with Visual Studio
- Open `%VCMI_DIR%/build/VCMI.sln` in Visual Studio
- Select `Release` build type in combobox
- Right click on `BUILD_ALL` project. This `BUILD_ALL` project should be in `CMakePredefinedTargets` tree in Solution Explorer.
- VCMI will be built in `%VCMI_DIR%/build/bin` folder!

# Create VCMI installer (This step is not required for just building & development)

Make sure NSIS is installed to default directory or have registry entry so CMake can find it.
After you build VCMI execute following commands from `%VCMI_DIR%/build`.

- for release build: `cpack`
- for debug build: `cpack -C Debug`

# Troubleshooting and workarounds

Vcpkg might be very unstable due to limited popularity and fact of using bleeding edge packages (such as most recent Boost). Using latest version of dependencies could also expose both problems in VCMI code or library interface changes that developers not checked yet. So if you're built Vcpkg yourself and can't get it working please try to use binary package.

Pre-built version we provide is always manually tested with all supported versions of MSVC for both Release and Debug builds and all known quirks are listed below.

#$# Build is successful but can not start new game
Make sure you have:
* Installed Heroes III from disk or using GOG installer
* Copied `Data`, `Maps` and `Mp3` folders from Heroes III to: `%USERPROFILE%\Documents\My Games\vcmi\`

### VCMI won't run since some library is missing

**If you open solution using vcmi.sln** Try to build INSTALL target and see if its output works as expected. Copy missing libraries or even all libraries from there to your build directory. Or run cpack and use produced installer and see if you can get libs from there. cpack -V will give more info. If cpack complains that it can not find dumpbin try Visual Studio Command Prompt (special version of cmd provided with Visual Studio with additional PATH) or modify PATH to have this tool available. Another alternative if you use prebuilt vcpkg package is to download latest msvc build, install it and copy missing/all libraries from there.

### Debug build is very slow

Debug builds with MSVC are generally extremely slow since it's not just VCMI binaries are built as debug, but every single dependency too and this usually means no optimizations at all. Debug information that available for release builds is often sufficient so just avoid full debug builds unless absolutely necessary. Instead use RelWithDebInfo configuration. Also Debug configuration might have some compilation issues because it is not checked via CI for now.

### I got crash within library XYZ.dll

VCPKG generated projects quite often have both debug and regular libs available to linker so it can select wrong lib. For stable RelWithDebInfo build you may try to remove debug folder from VCPKG/installed/x64-windows. Same is done on CI. Also it reduces package size at least twice.
