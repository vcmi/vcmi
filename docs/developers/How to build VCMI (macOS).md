# Requirements

1.  C++ toolchain, either of:
    -   Xcode Command Line Tools (aka CLT):
        `sudo xcode-select --install`
    -   Xcode IDE: <https://developer.apple.com/xcode/>
    -   (not tested) other C++ compilers, e.g. gcc/clang from
        [Homebrew](https://brew.sh/)
2.  CMake: `brew install --cask cmake` or get from
    <https://cmake.org/download/>
3.  (optional) Ninja: `brew install ninja` or get from
    <https://github.com/ninja-build/ninja/releases>

# Obtaining source code

Clone <https://github.com/vcmi/vcmi> with submodules. Example for
command line:

`git clone --recurse-submodules https://github.com/vcmi/vcmi.git`

# Obtaining dependencies

There're 2 ways to get dependencies automatically.

## Conan package manager

Please find detailed instructions in [VCMI
repository](https://github.com/vcmi/vcmi/tree/develop/docs/conan.md).
Note that the link points to the cutting-edge state in `develop` branch,
for the latest release check the same document in the [master
branch](https://github.com/vcmi/vcmi/tree/master/docs/conan.md).

On the step where you need to replace **PROFILE**, choose:

-   if you're on an Intel Mac: `macos-intel`
-   if you're on an Apple Silicon Mac: `macos-arm`

Note: if you wish to build 1.0 release in non-`Release` configuration,
you should define `USE_CONAN_WITH_ALL_CONFIGS=1` environment variable
when executing `conan install`.

## Homebrew

1.  [Install Homebrew](https://brew.sh/)
2.  Install dependencies:
    `brew install boost minizip sdl2 sdl2_image sdl2_mixer sdl2_ttf tbb`
3.  If you want to watch in-game videos, also install FFmpeg:
    `brew install ffmpeg@4`
4.  Install Qt dependency in either of the ways (note that you can skip
    this if you're not going to build Launcher):
    -   `brew install qt@5` for Qt 5 or `brew install qt` for Qt 6
    -   using [Qt Online Installer](https://www.qt.io/download) - choose
        **Go open source**

# Preparing build environment

This applies only to Xcode-based toolchain. If `xcrun -f clang` prints
errors, then use either of the following ways:

-   select an Xcode instance from Xcode application - Preferences -
    Locations - Command Line Tools
-   use `xcode-select` utility to set Xcode or Xcode Command Line Tools
    path: for example,
    `sudo xcode-select -s /Library/Developer/CommandLineTools`
-   set `DEVELOPER_DIR` environment variable pointing to Xcode or Xcode
    Command Line Tools path: for example,
    `export DEVELOPER_DIR=/Applications/Xcode.app`

# Configuring project for building

Note that if you wish to use Qt Creator IDE, you should skip this step
and configure respective variables inside the IDE.

1.  In Terminal `cd` to the source code directory
2.  Start assembling CMake invocation: type `cmake -S . -B BUILD_DIR`
    where *BUILD_DIR* can be any path, **don't press Return**
3.  Decide which CMake generator you want to use:
    -   Makefiles: no extra option needed or pass `-G 'Unix Makefiles'`
    -   Ninja (if you have installed it): pass `-G Ninja`
    -   Xcode IDE (if you have installed it): pass `-G Xcode`
4.  If you picked Makefiles or Ninja, pick desired *build type* - either
    of Debug / RelWithDebInfo / Release / MinSizeRel - and pass it in
    `CMAKE_BUILD_TYPE` option, for example:
    `-D CMAKE_BUILD_TYPE=Release`. If you don't pass this option,
    `RelWithDebInfo` will be used.
5.  If you don't want to build Launcher, pass `-D ENABLE_LAUNCHER=OFF`
6.  You can also pass `-Wno-dev` if you're not interested in CMake
    developer warnings
7.  Next step depends on the dependency manager you have picked:
    -   Conan: pass
        `-D CMAKE_TOOLCHAIN_FILE=conan-generated/conan_toolchain.cmake`
        where **conan-generated** must be replaced with your directory
        choice
    -   Homebrew: if you installed FFmpeg or Qt 5, you need to pass
        `-D "CMAKE_PREFIX_PATH="` variable. See below what you can
        insert after `=` (but **before the closing quote**), multiple
        values must be separated with `;` (semicolon):
        -   if you installed FFmpeg, insert `$(brew --prefix ffmpeg@4)`
        -   if you installed Qt 5 from Homebrew, insert:
            `$(brew --prefix qt@5)`
        -   if you installed Qt from Online Installer, insert your path
            to Qt directory, for example:
            `/Users/kambala/dev/Qt-libs/5.15.2/Clang64`
        -   example for FFmpeg + Qt 5:
            `-D "CMAKE_PREFIX_PATH=$(brew --prefix ffmpeg@4);$(brew --prefix qt@5)"`
8.  now press Return

# Building project

You must also install game files to be able to run the built version,
see [Installation on macOS](Installation_on_macOS "wikilink").

## From Xcode IDE

Open `VCMI.xcodeproj` from the build directory, select `vcmiclient`
scheme and hit Run (Cmd+R). To build Launcher, select `vcmilauncher`
scheme instead.

## From command line

`cmake --build `<path to build directory>

-   If using Makefiles generator, you'd want to utilize all your CPU
    cores by appending `-- -j$(sysctl -n hw.ncpu)` to the above
-   If using Xcode generator, you can also choose which configuration to
    build by appending `--config `<configuration name> to the above, for
    example: `--config Debug`

# Packaging project into DMG file

After building, run `cpack` from the build directory. If using Xcode
generator, also pass `-C `<configuration name> with the same
configuration that you used to build the project.

If you use Conan, it's expected that you use **conan-generated**
directory at step 4 of [#Conan package
manager](#Conan_package_manager "wikilink").

# Running VCMI

You can run VCMI from DMG, but it's will also work from your IDE be it
Xcode or Qt Creator.

Alternatively you can run binaries directly from "bin" directory:

    BUILD_DIR/bin/vcmilauncher
    BUILD_DIR/bin/vcmiclient
    BUILD_DIR/bin/vcmiserver

CMake include commands to copy all needed assets from source directory
into "bin" on each build. They'll work when you build from Xcode too.

# Some useful debugging tips

Anyone who might want to debug builds, but new to macOS could find
following commands useful:

    # To attach DMG file from command line use
    hdiutil attach vcmi-1.0.dmg
    # Detach volume:
    hdiutil detach /Volumes/vcmi-1.0
    # To view dependency paths
    otool -L /Volumes/vcmi-1.0/VCMI.app/Contents/MacOS/vcmiclient
    # To display load commands such as LC_RPATH 
    otool -l /Volumes/vcmi-1.0/VCMI.app/Contents/MacOS/vcmiclient

# Troubleshooting

In case of troubles you can always consult our CI build scripts or
contact the dev team via slack