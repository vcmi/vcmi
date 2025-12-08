# Building VCMI for Windows

## Preparations

Windows builds can be made in more than one way and with more than one tool. This guide will show how to do it using Microsoft Visual Studio 2022 (MSVC compiler) and MSYS2 (MinGW compiler).

## Prerequisites

1. Windows 7 or newer
2. [Microsoft Visual Studio](https://visualstudio.microsoft.com/downloads/)
3. Git or git GUI, for example, SourceTree [download link](http://www.sourcetreeapp.com/download)
4. CMake [download link](https://cmake.org/download/). During install after accepting license agreement make sure to check "Add CMake to the system PATH for all users". You can also install CMake from the Visual Studio Installer or from a package manager like [WinGet](https://github.com/microsoft/winget-cli).
5. Optional:
    - To create installer: [Inno Setup](https://jrsoftware.org/isinfo.php)
    - To speed up recompilation: [CCache](https://github.com/ccache/ccache/releases)

### Choose an installation directory

Create a directory for VCMI development, eg. `C:\VCMI` We will call this directory `%VCMI_DIR%`

Warning! Replace `%VCMI_DIR%` with path you've chosen for VCMI installation in the following commands.

It is recommended to avoid non-ascii characters in the path to your working folders. The folder should not be write-protected by system.

Good locations:

- `C:\VCMI`

Bad locations:

- `C:\Users\Michał\VCMI (non-ascii character)`
- `C:\Program Files (x86)\VCMI (write protection)`

## Install VCMI dependencies

This step is needed only for MSVC compiler. You can also find legacy Vcpkg instructions in the bottom of the document.

We use Conan package manager to build/consume dependencies, find detailed usage instructions [here](./Conan.md). Note that the link points to the state of the current branch, for the latest release check the same document in the [master branch](https://github.com/vcmi/vcmi/blob/master/docs/developers/Conan.md).

On the step where you need to replace **PROFILE**, choose:

- `msvc-x64` to build for Intel 64-bit (x64 / x86_64)
- `msvc-arm64` to build for ARM 64-bit (arm64)
- `msvc-x86` to build for Intel 32-bit (x86)

*Note*: we recommend using CMD (`cmd.exe`) for the next steps. If you absolutely want to use Powershell, then append `-c tools.env.virtualenv:powershell=powershell.exe` to the `conan install` command.

## Install CCache

Extract `ccache` to a folder of your choosing, add the folder to the `PATH` environment variable and log out and back in.

## Build VCMI

### Clone VCMI repository

#### From Git GUI

1. Open SourceTree
2. File -> Clone
3. Select `https://github.com/vcmi/vcmi/` as source
4. Select `%VCMI_DIR%/source` as destination
5. Expand Advanced Options and change Checkout Branch to `develop`
6. Tick `Recursive submodules`
7. Click Clone

#### From command line

```sh
git clone --recursive https://github.com/vcmi/vcmi.git %VCMI_DIR%/source
```

### Compile VCMI with Visual Studio

#### Generate solution

1. Open command line prompt (`cmd.exe`)
2. Execute `cd %VCMI_DIR%`
3. Now you need to run a script* from the Conan directory that you passed to `conan install` (the one that you passed in `--output-folder` parameter). For example, if you passed `conan-msvc`, then the script will be in `source\conan-msvc`.
    - for CMD: `source\conan-msvc\conanrun.bat`
    - for Powershell: `source\conan-msvc\conanrun.ps1`. If it gives an error, also run `Set-ExecutionPolicy -Scope CurrentUser -ExecutionPolicy Unrestricted`
4. Create VS solution: `cmake -S source -B build --toolchain source\conan-msvc\conan_toolchain.cmake`

\* This script sets up `PATH` required for Qt tools (`moc`, `uic` etc.) that run during CMake configure and build steps. Those tools depend on `zlib.dll` that was built with Conan, therefore its directory must be in `PATH`.

#### Build solution

You must launch Visual Studio in a modified `PATH` environment, see `*` in the previous subsection. You can launch it right from the current shell by pasting path to `devenv.exe` (Visual Studio executable, e.g. `"C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe"`). To launch it later with correct environment, you can create batch script (a file with `bat` extension) which you can double-click, here's an example (use your own path on the first line):

```batchfile
call "c:\Users\kamba\source\repos\vcmi\conan-msvc\conanrun.bat"
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\devenv.exe"
```

1. Open `%VCMI_DIR%/build/VCMI.sln` in Visual Studio
2. Select `RelWithDebInfo` build type in the combobox
3. If you want to use ccache:
    - Select `Manage Configurations...` in the combobox
    - Specify the following CMake variable: `ENABLE_CCACHE=ON`
    - See the [Visual Studio documentation](https://learn.microsoft.com/en-us/cpp/build/customize-cmake-settings?view=msvc-170#cmake-variables-and-cache) for details
4. Right click on `BUILD_ALL` project. This `BUILD_ALL` project should be in `CMakePredefinedTargets` tree in Solution Explorer. You can also build individual targets if you want.
5. VCMI will be built in `%VCMI_DIR%/build/bin/<config>` folder where `<config>` is e.g. `RelWithDebInfo`. To launch the built executables from a file manager, use respective `bat` files, e.g. `VCMI_launcher.bat`.

### Compile VCMI with MinGW64 or UCRT64 via MSYS2

1. Install MSYS2 from <https://www.msys2.org/>
2. Open the correct shell
   - For MinGW64 (MSVCRT): start `MSYS2 MinGW x64`
   - For UCRT64: start `MSYS2 UCRT64`

   (Sanity check: `echo $MSYSTEM` should be MINGW64 or UCRT64; don’t mix them.)
3. Update MSYS2 packages: `pacman -Syu`
4. Install dependencies
   - For MinGW64 (MSVCRT): `pacman -S mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-SDL2_mixer mingw-w64-x86_64-SDL2_ttf mingw-w64-x86_64-boost mingw-w64-x86_64-gcc mingw-w64-x86_64-ninja mingw-w64-x86_64-qt5-static mingw-w64-x86_64-qt5-tools mingw-w64-x86_64-tbb`
   - For UCRT64: `pacman -S --needed mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-boost mingw-w64-ucrt-x86_64-minizip mingw-w64-ucrt-x86_64-ffmpeg mingw-w64-ucrt-x86_64-SDL2_image mingw-w64-ucrt-x86_64-SDL2_mixer mingw-w64-ucrt-x86_64-SDL2_ttf mingw-w64-ucrt-x86_64-qt5-static mingw-w64-ucrt-x86_64-tbb`
5. Generate and build solution from VCMI-root dir: `cmake --preset windows-mingw-release && cmake --build --preset windows-mingw-release`

**NOTE:** This will link Qt5 statically to `VCMI_launcher.exe` and `VCMI_Mapeditor.exe`. See [PR #3421](https://github.com/vcmi/vcmi/pull/3421) for some background.

## Troubleshooting and workarounds

Vcpkg might be very unstable due to limited popularity and fact of using bleeding edge packages (such as most recent Boost). Using latest version of dependencies could also expose both problems in VCMI code or library interface changes that developers not checked yet. So if you're built Vcpkg yourself and can't get it working please try to use binary package.

Pre-built version we provide is always manually tested with all supported versions of MSVC for both Release and Debug builds and all known quirks are listed below.

### Build is successful but can not start new game

Make sure you have:

* Installed Heroes III from disk or using GOG installer
* Copied `Data`, `Maps` and `Mp3` folders from Heroes III to: `%USERPROFILE%\Documents\My Games\vcmi\`

### VCMI won't run since some library is missing

**If you open solution using vcmi.sln** Try to build INSTALL target and see if its output works as expected. Copy missing libraries or even all libraries from there to your build directory. Or run cpack and use produced installer and see if you can get libs from there. cpack -V will give more info. If cpack complains that it can not find dumpbin try Visual Studio Command Prompt (special version of cmd provided with Visual Studio with additional PATH) or modify PATH to have this tool available. Another alternative if you use prebuilt vcpkg package is to download latest msvc build, install it and copy missing/all libraries from there.

### Debug build is very slow

Debug builds with MSVC are generally extremely slow since it's not just VCMI binaries are built as debug, but every single dependency too and this usually means no optimizations at all. Debug information that available for release builds is often sufficient so just avoid full debug builds unless absolutely necessary. Instead use RelWithDebInfo configuration, optionally with Optimization Disabled (/Od) to avoid variables being optimized away. Also Debug configuration might have some compilation issues because it is not checked via CI for now.

### I got crash within library XYZ.dll

VCPKG generated projects quite often have both debug and regular libs available to linker so it can select wrong lib. For stable RelWithDebInfo build you may try to remove debug folder from VCPKG/installed/x64-windows. Same is done on CI. Also it reduces package size at least twice.

## Legacy instructions for Vcpkg package manager

We have switched to the Conan package manager in v1.7. Legacy Vcpkg integration is no longer supported and may or may not work out of the box.

You have two options: to use pre-built libraries or build your own. We strongly recommend start with using pre-built ones.

### Option A. Use pre-built Vcpkg

#### Download and unpack archive

Vcpkg Archives are available at our GitHub: <https://github.com/vcmi/vcmi-deps-windows/releases>

- Download latest version available.
EG: v1.6 assets - [vcpkg-export-x64-windows-v143.7z](https://github.com/vcmi/vcmi-deps-windows/releases/download/v1.6/vcpkg-export-x64-windows-v143.7z)
- Extract archive by right clicking on it and choosing "7-zip -> Extract Here".

#### Move dependencies to target directory

Once extracted, a `vcpkg` directory will appear with `installed` and `scripts` subfolders inside.
Move extracted `vcpkg` directory into your `%VCMI_DIR%`

### Option B. Build Vcpkg on your own

Please be aware that if you're running 32-bit Windows version, then this is impossible due to <https://github.com/microsoft/vcpkg/issues/26036>
Be aware that building Vcpkg might take a lot of time depend on your CPU model and 10-20GB of disk space.

#### Create initial directory

#### Clone vcpkg

1. open SourceTree
2. File -\> Clone
3. select **<https://github.com/microsoft/vcpkg/>** as source
4. select **%VCMI_DIR%/vcpkg** as destination
5. click **Clone**

From command line use:

```sh
git clone https://github.com/microsoft/vcpkg.git %VCMI_DIR%/vcpkg
```

#### Build vcpkg and dependencies

- Run
`%VCMI_DIR%/vcpkg/bootstrap-vcpkg.bat`
- For 32-bit build run:
`%VCMI_DIR%/vcpkg/vcpkg.exe install tbb:x64-windows fuzzylite:x64-windows sdl2:x64-windows sdl2-image:x64-windows sdl2-ttf:x64-windows sdl2-mixer[mpg123]:x64-windows boost:x64-windows qt5-base:x64-windows ffmpeg:x64-windows luajit:x64-windows`
- For 64-bit build run:
`%VCMI_DIR%/vcpkg/vcpkg.exe install install tbb:x86-windows fuzzylite:x86-windows sdl2:x86-windows sdl2-image:x86-windows sdl2-ttf:x86-windows sdl2-mixer[mpg123]:x86-windows boost:x86-windows qt5-base:x86-windows ffmpeg:x86-windows luajit:x86-windows`

For the list of the packages used you can also consult [vcmi-deps-windows readme](https://github.com/vcmi/vcmi-deps-windows) in case this article gets outdated a bit.

### CMake integration

When executing `cmake` configure step, pass `--toolchain %VCMI_DIR%/vcpkg/scripts/buildsystems/vcpkg.cmake`
