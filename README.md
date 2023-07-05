### What this version do
+ Works with gear protocal via gear-connector
+ Saves game states to chain
+ Load game states from chain

# Installation guide
## Contract
1. Download contract from https://github.com/gear-dapps/homm3
2. Upload contract .wasm file with IDEA to https://idea.gear-tech.io/programs?node=wss%3A%2F%2Ftestnet.vara.rs

## Game (for Ubuntu https://ubuntu.com/download/desktop)
1. Clone this repository
2. Install needed dependencies for tauri gui framework  

   ### Install dependencies for tauri.  

   Example for Ubuntu:  
  ```Ubuntu
    sudo apt update
    sudo apt install libwebkit2gtk-4.0-dev \
    build-essential \
    curl \
    wget \
    libssl-dev \
    libgtk-3-dev \
    libayatana-appindicator3-dev \
    librsvg2-dev
  ```
For Windows, macOS look at [official tauri docs](https://tauri.app/v1/guides/getting-started/prerequisites)

For another Linux distribution look at https://tauri.app/v1/guides/getting-started/prerequisites#setting-up-linux

3. Install VCMI 
## VCMI Installation guides

To use VCMI you need to own original data files.

 * [Android](https://wiki.vcmi.eu/Installation_on_Android)
 * [Linux](https://wiki.vcmi.eu/Installation_on_Linux)
 * [macOS](https://wiki.vcmi.eu/Installation_on_macOS)
 * [Windows](https://wiki.vcmi.eu/Installation_on_Windows)
 * [iOS](https://wiki.vcmi.eu/Installation_on_iOS)

## Building from source

Platform support is constantly tested by continuous integration and CMake configuration adjusted to generate nice looking projects for all major IDE. Following guides will help you to setup build environment with no effort:

 * (optional) All platforms: [using Conan package manager to obtain prebuilt dependencies](docs/conan.md)
 * [On Linux](https://wiki.vcmi.eu/How_to_build_VCMI_(Linux))
 * [On Linux for Windows with Conan and mingw](https://wiki.vcmi.eu/How_to_build_VCMI_(Linux/Cmake/Conan))
 * [On macOS](https://wiki.vcmi.eu/How_to_build_VCMI_(macOS))
 * [On Windows using MSVC and Vcpkg](https://wiki.vcmi.eu/How_to_build_VCMI_(Windows/Vcpkg))
 * [iOS on macOS](https://wiki.vcmi.eu/How_to_build_VCMI_(iOS))
 * [Android on any OS](https://wiki.vcmi.eu/How_to_build_VCMI_(Android))
