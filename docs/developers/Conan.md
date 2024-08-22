# Conan Dependencies

[Conan](https://conan.io/) is a package manager for C/C++. We provide prebuilt binary dependencies for some platforms that are used by our CI, but they can also be consumed by users to build VCMI. However, it's not required to use only the prebuilt binaries: you can build them from source as well.

## Supported platforms

The following platforms are supported and known to work, others might require changes to our [conanfile.py](../../conanfile.py) or upstream recipes.

- **macOS**: x86_64 (Intel) - target 10.13 (High Sierra), arm64 (Apple Silicon) - target 11.0 (Big Sur)
- **iOS**: arm64 - target 12.0
- **Windows**: x86_64 and x86 fully supported with building from Linux
- **Android**: armeabi-v7a (32-bit ARM) - target 4.4 (API level 19), aarch64-v8a (64-bit ARM) - target 5.0 (API level 21)

## Getting started

1. [Install Conan](https://docs.conan.io/1/installation.html). Currently we support only Conan v1, you can install it with `pip` like this: `pip3 install 'conan<2.0'`
2. Execute in terminal: `conan profile new default --detect`

## Download dependencies

0. If your platform is not on the list of supported ones or you don't want to use our prebuilt binaries, you can still build dependencies from source or try consuming prebuilt binaries from the central Conan repository - [ConanCenter](https://conan.io/center/). In this case skip to the [next section](#generate-cmake-integration) directly.

1. Check if your build environment can use the prebuilt binaries: basically, that your compiler version (or Xcode major version) matches the information below. If you're unsure, simply advance to the next step.

    - **macOS**: libraries are built with Apple clang 14 (Xcode 14.2), should be consumable by Xcode / Xcode CLT 14.x and later (older library versions are also available for Xcode 13, see Releases in the respective repo)
    - **iOS**: libraries are built with Apple clang 14 (Xcode 14.2), should be consumable by Xcode 14.x and later (older library versions are also available for Xcode 13, see Releases in the respective repo)
    - **Windows**: libraries are built with x86_64-mingw-w64-gcc version 10 (which is available in repositories of Ubuntu 22.04)
    - **Android**: libraries are built with NDK r25c (25.2.9519653)

2. Download the binaries archive and unpack it to `~/.conan` directory from https://github.com/vcmi/vcmi-dependencies/releases/latest

    - macOS: pick **dependencies-mac-intel.txz** if you have Intel Mac, otherwise - **dependencies-mac-arm.txz**
    - iOS: pick ***dependencies-ios.txz***
    - Windows: currently only mingw is supported. Pick **dependencies-mingw.tgz** if you want x86_64, otherwise pick **dependencies-mingw-32.tgz**
    - Android: current limitation is that building works only on a macOS host due to Qt 5 for Android being compiled on macOS. Simply delete directory `~/.conan/data/qt/5.15.x/_/_/package` (`5.15.x` is a placeholder) after unpacking the archive and build Qt from source. Alternatively, if you have (or are [willing to build](https://github.com/vcmi/vcmi-ios-deps#note-for-arm-macs)) Qt host tools for your platform, then simply replace those in the archive with yours and most probably it would work.

3. Only if you have Apple Silicon Mac and trying to build for macOS or iOS:

    1. Open file `~/.conan/data/qt/5.15.x/_/_/export/conanfile.py` (`5.15.x` is a placeholder), search for string `Designer` (there should be only one match), comment this line and the one above it by inserting `#` in the beginning, and save the file.
    2. (optional) If you don't want to use Rosetta, follow [instructions how to build Qt host tools for Apple Silicon](https://github.com/vcmi/vcmi-ios-deps#note-for-arm-macs), on step 3 copy them to `~/.conan/data/qt/5.15.x/_/_/package/SOME_HASH/bin` (`5.15.x` and `SOME_HASH` are placeholders). Make sure **not** to copy `qt.conf`!

## Generate CMake integration

Conan needs to generate CMake toolchain file to make dependencies available to CMake. See `CMakeDeps` and `CMakeToolchain` [in the official documentation](https://docs.conan.io/1/reference/conanfile/tools/cmake.html) for details.

In terminal `cd` to the VCMI source directory and run the following command. Also check subsections for additional requirements on consuming prebuilt binaries.

<pre>
conan install . \
  --install-folder=<b><i>conan-generated</i></b> \
  --no-imports \
  --build=<b><i>never</i></b> \
  --profile:build=default \
  --profile:host=<b><i>CI/conan/PROFILE</i></b>
</pre>

The highlighted parts can be adjusted:

- ***conan-generated***: directory (absolute or relative) where the generated files will appear. This value is used in CMake presets from VCMI, but you can actually use any directory and override it in your local CMake presets.
- ***never***: use this value to avoid building any dependency from source. You can also use `missing` to build recipes, that are not present in your local cache, from source.
- ***CI/conan/PROFILE***: if you want to consume our prebuilt binaries, ***PROFILE*** must be replaced with one of filenames from our [Conan profiles directory](../../CI/conan) (determining the right file should be straight-forward). Otherwise, either select one of our profiles or replace ***CI/conan/PROFILE*** with `default` (your default profile).
- ***note for Windows x86***: use profile mingw32-linux.jinja for building instead of mingw64-linux.jinja

If you use `--build=never` and this command fails, then it means that you can't use prebuilt binaries out of the box. For example, try using `--build=missing` instead.

VCMI "recipe" also has some options that you can specify. For example, if you don't care about game videos, you can disable FFmpeg dependency by passing `-o with_ffmpeg=False`. If you only want to make release build, you can use `GENERATE_ONLY_BUILT_CONFIG=1` environment variable to skip generating files for other configurations (our CI does this).

_Note_: you can find full reference of this command [in the official documentation](https://docs.conan.io/1/reference/commands/consumer/install.html) or by executing `conan help install`.

### Using our prebuilt binaries for macOS/iOS

We use custom recipes for some libraries that are provided by the OS. You must additionally pass `-o with_apple_system_libs=True` for `conan install` to use them.

### Using prebuilt binaries from ConanCenter

First, check if binaries for [your platform](https://github.com/conan-io/conan-center-index/blob/master/docs/supported_platforms_and_configurations.md) are produced.

You must adjust the above `conan install` command:

1. Replace ***CI/conan/PROFILE*** with `default`.
2. Additionally pass `-o default_options_of_requirements=True`: this disables all custom options of our `conanfile.py` to match ConanCenter builds.

### Building dependencies from source

This subsection describes platform specifics to build libraries from source properly.

#### Building for macOS/iOS

- To build Locale module of Boost in versions >= 1.81, you must use `compiler.cppstd=11` Conan setting (our profiles already contain it). To use it with another profile, either add this setting to your _host_ profile or pass `-s compiler.cppstd=11` on the command line.
- If you wish to build dependencies against system libraries (like our prebuilt ones do), follow [below instructions](#using-recipes-for-system-libraries) executing `conan create` for all directories. Don't forget to pass `-o with_apple_system_libs=True` to `conan install` afterwards.

#### Building for Android

Android has issues loading self-built shared zlib library because binary name is identical to the system one, so we enforce using the OS-provided library. To achieve that, follow [below instructions](#using-recipes-for-system-libraries), you only need `zlib` directory.

Also, Android requires a few Qt patches. They are needed only for specific use cases, so you may evaluate whether you need them. The patches can be found [here](https://github.com/kambala-decapitator/Qt5-iOS-patches/tree/master/5.15.14/android). To apply selected patch(es), let Conan finish building dependencies first. Then `cd` to `qtbase` **source directory** (e.g. `~/.conan/data/qt/5.15.14/_/_/source/qt5/qtbase`) and run `patch -p1 < /path/to/patch` for each patch file (on Windows you'll need some sort of GNU environment like Git Bash to access `patch` utility).

1. [Safety measure for Xiaomi devices](https://github.com/kambala-decapitator/Qt5-iOS-patches/blob/master/5.15.14/android/xiaomi.diff). It's unclear whether it's really needed, but without it [I faced a crash once](https://bugreports.qt.io/browse/QTBUG-111960).
2. [Fix running on Android 5.0-5.1](https://github.com/kambala-decapitator/Qt5-iOS-patches/blob/master/5.15.14/android/android-21-22.diff) (API level 21-22).
3. [Enable running on Android 4.4](https://github.com/kambala-decapitator/Qt5-iOS-patches/blob/master/5.15.14/android/android-19-jar.diff) (API level 19-20, 32-bit only). You must also apply [one more patch](https://github.com/kambala-decapitator/Qt5-iOS-patches/blob/master/5.15.14/android/android-19-java.diff), but you must `cd` to the **package directory** for that, e.g. `~/.conan/data/qt/5.15.14/_/_/package/SOME_HASH`.

After applying patch(es):

1. `cd` to `qtbase/src/android/jar` in the **build directory**, e.g. `~/.conan/data/qt/5.15.14/_/_/build/SOME_HASH/build_folder/qtbase/src/android/jar`.
2. Run `make`
3. Copy file `qtbase/jar/QtAndroid.jar` from the build directory to the **package directory**, e.g. `~/.conan/data/qt/5.15.14/_/_/package/SOME_HASH/jar`.

_Note_: if you plan to build Qt from source again, then you don't need to perform the above _After applying patch(es)_ steps after building.

##### Using recipes for system libraries

1. Clone/download https://github.com/kambala-decapitator/conan-system-libs
2. Execute `conan create PACKAGE vcmi/CHANNEL`, where `PACKAGE` is a directory path in that repository and `CHANNEL` is **apple** for macOS/iOS and **android** for Android. Do it for each library you need.
3. Now you can execute `conan install` to build all dependencies.

## Configure project for building

You must pass the generated toolchain file to CMake invocation.

- if using custom CMake presets, just make sure to inherit our `build-with-conan` preset. If you store Conan generated files in a non-default directory, define the path to the generated toolchain in `toolchainFile` field (or `CMAKE_TOOLCHAIN_FILE` cache variable) or include CMake presets file generated by Conan.
- otherwise, if passing CMake options on the command line, use `--toolchain` option (available in CMake 3.21+) or `CMAKE_TOOLCHAIN_FILE` variable.

## Examples

In these examples only the minimum required amount of options is passed to `cmake` invocation, you can pass additional ones as needed.

### Use our prebuilt binaries to build for macOS x86_64 with Xcode

```
conan install . \
  --install-folder=conan-generated \
  --no-imports \
  --build=never \
  --profile:build=default \
  --profile:host=CI/conan/macos-intel \
  -o with_apple_system_libs=True

cmake -S . -B build -G Xcode \
  --toolchain conan-generated/conan_toolchain.cmake
```

### Try to use binaries from ConanCenter for your platform

If you also want to build the missing binaries from source, use `--build=missing` instead of `--build=never`.

```
conan install . \
  --install-folder=~/my-dir \
  --no-imports \
  --build=never \
  --profile:build=default \
  --profile:host=default \
  -o default_options_of_requirements=True

cmake -S . -B build \
  -D CMAKE_TOOLCHAIN_FILE=~/my-dir/conan_toolchain.cmake
```

### Use our prebuilt binaries to build for iOS arm64 device with custom preset

```
conan install . \
  --install-folder=~/my-dir \
  --no-imports \
  --build=never \
  --profile:build=default \
  --profile:host=CI/conan/ios-arm64 \
  -o with_apple_system_libs=True

cmake --preset ios-conan
```

`CMakeUserPresets.json` file:

```json
{
    "version": 3,
    "cmakeMinimumRequired": {
        "major": 3,
        "minor": 21,
        "patch": 0
    },
    "configurePresets": [
        {
            "name": "ios-conan",
            "displayName": "iOS",
            "inherits": ["build-with-conan", "ios-device"],
            "toolchainFile": "~/my-dir/conan_toolchain.cmake",
            "cacheVariables": {
                "BUNDLE_IDENTIFIER_PREFIX": "com.YOUR-NAME",
                "CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM": "YOUR_TEAM_ID"
            }
        }
    ]
}
```

### Build VCMI with all deps for 32-bit windows in Ubuntu 22.04 WSL

```powershell
wsl --install
wsl --install -d Ubuntu
ubuntu
```

Next steps are identical both in WSL and in real Ubuntu 22.04

```bash
sudo pip3 install conan
sudo apt install cmake build-essential
sed -i 's/x86_64-w64-mingw32/i686-w64-mingw32/g' CI/mingw-ubuntu/before-install.sh
sed -i 's/x86-64/i686/g' CI/mingw-ubuntu/before-install.sh
sudo ./CI/mingw-ubuntu/before-install.sh
conan install . \
  --install-folder=conan-generated \
  --no-imports \
  --build=missing \
  --profile:build=default \
  --profile:host=CI/conan/mingw32-linux \
  -c tools.cmake.cmaketoolchain.presets:max_schema_version=2
cmake --preset windows-mingw-conan-linux
cmake --build --preset windows-mingw-conan-linux --target package
```

After that, you will have functional VCMI installer for 32-bit windows.
