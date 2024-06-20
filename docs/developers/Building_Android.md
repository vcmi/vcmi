The following instructions apply to **v1.2 and later**. For earlier versions the best documentation is https://github.com/vcmi/vcmi-android/blob/master/building.txt (and reading scripts in that repo), however very limited to no support will be provided from our side if you wish to go down that rabbit hole.

*Note*: building has been tested only on Linux and macOS. It may or may not work on Windows out of the box.

## Requirements

1. CMake 3.20+: download from your package manager or from https://cmake.org/download/
2. JDK 11, not necessarily from Oracle
3. Android command line tools or Android Studio for your OS: https://developer.android.com/studio/
4. Android NDK version **r25c (25.2.9519653)**, there're multiple ways to obtain it:
    - install with Android Studio
    - install with `sdkmanager` command line tool
    - download from https://developer.android.com/ndk/downloads
    - download with Conan, see [#NDK and Conan](#ndk-and-conan)
5. Optional:
    - Ninja: download from your package manager or from https://github.com/ninja-build/ninja/releases
    - Ccache: download from your package manager or from https://github.com/ccache/ccache/releases

## Obtaining source code

Clone https://github.com/vcmi/vcmi with submodules. Example for command line:

```
git clone --recurse-submodules https://github.com/vcmi/vcmi.git
```

## Obtaining dependencies

We use Conan package manager to build/consume dependencies, find detailed usage instructions [here](./Conan.md). Note that the link points to the state of the current branch, for the latest release check the same document in the [master branch](https://github.com/vcmi/vcmi/blob/master/docs/developers/Сonan.md).

On the step where you need to replace **PROFILE**, choose:
- `android-32` to build for 32-bit architecture (armeabi-v7a)
- `android-64` to build for 64-bit architecture (aarch64-v8a)

### NDK and Conan

Conan must be aware of the NDK location when you execute `conan install`. There're multiple ways to achieve that as written in the [Conan docs](https://docs.conan.io/1/integrations/cross_platform/android.html):

- the easiest is to download NDK from Conan (option 1 in the docs), then all the magic happens automatically. On the step where you need to replace **PROFILE**, choose _android-**X**-ndk_ where _**X**_ is either `32` or `64`.
- to use an already installed NDK, you can simply pass it on the command line to `conan install`: (note that this will work only when consuming the pre-built binaries)

```
conan install -c tools.android:ndk_path=/path/to/ndk ...
```

## Build process

Building for Android is a 2-step process. First, native C++ code is compiled to a shared library (unlike an executable on other platforms), then Java code is compiled to an actual executable which will be loading the native shared library at runtime.

This is a traditional CMake project, you can build it from command line or some IDE. You're not required to pass any custom options (except Conan toolchain file), defaults are already good. If you wish to use your own CMake presets, inherit them from our `build-with-conan` preset.

The Java code (located in the `android` directory of the repo) will be built automatically after the native code using the `androiddeployqt` tool. But you must set `JAVA_HOME` and `ANDROID_HOME` environment variables.

APK will appear in `<build dir>/android-build/vcmi-app/build/outputs/apk/debug` directory which you can then install to your device with `adb install -r /path/to/apk` (`adb` command is from Android command line tools).

### Example

```sh
# the following environment variables must be set
export JAVA_HOME=/path/to/jdk11
export ANDROID_HOME=/path/to/android/sdk

cmake -S . -B ../build -G Ninja -D CMAKE_BUILD_TYPE=Debug -D ENABLE_CCACHE:BOOL=ON --toolchain ...
cmake --build ../build
```

You can also see a more detailed walkthrough on CMake configuration at [How to build VCMI (macOS)](./Building_macOS.md).
