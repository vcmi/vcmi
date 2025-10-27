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
2. Execute in terminal: `conan profile detect`. It will create *build profile* for your machine.

## Download dependencies

0. If your platform is not on the list of supported ones or you don't want to use our prebuilt binaries, you can still build dependencies from source or try consuming prebuilt binaries from the central Conan repository - [ConanCenter](https://conan.io/center/). In this case skip to the [next section](#generate-cmake-integration) directly.

1. Check if your build environment can use the prebuilt binaries: basically, that your compiler version (or Xcode major version) matches the information below. If you're unsure, simply advance to the next step.
    - *macOS*: libraries are built with Apple clang 16 (Xcode 16.2), should be consumable by Xcode / Xcode CLT 16.x and later
    - *iOS*: libraries are built with Apple clang 16 (Xcode 16.2), should be consumable by Xcode 16.x and later
    - *Windows*: libraries are built with MSVC 19.29 (v142 toolset, but can be consumed by v143) for Intel and with MSVC 19.4x (v143 toolset) for ARM64
    - *Android*: libraries are built with NDK r25c (25.2.9519653)

2. Download the binaries archive from <https://github.com/vcmi/vcmi-dependencies/releases> (pre-release is for development version and the latest release is for respective VCMI release) and use `conan cache restore <path to archive>` command to unpack them.
    - *macOS*: pick **dependencies-mac-intel.tgz** if you have Intel Mac, otherwise - **dependencies-mac-arm.tgz**
    - *iOS*: pick **dependencies-ios.tgz**
    - *Windows*: pick **dependencies-windows-x64.tgz** for Windows x64, **dependencies-windows-arm64.tgz** for Windows ARM64 or **dependencies-windows-x86.tgz** for Windows x86
    - *Android*: pick **dependencies-android-arm64-v8a.tgz** for arm64 (ARM 64-bit), **dependencies-android-armeabi-v7a.tgz** for armv7 (ARM 32-bit) or **dependencies-android-x64.tgz** for x86_64 (Intel 64-bit)

### Platform-specific preparation

Follow this subsection only if any of the following applies to you, otherwise skip to the [next section](#generate-cmake-integration) directly.

- you're trying to build for Android and your OS is Windows or macOS or Linux aarch64
- you're trying to build for iOS and you have Intel Mac

Qt 5 has some utilities required for the build process (moc, uic etc.) that are built for your desktop OS. Our CI (GitHub Actions) makes Android builds on Ubuntu Linux amd64 and iOS builds on an Apple Silicon Mac, therefore those Qt utilities are built for Linux amd64 and macOS arm64 respectively and can't be used/run on other OS / CPU architectures. To solve that, you must add/replace those utilities built for your desktop OS.

Once you have the executable files of those utilities, copy them to the `bin` directory of Qt package. You can find the package at `~/.conan2/p/qt<some hash>/p`.

#### Option 1

The easiest way would be downloading prebuilt dependencies for your platform (Windows / macOS), unpacking the archive (using `conan cache restore` isn't required, unpack as an ordinary archive) and copying (or making a hard or soft link) executable files from `<unpacked dir>/b/qt<some hash>/p/bin` directory. We don't provide prebuilts for Linux - simply install Qt development package from your package manager and copy executables from its `bin` directory. But you can also build all the executables manually, see [below](#option-2).

However, Qt for Android requires one more executable that can't be found in your desktop Qt build - `androiddeployqt`. And you'll have to build it manually, see next subsection.

#### Option 2

Building all those executables is rather fast as it doesn't require building whole Qt. This is how you do it in a Bash-like shell (this is not a ready script that can be run as is, read it and adjust to your platform / needs):

```sh
# set Qt version that you're going to download and build
qtVer=5.15.16

# for Android:
# ensure that ANDROID_HOME environment variable is set pointing to Android SDK directory
# also set Min SDK and NDK versions
minSdkVersion=21
ndkVersion=25.2.9519653

qtSrcDir="qt-everywhere-src-$qtVer"
qtInstallDir="$(pwd)/install"

# download Qt sources, unpack only the required subset of them
# use URL from a mirror if needed
curl -L "https://download.qt.io/official_releases/qt/5.15/$qtVer/single/qt-everywhere-opensource-src-$qtVer.tar.xz" \
  | tar -xf - --xz "$qtSrcDir"/{'.*','LICENSE*','configure*',qt.pro,'qtbase/*','qttools/*'}

# create build directory
mkdir build
cd build

# configure Qt for building, also pass to the following command:
# for Android: -shared -xplatform android-clang -android-sdk "$ANDROID_HOME" -android-ndk "$ANDROID_HOME/ndk/$ndkVersion" -android-ndk-platform android-$minSdkVersion -android-abis arm64-v8a
# for iOS: -static -xplatform macx-ios-clang -no-framework
"../$qtSrcDir/configure" -prefix "$qtInstallDir" -opensource -confirm-license -release -strip -appstore-compliant -make libs -no-compile-examples -no-dbus -system-zlib -no-openssl -opengl es2 -no-gif -no-ico -nomake examples -skip qt3d -skip qtactiveqt -skip qtandroidextras -skip qtcharts -skip qtconnectivity -skip qtdatavis3d -skip qtdeclarative -skip qtdoc -skip qtgamepad -skip qtgraphicaleffects -skip qtimageformats -skip qtlocation -skip qtlottie -skip qtmacextras -skip qtmultimedia -skip qtnetworkauth -skip qtpurchasing -skip qtquick3d -skip qtquickcontrols -skip qtquickcontrols2 -skip qtquicktimeline -skip qtremoteobjects -skip qtscript -skip qtscxml -skip qtsensors -skip qtserialbus -skip qtserialport -skip qtspeech -skip qtsvg -skip qttranslations -skip qtvirtualkeyboard -skip qtwayland -skip qtwebchannel -skip qtwebengine -skip qtwebglplugin -skip qtwebsockets -skip qtwebview -skip qtwinextras -skip qtx11extras -skip qtxmlpatterns

make --jobs=$(getconf _NPROCESSORS_ONLN) module-qtbase-qmake_all module-qttools-qmake_all

# if you need to build just androiddeployqt, on the second line pass only: sub-androiddeployqt
make --directory=qtbase/src --jobs=$(getconf _NPROCESSORS_ONLN) \
  sub-bootstrap sub-moc sub-qlalr sub-qvkgen sub-rcc sub-tracegen sub-uic sub-androiddeployqt

make --directory=qttools/src/linguist --jobs=$(getconf _NPROCESSORS_ONLN) \
  sub-lconvert sub-lprodump sub-lrelease sub-lrelease-pro sub-lupdate sub-lupdate-pro
```

After that you'll find all the executables in `build/qtbase/bin` and `build/qttools/bin` directories.

#### Option 3

Simply build whole Qt from source.

1. Remove Qt binary package: `conan remove "qt/5.15.*"`
2. In the next section use `--build=missing`

## Generate CMake integration

Conan needs to generate CMake toolchain file to make dependencies available to CMake. See `CMakeDeps` and `CMakeToolchain` [in the official documentation](https://docs.conan.io/2/reference/tools/cmake.html) for details.

Make sure that you've cloned VCMI repository with submodules! (or initialized them afterwards)

In terminal `cd` to the VCMI source directory and run the following command (it's written in Bash syntax, for other shells like Cmd or Powershell use appropriate line continuation character instead of `\` or type everything on a single line). Also check subsections for additional requirements on consuming prebuilt binaries.

*Note*: if you're going to build for Windows MSVC, it's recommended to use Cmd shell. If you absolutely want to use Powershell, then append `-c tools.env.virtualenv:powershell=powershell.exe` to the below command.

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
- ***dependencies/conan_profiles/PROFILE***: if you want to consume our prebuilt binaries, ***PROFILE*** must be replaced with one of filenames from our [Conan profiles directory](https://github.com/vcmi/vcmi-dependencies/tree/main/conan_profiles) (determining the right file should be straight-forward). Otherwise, either select one of our profiles or replace ***CI/conan/PROFILE*** with `default` (your default profile, will build for your desktop OS) or create your own profile for the desired platform.
- ***EXTRA PARAMS***: additional params to the `conan install` command, you can specify multiple:

  - if you want to consume our prebuilt binaries for Apple platforms (macOS / iOS), pass `--profile=dependencies/conan_profiles/base/apple-system`
  - if you want to consume our prebuilt binaries for Android, pass `--profile=dependencies/conan_profiles/base/android-system`
  - if your intention is to make a Debug build, pass `-s "&:build_type=RelWithDebInfo"` for Windows MSVC and `-s "&:build_type=Debug"` for other platforms
  - if you're building on Windows 11 ARM64, pass `-o "&:lua_lib=lua"`
  - if you're building on (or for) Windows < 10, pass `-o "&:target_pre_windows10=True"`

If you use `--build=never` and this command fails, then it means that you can't use prebuilt binaries out of the box. For example, try using `--build=missing` instead.

VCMI "recipe" also has some options that you can specify. For example, if you don't care about game videos, you can disable FFmpeg dependency by passing `-o "&:with_ffmpeg=False"`. Check [the recipe](https://github.com/vcmi/vcmi-dependencies/tree/main/conanfile.py) for details.

*Note*: you can find full reference of this command [in the official documentation](https://docs.conan.io/2/reference/commands/install.html) or by executing `conan install -h`.

### Using prebuilt binaries from ConanCenter

First, check if binaries for [your platform](https://github.com/conan-io/conan-center-index/blob/master/docs/supported_platforms_and_configurations.md) are produced.

You must adjust the above `conan install` command by replacing ***dependencies/conan_profiles/PROFILE*** with `default` (or simply omit the `--profile` parameter).

### Building dependencies from source

This subsection describes platform specifics to build libraries from source properly. Commands that our CI uses to build the dependencies can also be used as a reference, you can find them in the [dependcies repository](https://github.com/vcmi/vcmi-dependencies/blob/main/.github/workflows/rebuildDependencies.yml).

You can use our Conan profiles or create your own (e.g. with different options), the choice is yours.

*Note:* our profiles expect you to have CMake and Ninja in `PATH`, otherwise build would fail.

#### Building for macOS/iOS

If you wish to build dependencies against system libraries (like our prebuilts do), follow [below instructions](#using-recipes-for-system-libraries) executing `conan create` for all directories.

If you're going to build for iOS without using our profile: to build Qt 5 with `md4c` library, make sure to enforce this library's version 0.5 or later. This is what we have in our profile:

> [replace_requires]\
> md4c/0.4.8: md4c/0.5.2

#### Building for Android

It's highly recommended to use NDK recipe provided by Conan, otherwise build may fail. If you're using your own Conan profile, you can include our [NDK profile](https://github.com/vcmi/vcmi-dependencies/tree/main/conan_profiles/base/android-ndk) via an additional `--profile` parameter on the command line.

Android has issues loading self-built shared zlib library because binary name is identical to the system one, so we enforce using the OS-provided library. To achieve that, follow [below instructions](#using-recipes-for-system-libraries), you only need `zlib` directory.

Also, Android requires a few patches, but they are needed only for specific use cases, so you may evaluate whether you need them. The patches can be found in the [dependcies repository](https://github.com/vcmi/vcmi-dependencies/blob/main/conan_patches/qt/patches).

##### Qt patches

1. [Safety measure for Xiaomi devices](https://github.com/vcmi/vcmi-dependencies/blob/main/conan_patches/qt/patches/xiaomi.diff). It's unclear whether it's really needed, but without it [I faced a crash once](https://bugreports.qt.io/browse/QTBUG-111960).
2. [Fix running on Android 5.0-5.1](https://github.com/vcmi/vcmi-dependencies/blob/main/conan_patches/qt/patches/android-21-22.diff) (API level 21-22).
3. Enable running on Android 4.4 (API level 19-20, 32-bit only): [patch 1](https://github.com/vcmi/vcmi-dependencies/blob/main/conan_patches/qt/patches/android-19-jar.diff), [patch 2](https://github.com/vcmi/vcmi-dependencies/blob/main/conan_patches/qt/patches/android-19-java.diff).

##### Patches for other libraries

- Flac requires patch to be able to build it for 32-bit targeting API Level < 24
- Minizip requires patch to be able to build it for 32-bit targeting API Level < 21

Also, to build Flac, Luajit and Opusfile for 32-bit targeting API Level < 24, a couple of C defines must be added to your Conan profile - see our [profile for ARM 32-bit](https://github.com/vcmi/vcmi-dependencies/blob/main/conan_profiles/android-32).

#### Using recipes for system libraries

1. Clone/download <https://github.com/kambala-decapitator/conan-system-libs>
2. Execute `conan create PACKAGE --user system`, where `PACKAGE` is a directory path in that repository. Do it for each library you need. (basically just read repo's readme)

#### Build it

First, you must build separate libraries with `conan create` if:

- you chose to use patches (ours listed above or your own) for a library. You must also pass `--core-conf core.sources.patch:extra_path=<patches path>` parameter where `<patches path>` is the path to the patches directory, ours is located at [dependencies/conan_patches](https://github.com/vcmi/vcmi-dependencies/tree/main/conan_patches).
- you want to build LuaJIT for iOS or Android (they also require patches, see the above point). Upstream Conan recipe doesn't support this yet (but there's a [pull request](https://github.com/conan-io/conan-center-index/pull/26577)), so you'll have to use [fork](https://github.com/kambala-decapitator/conan-center-index/tree/package/luajit) where it works. *Note*: to build for 32-bit architecture (e.g. Android armv7) your OS must be able to run 32-bit executables, see [this issue](https://github.com/LuaJIT/LuaJIT/issues/664) for details (for example, macOS since 10.15 can't do that); on Linux amd64 you'll have to install `libc6-dev-i386` package.

After that you can execute `conan install` to build the rest of the dependencies.

## Configure project for building

You must pass the generated toolchain file to CMake invocation.

- if using custom CMake presets, define the path to the generated toolchain in `toolchainFile` field (or `CMAKE_TOOLCHAIN_FILE` cache variable) or include CMake presets file generated by Conan.
- otherwise (passing CMake options on the command line, configuring them in an IDE etc.), use `--toolchain` CLI option (available in CMake 3.21+) or `CMAKE_TOOLCHAIN_FILE` variable.

## Examples

In these examples only the minimum required amount of options is passed to `cmake` invocation, you can pass additional ones as needed.

### Use our prebuilt binaries to build for Windows x64 with Visual Studio (CMD shell)

```batchfile
conan install . ^
  --output-folder=conan-msvc ^
  --build=never ^
  --profile=dependencies\conan_profiles\msvc-x64 ^
  -s "&:build_type=RelWithDebInfo"

REM this is important!
conan-msvc\conanrun.bat

cmake -S . -B build ^
  --toolchain conan-msvc\conan_toolchain.cmake
```

### Use our prebuilt binaries to build for macOS arm64 with Xcode

```sh
conan install . \
  --output-folder=conan-macos \
  --build=never \
  --profile=dependencies/conan_profiles/macos-arm \
  --profile=dependencies/conan_profiles/base/apple-system \
  -s "&:build_type=Debug"

cmake -S . -B build -G Xcode \
  --toolchain conan-macos/conan_toolchain.cmake
```

### Use our prebuilt binaries to build for Android arm64 with Ninja

```sh
conan install . \
  --output-folder=conan-android64 \
  --build=never \
  --profile=dependencies/conan_profiles/android-64-ndk \
  --profile=dependencies/conan_profiles/base/android-system \
  -s "&:build_type=Debug"

cmake -S . -B build -G Ninja \
  --toolchain conan-android64/conan_toolchain.cmake
```

### Try to use binaries from ConanCenter for your platform

If you also want to build the missing binaries from source, use `--build=missing` instead of `--build=never`.

```sh
conan install . \
  --install-folder=~/my-dir \
  --build=never \
  -s "&:build_type=Debug"

cmake -S . -B build \
  -D CMAKE_TOOLCHAIN_FILE=~/my-dir/conan_toolchain.cmake
```

### Use our prebuilt binaries to build for iOS arm64 device with custom preset

```sh
conan install . \
  --output-folder=conan-ios \
  --build=never \
  --profile=dependencies/conan_profiles/ios-arm64 \
  --profile=dependencies/conan_profiles/base/apple-system \
  -s "&:build_type=Debug"

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
            "inherits": ["ios-device"],
            "toolchainFile": "conan-ios/conan_toolchain.cmake",
            "cacheVariables": {
                "BUNDLE_IDENTIFIER_PREFIX": "com.YOUR-NAME",
                "CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM": "YOUR_TEAM_ID"
            }
        }
    ]
}
```
