# Building VCMI for iOS

## Requirements

1. **macOS**
2. Xcode: <https://developer.apple.com/xcode/>
3. CMake 3.21+: `brew install --cask cmake` or get from <https://cmake.org/download/>
4. Optional:
   - CCache to speed up recompilation: `brew install ccache`

## Obtaining source code

Clone <https://github.com/vcmi/vcmi> with submodules. Example for command line:

```sh
git clone --recurse-submodules https://github.com/vcmi/vcmi.git
```

## Obtaining dependencies

The primary and officially supported way is [Conan package manager](./Conan.md). Note that the link points to the state of the current branch, for the latest release check the same document in the [master branch](https://github.com/vcmi/vcmi/blob/master/docs/developers/Conan.md).

There are also [legacy manually built libraries](https://github.com/vcmi/vcmi-ios-deps) which can be used if you have Xcode 11/12 or to build for simulator / armv7 device, but this way is no longer supported. Using Conan will also let you build with any Xcode version and for any architecture / SDK.

## Configuring project

Only Xcode generator (`-G Xcode`) is supported!

As a minimum, you must pass the following variables to CMake:

- `BUNDLE_IDENTIFIER_PREFIX`: unique bundle identifier prefix, something like `com.MY-NAME`

There's a [CMake preset](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html) for device named `ios-device-conan`. You can also create your local "user preset" to avoid typing variables each time, see example [here](https://gist.github.com/kambala-decapitator/59438030c34b53aed7d3895aaa48b718).

Open terminal and `cd` to the directory with source code. Configuration example for device with Conan:

```sh
cmake --preset ios-device-conan -D BUNDLE_IDENTIFIER_PREFIX=com.MY-NAME
```

By default build directory containing Xcode project will appear at `../build-ios-device-conan`, but you can change it with `-B` option.

If you want to speed up the recompilation, add `-D ENABLE_CCACHE=ON`

### Building for device

To be able to build for iOS device, you must also specify codesigning settings. If you don't know your development team ID, open the generated Xcode project, open project settings (click **VCMI** with blue icon on the very top in the left panel with files), select **vcmiclient** target, open **Signing & Capabilities** tab and select your team. Now you can copy the value from **Build Settings** tab - `DEVELOPMENT_TEAM` variable (paste it in the Filter field on the right) - click the greenish value - Other... - copy. Now you can pass it in `CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM` variable when configuring the project to avoid selecting the team manually every time CMake re-generates the project.

Advanced users who know exact private key and provisioning profile to sign with, can use `CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY` and `CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER` variables instead. In this case you must also pass `-D CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE=Manual`.

## Building project

### From Xcode IDE

Open `VCMI.xcodeproj` from the build directory, select `vcmiclient` scheme (the only one with nice icon) with your destination device/simulator and hit Run (Cmd+R).

You must also install game files, see [Installation on iOS](../players/Installation_iOS.md). But this is not necessary if you are going to run on simulator, as it is able to use game data from your Mac located at `~/Library/Application Support/vcmi`.

### From command line

```sh
cmake --build <path to build directory> --target vcmiclient -- -quiet
```

You can pass additional xcodebuild options after the `--`. Here `-quiet` is passed to reduce amount of output.

Alternatively, you can invoke `xcodebuild` directly.

There's also `ios-release-conan` configure and build preset that is used to create release build on CI.

## Creating ipa file for distribution

Invoke `cpack` after building:

`cpack -C Release`

This will generate file with extension **ipa** if you use CMake 3.25+ and **zip** otherwise (simply change extension to ipa).
