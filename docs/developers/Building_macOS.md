# Building VCMI for macOS

## Requirements

1. C++ toolchain, either of:
    - Xcode Command Line Tools (aka CLT): `sudo xcode-select --install`
    - Xcode IDE: <https://developer.apple.com/xcode/>
    - (not tested) other C++ compilers, e.g. gcc/clang from [Homebrew](https://brew.sh/)
2. CMake: `brew install --cask cmake` or get from <https://cmake.org/download/>
4. Optional:
    * Ninja: `brew install ninja` or get it from <https://github.com/ninja-build/ninja/releases>
    * CCache to speed up recompilation: `brew install ccache`

## Obtaining source code

Clone <https://github.com/vcmi/vcmi> with submodules. Example for command line:

```sh
git clone --recurse-submodules https://github.com/vcmi/vcmi.git
```

## Obtaining dependencies

There're 2 ways to get dependencies automatically.

### Conan package manager (recommended)

We use this to produce builds on CI.

Please find detailed instructions [here](./Conan.md). Note that the link points to the state of the current branch, for the latest release check the same document in the [master branch](https://github.com/vcmi/vcmi/blob/master/docs/developers/Conan.md).

On the step where you need to replace **PROFILE**, choose:

- if you're on an Intel Mac: `macos-intel`
- if you're on an Apple Silicon Mac: `macos-arm`

### Homebrew

1. [Install Homebrew](https://brew.sh/)
2. Install dependencies: `brew install boost minizip sdl2 sdl2_image sdl2_mixer sdl2_ttf tbb`
    - you can also use minizip-ng instead of minizip
3. If you want to watch in-game videos, also install FFmpeg: `brew install ffmpeg` (you can also use an earlier FFmpeg version)
4. Install Qt dependency in either of the ways (note that you can skip this if you're not going to build Launcher and Map editor):
    - `brew install qt@5` for Qt 5 or `brew install qt` for Qt 6
    - using [Qt Online Installer](https://www.qt.io/download-qt-installer-oss)

## Preparing build environment

This applies only to Xcode-based toolchain. If `xcrun -f clang` prints errors, then use either of the following ways:

- select an Xcode instance from Xcode application - Preferences - Locations - Command Line Tools
- use `xcode-select` utility to set Xcode or Xcode Command Line Tools path, example: `sudo xcode-select -s /Library/Developer/CommandLineTools`
- set `DEVELOPER_DIR` environment variable pointing to Xcode or Xcode Command Line Tools path, example: `export DEVELOPER_DIR=/Applications/Xcode.app`

## Configuring project for building

Note that if you wish to use Qt Creator or CLion IDE, you should skip this step and configure respective variables inside the IDE. Or you could create a [CMake preset](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) to avoid manual configuration.

The following walkthrough lists only the bare minimum of required CMake options.

1. In Terminal `cd` to the source code directory
2. Start assembling CMake invocation: type `cmake -S . -B BUILD_DIR` where *BUILD_DIR* can be any path, **don't press Return**
3. Decide which CMake generator you want to use:
    - Xcode IDE (if you have installed it): pass `-G Xcode`
    - Ninja (if you have installed it): pass `-G Ninja`
    - Makefiles: no extra option needed or pass `-G 'Unix Makefiles'`
4. If you picked Makefiles or Ninja, pick desired *build type* - either of `Debug` / `RelWithDebInfo` / `Release` / `MinSizeRel` - and pass it in `CMAKE_BUILD_TYPE` option, example: `-D CMAKE_BUILD_TYPE=Debug`. If you use don't pass this option, `RelWithDebInfo` will be used.
5. Next step depends on the dependency manager you have picked:
    - Conan: pass `--toolchain conan-generated/conan_toolchain.cmake` (or via `CMAKE_TOOLCHAIN_FILE` variable) where **conan-generated** must be replaced with your directory choice
    - Homebrew: if you installed Qt 5 or from the Online Installer, you need to pass `-D "CMAKE_PREFIX_PATH="` variable. See below what you can insert after `=` (but **before the closing quote**), multiple values must be separated with `;` (semicolon):
        - if you installed Qt 5 from Homebrew, insert `$(brew --prefix qt@5)`
        - if you installed Qt from Online Installer, insert your path to Qt directory, example: `/Users/kambala/dev/Qt-libs/5.15.2/Clang64`
6. Now press Return

## Building project

You must also install game files to be able to run the built version, see [Installation on macOS](../players/Installation_macOS.md).

### From Xcode IDE

Open `VCMI.xcodeproj` from the build directory, select `vcmiclient` scheme and hit Run (Cmd+R). To build Launcher, select `vcmilauncher` scheme instead.

### From command line

`cmake --build <path to build directory>`

- If using Makefiles generator, you'd want to utilize all your CPU cores by appending `-- -j$(sysctl -n hw.ncpu)` to the above
- If using Xcode generator, you can also choose which configuration to build by appending `--config <configuration name>` to the above, example: `--config Debug`

## Running VCMI

You can run binaries from your IDE or directly from the **bin** directory:

- BUILD_DIR/bin/vcmilauncher
- BUILD_DIR/bin/vcmiclient
- BUILD_DIR/bin/vcmiserver
- BUILD_DIR/bin/vcmimapeditor
