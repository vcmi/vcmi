The following instructions apply to **v1.2 and later**. For earlier
versions the best documentation is
<https://github.com/vcmi/vcmi-android/blob/master/building.txt> (and
reading scripts in that repo), however very limited to no support will
be provided from our side if you wish to go down that rabbit hole.

*Note*: building has been tested only on Linux and macOS. It may or may
not work on Windows out of the box.

## Requirements

1.  CMake 3.20+: download from your package manager or from
    <https://cmake.org/download/>
2.  JDK 11, not necessarily from Oracle
3.  Android command line tools or Android Studio for your OS:
    <https://developer.android.com/studio/>
4.  Android NDK version **r25c (25.2.9519653)**, there're multiple ways
    to obtain it:
    -   install with Android Studio
    -   install with `sdkmanager` command line tool
    -   download from <https://developer.android.com/ndk/downloads>
    -   download with Conan, see [#NDK and
        Conan](#NDK_and_Conan "wikilink")
5.  (optional) Ninja: download from your package manager or from
    <https://github.com/ninja-build/ninja/releases>

## Obtaining source code

Clone <https://github.com/vcmi/vcmi> with submodules. Example for
command line:

`git clone --recurse-submodules https://github.com/vcmi/vcmi.git`

## Obtaining dependencies

We use Conan package manager to build/consume dependencies, find
detailed usage instructions
[here](https://github.com/vcmi/vcmi/tree/develop/docs/conan.md). Note
that the link points to the cutting-edge state in `develop` branch, for
the latest release check the same document in the [master
branch](https://github.com/vcmi/vcmi/tree/master/docs/conan.md).

On the step where you need to replace **PROFILE**, choose:

-   `android-32` to build for 32-bit architecture (armeabi-v7a)
-   `android-64` to build for 64-bit architecture (aarch64-v8a)

### NDK and Conan

Conan must be aware of the NDK location when you execute
`conan install`. There're multiple ways to achieve that as written in
the [Conan
docs](https://docs.conan.io/1/integrations/cross_platform/android.html):

-   the easiest is to download NDK from Conan (option 1 in the docs),
    then all the magic happens automatically. You need to create your
    own Conan profile that imports our Android profile and adds 2 new
    lines (you can of course just copy everything from our profile into
    yours without including) and then pass this new profile to
    `conan install`:

`include(/path/to/vcmi/CI/conan/android-64)`  
`[tool_requires]`  
`android-ndk/r25c`

-   to use an already installed NDK, you can simply pass it on the
    command line to `conan install`:

`conan install -c tools.android:ndk_path=/path/to/ndk ...`

## Build process

Building for Android is a 2-step process. First, native C++ code is
compiled to a shared library (unlike executable on other platforms),
then Java code is compiled to an actual executable which will be loading
the native shared library at runtime.

### C++ code

This is a traditional CMake project, you can build it from command line
or some IDE. You're not required to pass any custom options (except
Conan toolchain file), defaults are already good. If you wish to use
your own CMake presets, inherit them from our `build-with-conan` preset.
Example:

`cmake -S . -B ../build -G Ninja -D CMAKE_BUILD_TYPE=Debug --toolchain ...`  
`cmake --build ../build`

You can also see a more detailed walkthrough on CMake configuration at
[How to build VCMI (macOS)#Configuring project for
building](How_to_build_VCMI_(macOS)#Configuring_project_for_building "wikilink").

### Java code

After the C++ part is built, native shared libraries are copied to the
appropriate location of the Java project (they will be packaged in the
APK). You can either open the Java project located in `android`
directory of the repo in Android studio and work with it as with any
Android project or build from command line.

Example how to build from command line in Bash shell, assumes that
current working directory is VCMI repository:

`# the following environment variables must be set`  
`export JAVA_HOME=/path/to/jdk11`  
`export ANDROID_HOME=/path/to/android/sdk`  
``  
`cd android`  
`./gradlew assembleDebug`

APK will appear in `android/vcmi-app/build/outputs/apk/debug` directory
which you can then install to your device with
`adb install -r /path/to/apk` (adb command is from Android command line
tools).

If you wish to build and install to your device in single action, use
`installDebug` instead of `assembleDebug`.