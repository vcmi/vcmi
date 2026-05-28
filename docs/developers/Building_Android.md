# Building Android

The following instructions apply to **v1.7 and later**. For earlier versions see older Git revision, e.g. from [1.6.8 release](https://github.com/vcmi/vcmi/blob/1.6.8/docs/developers/Building_Android.md).

*Note*: building has been tested only on Linux and macOS. It may or may not work on Windows out of the box.

## Requirements

1. CMake 3.26+: download from your package manager or from <https://cmake.org/download/>
2. JDK 17, not necessarily from Oracle
3. Android command line tools or Android Studio for your OS: <https://developer.android.com/studio/>
4. Android NDK version **r25c (25.2.9519653)** (later version would also probably work), there're multiple ways to obtain it:
    - recommended: download with Conan, especially if you're going to build VCMI dependencies from source, see [#NDK and Conan](#ndk-and-conan)
    - install with Android Studio
    - install with `sdkmanager` command line tool
    - download from <https://developer.android.com/ndk/downloads>
5. Optional:
    - Ninja: download from your package manager or from <https://github.com/ninja-build/ninja/releases>
    - Ccache: download from your package manager or from <https://github.com/ccache/ccache/releases>

## Obtaining source code

Clone <https://github.com/vcmi/vcmi> with submodules. Example for command line:

```sh
git clone --recurse-submodules https://github.com/vcmi/vcmi.git
```

## Obtaining dependencies

We use Conan package manager to build/consume dependencies, find detailed usage instructions [here](./Conan.md). Note that the link points to the state of the current branch, for the latest release check the same document in the [master branch](https://github.com/vcmi/vcmi/blob/master/docs/developers/Conan.md).

On the step where you need to replace **PROFILE**, choose:

- `android-32-ndk` to build for ARM 32-bit (armeabi-v7a)
- `android-64-ndk` to build for ARM 64-bit (aarch64-v8a)
- `android-x64-ndk` to build for Intel 64-bit (x86_64)

Advanced users may choose profile without `-ndk` suffix to use NDK that's already installed in their system.

### NDK and Conan

Conan must be aware of the NDK location when you execute `conan install`. There're multiple ways to achieve that as written in the [Conan docs](https://docs.conan.io/2/examples/cross_build/android/ndk.html#examples-cross-build-android-ndk):

- the easiest is to download NDK from Conan (option 1 in the docs), then all the magic happens automatically
- to use an already installed NDK, you can simply pass it on the command line to `conan install`: (note that this will likely work only when consuming the prebuilt binaries)

```sh
conan install -c tools.android:ndk_path=/path/to/ndk ...
```

## Build process

Before starting the build, local Gradle configuration must be created (it's a requirement for Qt 5):

```sh
mkdir ~/.gradle
echo "android.bundle.enableUncompressedNativeLibs=true" > ~/.gradle/gradle.properties
```

Building for Android is a 2-step process. First, native C++ code is compiled to a shared library (unlike an executable on other platforms), then Java code is compiled to an actual executable which will be loading the native shared library at runtime.

This is a traditional CMake project, you can build it from command line or some IDE. You're not required to pass any custom options (except Conan toolchain file), defaults are already good.

The Java code (located in the `android` directory of the repo) will be built automatically after the native code using the `androiddeployqt` tool. But you must set `JAVA_HOME` and `ANDROID_HOME` environment variables.

APK will appear in `<build dir>/android-build/vcmi-app/build/outputs/apk/<build configuration>` directory which you can then install to your device with `adb install -r /path/to/apk` (`adb` command is from Android command line tools).

### Example

```sh
# the following environment variables must be set
export JAVA_HOME=/path/to/jdk17
export ANDROID_HOME=/path/to/android/sdk

cmake -S . -B ../build -G Ninja -D CMAKE_BUILD_TYPE=Debug -D ENABLE_CCACHE:BOOL=ON --toolchain ...
cmake --build ../build
```

## Docker

For developing it's also possible to use Docker to build android APK. The only requirement is to have Docker installed. The container image contains all the other prerequisites.

To build using docker just open a terminal with `vcmi` repo root as working directory.

Build the image with (only needed once):

```sh
docker build -f docker/BuildAndroid-aarch64.dockerfile -t vcmi-android-build .
```

After building the image you can compile vcmi with:

```sh
docker run -it --rm -v $PWD/:/vcmi vcmi-android-build
```

The current dockerfile is aarch64 only but can be easily adjusted for other architectures.
