## Requirements

1.  **macOS**
2.  Xcode: <https://developer.apple.com/xcode/>
3.  CMake 3.21+: `brew install --cask cmake` or get from <https://cmake.org/download/>

## Obtaining source code

Clone <https://github.com/vcmi/vcmi> with submodules. Example for command line:

`git clone --recurse-submodules https://github.com/vcmi/vcmi.git`

## Obtaining dependencies

There're 2 ways to get prebuilt dependencies:

-   [Conan package manager](https://github.com/vcmi/vcmi/tree/develop/docs/conan.md) - recommended. Note that the link points to the cutting-edge state in `develop` branch, for the latest release check the same document in the [master branch (https://github.com/vcmi/vcmi/tree/master/docs/conan.md).
-   [legacy manually built libraries](https://github.com/vcmi/vcmi-ios-deps) - can be used if you have Xcode 11/12 or to build for simulator / armv7 device

## Configuring project

Only Xcode generator (`-G Xcode`) is supported!

As a minimum, you must pass the following variables to CMake:

-   `BUNDLE_IDENTIFIER_PREFIX`: unique bundle identifier prefix, something like `com.MY-NAME`
-   (if using legacy dependencies) `CMAKE_PREFIX_PATH`: path to the downloaded dependencies, e.g. `~/Downloads/vcmi-ios-depends/build/iphoneos`

There're a few [CMake presets](https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html): for device (Conan and legacy dependencies) and for simulator, named `ios-device-conan`, `ios-device` and `ios-simulator` respectively. You can also create your local "user preset" to avoid typing variables each time, see example [here](https://gist.github.com/kambala-decapitator/59438030c34b53aed7d3895aaa48b718).

Open terminal and `cd` to the directory with source code. Configuration example for device with Conan:

`cmake --preset ios-device-conan \`  
`  -D BUNDLE_IDENTIFIER_PREFIX=com.MY-NAME`

By default build directory containing Xcode project will appear at `../build-ios-device-conan`, but you can change it with `-B` option. 

### Building for device

To be able to build for iOS device, you must also specify codesigning settings. If you don't know your development team ID, open the generated Xcode project, open project settings (click **VCMI** with blue icon on the very top in the left panel with files), select **vcmiclient** target, open **Signing & Capabilities** tab and select yout team. Now you can copy the value from **Build Settings** tab - `DEVELOPMENT_TEAM` variable (paste it in the Filter field on the right) - click the greenish value - Other... - copy. Now you can pass it in `CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM` variable when configuring the project to avoid selecting the team manually every time CMake re-generates the project.

Advanced users who know exact private key and provisioning profile to sign with, can use `CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY` and `CMAKE_XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER` variables instead. In this case you must also pass `-D CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE=Manual`.

## Building project

### From Xcode IDE

Open `VCMI.xcodeproj` from the build directory, select `vcmiclient` scheme (the only one with nice icon) with your destination device/simulator and hit Run (Cmd+R).

You must also install game files, see [Installation on iOS](Installation_on_iOS "wikilink"). But this is not necessary if you are going to run on simulator, as it is able to use game data from your Mac located at `~/Library/Application Support/vcmi`.

### From command line

`cmake --build `<path to build directory>` --target vcmiclient -- -quiet`

You can pass additional xcodebuild options after the `--`. Here `-quiet` is passed to reduce amount of output.

Alternatively, you can invoke `xcodebuild` directly.

There's also `ios-release-conan` configure and build preset that is used to create release build on CI.

## Creating ipa file for distribution

Invoke `cpack` after building:

`cpack -C Release`

This will generate file with extension **ipa** if you use CMake 3.25+and **zip** otherwise (simply change extension to ipa).