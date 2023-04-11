# Using dependencies from Conan

[Conan](https://conan.io/) is a package manager for C/C++. We provide prebuilt binary dependencies for some platforms that are used by our CI, but they can also be consumed by users to build VCMI. However, it's not required to use only the prebuilt binaries: you can build them from source as well.

## Supported platforms

The following platforms are supported and known to work, others might require changes to our [conanfile.py](../conanfile.py) or upstream recipes.

- **macOS**: x86_64 (Intel) - target 10.13 (High Sierra), arm64 (Apple Silicon) - target 11.0 (Big Sur)
- **iOS**: arm64 - target 12.0
- **Windows**: x86_64 and x86 fully supported with building from Linux
- **Android**: armeabi-v7a (32-bit ARM) - target 4.4 (API level 19), aarch64-v8a (64-bit ARM) - target 5.0 (API level 21)

## Getting started

1. [Install Conan](https://docs.conan.io/en/latest/installation.html). Currently we support only Conan v1, you can install it with `pip` like this: `pip3 install 'conan<2.0'`
2. Execute in terminal: `conan profile new default --detect`

## Download dependencies

0. If your platform is not on the list of supported ones or you don't want to use our prebuilt binaries, you can still build dependencies from source or try consuming prebuilt binaries from the central Conan repository - [ConanCenter](https://conan.io/center/). In this case skip to the [next section](#generate-cmake-integration) directly.

1. Check if your build environment can use the prebuilt binaries: basically, that your compiler version (or Xcode major version) matches the information below. If you're unsure, simply advance to the next step.

    - **macOS**: libraries are built with Apple clang 14 (Xcode 14.2), should be consumable by Xcode and Xcode CLT 14.x (older library versions are also available for Xcode 13, see Releases in the respective repo)
    - **iOS**: libraries are built with Apple clang 14 (Xcode 14.2), should be consumable by Xcode 14.x (older library versions are also available for Xcode 13, see Releases in the respective repo)
    - **Windows**: libraries are built with x86_64-mingw-w64-gcc version 10 (which is available in repositories of Ubuntu 22.04)
    - **Android**: libraries are built with NDK r25c (25.2.9519653)

2. Download the binaries archive and unpack it to `~/.conan` directory:

    - [macOS](https://github.com/vcmi/vcmi-deps-macos/releases/latest): pick **intel.txz** if you have Intel Mac, otherwise - **intel-cross-arm.txz**
    - [iOS](https://github.com/vcmi/vcmi-ios-deps/releases/latest)
    - [Windows](https://github.com/vcmi/vcmi-deps-windows-conan/releases/latest): pick **vcmi-deps-windows-conan-w64.tgz**
    if you want x86, otherwise pick **vcmi-deps-windows-conan.tgz**
    - [Android](https://github.com/vcmi/vcmi-dependencies/releases)

3. Only if you have Apple Silicon Mac and trying to build for macOS or iOS:

    1. Open file `~/.conan/data/qt/5.15.x/_/_/export/conanfile.py` (`5.15.x` is a placeholder), search for string `Designer` (there should be only one match), comment this line and the one above it by inserting `#` in the beginning, and save the file.
    2. (optional) If you don't want to use Rosetta, follow [instructions how to build Qt host tools for Apple Silicon](https://github.com/vcmi/vcmi-ios-deps#note-for-arm-macs), on step 3 copy them to `~/.conan/data/qt/5.15.x/_/_/package/SOME_HASH/bin` (`5.15.x` and `SOME_HASH` are placeholders). Make sure **not** to copy `qt.conf`!

## Generate CMake integration

Conan needs to generate CMake toolchain file to make dependencies available to CMake. See `CMakeDeps` and `CMakeToolchain` [in the official documentation](https://docs.conan.io/en/latest/reference/conanfile/tools/cmake.html) for details.

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
- ***CI/conan/PROFILE***: if you want to consume our prebuilt binaries, ***PROFILE*** must be replaced with one of filenames from our [Conan profiles directory](../CI/conan) (determining the right file should be straight-forward). Otherwise, either select one of our profiles or replace ***CI/conan/PROFILE*** with `default` (your default profile).
- ***note for Windows x86***: use profile mingw32-linux.jinja for building instead of mingw64-linux.jinja

If you use `--build=never` and this command fails, then it means that you can't use prebuilt binaries out of the box. For example, try using `--build=missing` instead.

VCMI "recipe" also has some options that you can specify. For example, if you don't care about game videos, you can disable FFmpeg dependency by passing `-o with_ffmpeg=False`. If you only want to make release build, you can use `GENERATE_ONLY_BUILT_CONFIG=1` environment variable to skip generating files for other configurations (our CI does this).

_Note_: you can find full reference of this command [in the official documentation](https://docs.conan.io/en/latest/reference/commands/consumer/install.html) or by executing `conan help install`.

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

Android has issues loading self-built shared Zlib library because binary name is identical to the system one, so we enforce using the OS-provided library. To achieve that, follow [below instructions](#using-recipes-for-system-libraries), you only need `zlib` directory.

##### Using recipes for system libraries

1. Clone/download https://github.com/kambala-decapitator/conan-system-libs
2. Execute `conan create PACKAGE vcmi/apple`, where `PACKAGE` is a directory path in that repository. Do it for each library you need.
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
