# Fork features

- [x] Works with Gear protocol via the `gear-connector` app.
- [x] Saves game states to the chain.
- [x] Load game states from the chain.

# Installation guide

## Contract

1. Download the contract from https://github.com/gear-dapps/homm3
2. Upload contracts `.opt.wasm` files with IDEA to https://idea.gear-tech.io/programs?node=wss%3A%2F%2Ftestnet.vara.rs

## IPFS daemon

```bash
ipfs init # When running for the first time only
ipfs daemon
```

## Game

1. Download the binaries package from the [Releases](https://github.com/gear-dapps/vcmi/releases) section according to your OS.
2. Unpack the archive and run:

```bash
VCMICLIENT_PATH=./vcmiclient ./gear-connector
```

## VCMI Installation guides

To use VCMI you need to own original data files.

 * [Android](https://wiki.vcmi.eu/Installation_on_Android)
 * [Linux](https://wiki.vcmi.eu/Installation_on_Linux)
 * [macOS](https://wiki.vcmi.eu/Installation_on_macOS)
 * [Windows](https://wiki.vcmi.eu/Installation_on_Windows)
 * [iOS](https://wiki.vcmi.eu/Installation_on_iOS)

 # Building from source

## Clone this repository

```bash
git clone https://github.com/gear-dapps/vcmi
cd vcmi
```

## Install dependencies

```bash
cd vcmi
./CI/<platform>/before_install.sh
```

## Build VCMI from the source

```bash
mkdir -p build
cmake -S . -B build
cmake --build build
```

Find executables in the `build/bin` directory.

## Install dependencies for Tauri.

Example for Ubuntu:

```bash
  sudo apt update
  sudo apt install -y libwebkit2gtk-4.0-dev \
    build-essential curl wget \
    libssl-dev libgtk-3-dev \
    libayatana-appindicator3-dev librsvg2-dev
```

For Windows, macOS look at [official Tauri docs](https://tauri.app/v1/guides/getting-started/prerequisites)

For another Linux distribution look at https://tauri.app/v1/guides/getting-started/prerequisites#setting-up-linux

## Build Tauri app

```bash
cargo b -r --manifest-path=gear-connector/src-tauri/Cargo.toml
```

Find the `gear-connector` executable in the `gear-connector/src-tauri/target/release` directory.

## Run Tauri app

```bash
VCMICLIENT_PATH=./build/bin/vcmiclient ./gear-connector/src-tauri/target/release/gear-connector
```
