# Conan Dependencies

[Conan](https://conan.io/) is a package manager for C/C++. We provide prebuilt binary dependencies for some platforms that are used by our CI, but they can also be consumed by users to build VCMI. However, it's not required to use only the prebuilt binaries: you can build them from source as well.

## Supported platforms

The following platforms are supported and known to work, others might require changes to our [conanfile.py](../../conanfile.py) or upstream recipes.

- **macOS**:
    - x86_64 (Intel) - target 10.13 (High Sierra)
    - arm64 (Apple Silicon) - target 11.0 (Big Sur)
- **iOS**: arm64 - target 12.0
- **Windows** using MSVC compiler:
    - x86_64 (x64) - target Windows 7
    - x86 - target Windows 7
    - arm64 - target Windows 11
- **Android**:
    - arvm7 / armeabi-v7a (32-bit ARM) - target 4.4 (API level 19)
    - arm64 / aarch64-v8a (64-bit ARM) - target 5.0 (API level 21)
    - x86_64 (64-bit Intel) - target 5.0 (API level 21)

## Getting started

1. [Install Conan](https://docs.conan.io/2/installation.html). For example: `pip3 install conan`
2. Execute in terminal: `conan profile detect`. It will create _build profile_ for your machine.

## Download dependencies

0. If your platform is not on the list of supported ones or you don't want to use our prebuilt binaries, you can still build dependencies from source or try consuming prebuilt binaries from the central Conan repository - [ConanCenter](https://conan.io/center/). In this case skip to the [next section](#generate-cmake-integration) directly.

1. Check if your build environment can use the prebuilt binaries: basically, that your compiler version (or Xcode major version) matches the information below. If you're unsure, simply advance to the next step.

    - **macOS**: libraries are built with Apple clang 16 (Xcode 16.2), should be consumable by Xcode / Xcode CLT 14.x and later
    - **iOS**: libraries are built with Apple clang 16 (Xcode 16.2), should be consumable by Xcode 14.x and later
    - **Windows**: libraries are built with MSVC 19.4x (v143 toolset)
    - **Android**: libraries are built with NDK r25c (25.2.9519653)

2. Download the binaries archive from <https://github.com/vcmi/vcmi-dependencies/releases> (pre-release is for development version and the latest release is for respective VCMI release) and use `conan cache restore <path to archive>` command to unpack them.

    - macOS: pick **dependencies-mac-intel.tgz** if you have Intel Mac, otherwise - **dependencies-mac-arm.tgz**
    - iOS: pick **dependencies-ios.tgz**
    - Windows: pick **dependencies-windows-x64.tgz** for Windows x64, **dependencies-windows-arm64.tgz** for Windows ARM64 or **dependencies-windows-x86.tgz** for Windows x86
    - Android: pick **dependencies-android-arm64-v8a.tgz** for arm64 (ARM 64-bit), **dependencies-android-armeabi-v7a.tgz** for armv7 (ARM 32-bit) or **dependencies-android-x64.tgz** for x86_64 (Intel 64-bit). Current limitation is that building works only on a Linux machine out of the box due to Qt 5 for Android being compiled on CI running Ubuntu Linux. But there's an easy workaround for Windows and macOS, see below. TODO

### Platform-specific preparation

Follow this subsection only if any of the following applies to you, otherwise skip to the [next section](#generate-cmake-integration) directly.

- you're trying to build for Android and your OS is Windows or macOS or Linux aarch64
- you're trying to build for iOS and you have Intel Mac

Qt 5 has some utilities required for the build process (moc, uic etc.) that are built for your desktop OS. Our CI (GitHub Actions) makes Android builds on Ubuntu Linux amd64 and iOS builds on an Apple Silicon Mac, therefore those Qt utilities are built for Linux amd64 and macOS arm64 respectively and can't be used/run on other OS. To solve that, you must add/replace those utilities built for your desktop OS.

The easiest way would be downloading prebuilt dependencies for your platform (Windows / macOS), unpacking the archive (no need to use `conan cache restore`, unpack as an ordinary archive) and copying executable files from `<unpacked dir>/b/qt<some hash>/p/bin` directory. We don't provide prebuilts for Linux - simply install Qt development package from your package manager and copy executables from its `bin` directory.

## Generate CMake integration

Conan needs to generate CMake toolchain file to make dependencies available to CMake. See `CMakeDeps` and `CMakeToolchain` [in the official documentation](https://docs.conan.io/2/reference/tools/cmake.html) for details.

Make sure that you've cloned VCMI repository with submodules! (or initialized them afterwards)

In terminal `cd` to the VCMI source directory and run the following command (it's written in Bash syntax, for other shells like Cmd or Powershell use appropriate line continuation character instead of `\`). Also check subsections for additional requirements on consuming prebuilt binaries.

*Note*: if you're going to build for Windows MSVC, it's recommended to use Cmd shell. If you absolutely want to use Powershell, then run the below command twice appending `-c tools.env.virtualenv:powershell=powershell.exe` on the second run.

<pre>
conan install . \
  --output-folder=<b><i>conan-generated</i></b> \
  --build=<b><i>never</i></b> \
  --profile=<b><i>dependencies/conan_profiles/PROFILE</i></b> \
  <b><i>EXTRA PARAMS</i></b>
</pre>

The highlighted parts can be adjusted:

- ***conan-generated***: directory (absolute or relative) where the generated files will appear. This value is used in CMake presets from VCMI, but you can actually use any directory and override it in your local CMake presets.
- ***never***: use this value to avoid building any dependency from source. You can also use `missing` to build binary packages, that are not present in your local cache, from source. There're also other values, see `conan install -h` or the full documentation linked below.
- ***dependencies/conan_profiles/PROFILE***: if you want to consume our prebuilt binaries, ***PROFILE*** must be replaced with one of filenames from our [Conan profiles directory](../../dependencies/conan_profiles) (determining the right file should be straight-forward). Otherwise, either select one of our profiles or replace ***CI/conan/PROFILE*** with `default` (your default profile, will build for your desktop OS) or create your own profile for the desired platform.
- ***EXTRA PARAMS***: additional params to the `conan install` command, you can specify multiple:
    - if you want to consume our prebuilt binaries for Apple platforms (macOS / iOS), pass `--profile=dependencies/conan_profiles/base/apple-system`
    - if you want to consume our prebuilt binaries for Android, pass `--profile=dependencies/conan_profiles/base/android-system`
    - if your intention is to make a Debug build, pass `-s "&:build_type=RelWithDebInfo"` for Windows MSVC and `-s "&:build_type=Debug"` for other platforms
    - if you're building on Windows 11 ARM64, pass `-o "&:lua_lib=lua"`
    - if you're building on (or for) Windows < 10, pass `-o "&:target_pre_windows10=True"`

If you use `--build=never` and this command fails, then it means that you can't use prebuilt binaries out of the box. For example, try using `--build=missing` instead.

VCMI "recipe" also has some options that you can specify. For example, if you don't care about game videos, you can disable FFmpeg dependency by passing `-o with_ffmpeg=False`. Check [the recipe](../../dependencies/conanfile.py) for details.

*Note*: you can find full reference of this command [in the official documentation](https://docs.conan.io/2/reference/commands/install.html) or by executing `conan help install`.

### Using prebuilt binaries from ConanCenter

First, check if binaries for [your platform](https://github.com/conan-io/conan-center-index/blob/master/docs/supported_platforms_and_configurations.md) are produced.

You must adjust the above `conan install` command by replacing ***dependencies/conan_profiles/PROFILE*** with `default`.

### Building dependencies from source

This subsection describes platform specifics to build libraries from source properly.

#### Building for macOS/iOS

- To build Locale module of Boost in versions >= 1.81, you must use `compiler.cppstd=11` Conan setting (our profiles already contain it). To use it with another profile, either add this setting to your *host* profile or pass `-s compiler.cppstd=11` on the command line.
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

*Note*: if you plan to build Qt from source again, then you don't need to perform the above *After applying patch(es)* steps after building.

##### Using recipes for system libraries

1. Clone/download <https://github.com/kambala-decapitator/conan-system-libs>
2. Execute `conan create PACKAGE vcmi/CHANNEL`, where `PACKAGE` is a directory path in that repository and `CHANNEL` is **apple** for macOS/iOS and **android** for Android. Do it for each library you need.
3. Now you can execute `conan install` to build all dependencies.

## Configure project for building

You must pass the generated toolchain file to CMake invocation.

- if using custom CMake presets, just make sure to inherit our `build-with-conan` preset. If you store Conan generated files in a non-default directory, define the path to the generated toolchain in `toolchainFile` field (or `CMAKE_TOOLCHAIN_FILE` cache variable) or include CMake presets file generated by Conan.
- otherwise, if passing CMake options on the command line, use `--toolchain` option (available in CMake 3.21+) or `CMAKE_TOOLCHAIN_FILE` variable.

## Examples

In these examples only the minimum required amount of options is passed to `cmake` invocation, you can pass additional ones as needed.

### Use our prebuilt binaries to build for macOS x86_64 with Xcode

```sh
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

```sh
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

```sh
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

### 
