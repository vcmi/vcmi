# Using dependencies from Conan

[Conan](https://conan.io/) is a package manager for C/C++. We provide prebuilt binary dependencies for some platforms that are used by our CI, but they can also be consumed by users to build VCMI. However, it's not required to use only the prebuilt binaries: you can build them from source as well.

## Supported platforms

The following platforms are supported and known to work, others might require changes to our [conanfile.py](../conanfile.py) or upstream recipes.

- **macOS**: x86_64 (Intel) - target 10.13 (High Sierra), arm64 (Apple Silicon) - target 11.0 (Big Sur)
- **iOS**: arm64 - target 12.0

## Getting started

1. [Install Conan](https://docs.conan.io/en/latest/installation.html)
2. Execute in terminal: `conan profile new default --detect`

## Download dependencies

0. If your platform is not on the list of supported ones or you don't want to use our prebuilt binaries, you can still build dependencies from source or try consuming prebuilt binaries from the central Conan repository - [ConanCenter](https://conan.io/center/). In this case skip to the [next section](#generate-cmake-integration) directly.

1. Check if your build environment can use the prebuilt binaries: basically, that your compiler version (or Xcode major version) matches the information below. If you're unsure, simply advance to the next step.

    - macOS: libraries are built with Apple clang 14 (Xcode 14.2), should be consumable by Xcode and Xcode CLT 14.x (older library versions are also available for Xcode 13, see Releases in the respective repo)
    - iOS: libraries are built with Apple clang 14 (Xcode 14.2), should be consumable by Xcode 14.x (older library versions are also available for Xcode 13, see Releases in the respective repo)

2. Download the binaries archive and unpack it to `~/.conan` directory:

    - [macOS](https://github.com/vcmi/vcmi-deps-macos/releases/latest): pick **intel.txz** if you have Intel Mac, otherwise - **intel-cross-arm.txz**
    - [iOS](https://github.com/vcmi/vcmi-ios-deps/releases/latest)

3. Only if you have Apple Silicon Mac and trying to build for macOS or iOS: follow [instructions how to build Qt host tools for Apple Silicon](https://github.com/vcmi/vcmi-ios-deps#note-for-arm-macs), on step 3 copy them to `~/.conan/data/qt/5.15.x/_/_/package/SOME_HASH/bin` (`5.15.x` and `SOME_HASH` are placeholders).

## Generate CMake integration

Conan needs to generate CMake toolchain file to make dependencies available to CMake. See `CMakeDeps` and `CMakeToolchain` [in the official documentation](https://docs.conan.io/en/latest/reference/conanfile/tools/cmake.html) for details.

In terminal `cd` to the VCMI source directory and run the following command. If you want to download prebuilt binaries from ConanCenter, also read the [next section](#using-prebuilt-binaries-from-conancenter).

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

If you use `--build=never` and this command fails, then it means that you can't use prebuilt binaries out of the box. For example, try using `--build=missing` instead.

VCMI "recipe" also has some options that you can specify. For example, if you don't care about game videos, you can disable FFmpeg dependency by passing `-o with_ffmpeg=False`. If you only want to make release build, you can use `GENERATE_ONLY_BUILT_CONFIG=1` environment variable to skip generating files for other configurations (our CI does this).

_Note_: you can find full reference of this command [in the official documentation](https://docs.conan.io/en/latest/reference/commands/consumer/install.html) or by executing `conan help install`.

### Using prebuilt binaries from ConanCenter

First, check if binaries for [your platform](https://github.com/conan-io/conan-center-index/blob/master/docs/supported_platforms_and_configurations.md) are produced.

You must adjust the above `conan install` command:

1. Replace ***CI/conan/PROFILE*** with `default`.
2. Additionally pass `-o default_options_of_requirements=True`: this disables all custom options of our `conanfile.py` to match ConanCenter builds.

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
