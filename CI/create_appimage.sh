#!/bin/bash
set -e

# Parse arguments
USE_CONAN=0
for arg in "$@"; do
    case "$arg" in
        --use-conan) USE_CONAN=1 ;;
        *) echo "Unknown argument: $arg"; exit 1 ;;
    esac
done

# Define paths
SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(dirname "$SCRIPT_DIR")
# Allow BUILD_DIR to be overridden by environment variable
if [ -z "$BUILD_DIR" ]; then
    BUILD_DIR="$REPO_ROOT/out/build/linux-clang-debug"
fi
APP_DIR="$REPO_ROOT/AppDir"
OUT_DIR="$REPO_ROOT/out"

# Define architecture
if [ -z "$ARCH" ]; then
    ARCH="x86_64"
fi

if [ "$ARCH" == "arm64" ]; then
    LINUXDEPLOY_ARCH="aarch64"
elif [ "$ARCH" == "aarch64" ]; then
    LINUXDEPLOY_ARCH="aarch64"
else
    LINUXDEPLOY_ARCH="x86_64"
fi

# Function to download linuxdeploy if not present
download_tools() {
    if [ ! -f "linuxdeploy-$LINUXDEPLOY_ARCH.AppImage" ]; then
        echo "Downloading linuxdeploy..."
        wget -q "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$LINUXDEPLOY_ARCH.AppImage"
        chmod +x "linuxdeploy-$LINUXDEPLOY_ARCH.AppImage"
    fi

    if [ ! -f "linuxdeploy-plugin-qt-$LINUXDEPLOY_ARCH.AppImage" ]; then
        echo "Downloading linuxdeploy-plugin-qt..."
        wget -q "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-$LINUXDEPLOY_ARCH.AppImage"
        chmod +x "linuxdeploy-plugin-qt-$LINUXDEPLOY_ARCH.AppImage"
    fi
}

# cleanup previous run
rm -rf "$APP_DIR"
mkdir -p "$APP_DIR/usr/bin" "$APP_DIR/usr/lib" "$APP_DIR/usr/share/applications" "$APP_DIR/usr/share/icons/hicolor/512x512/apps"

# Copy binaries
echo "Copying binaries..."
cp "$BUILD_DIR/bin/vcmiclient" "$APP_DIR/usr/bin/"
cp "$BUILD_DIR/bin/vcmiserver" "$APP_DIR/usr/bin/"
cp "$BUILD_DIR/bin/vcmilauncher" "$APP_DIR/usr/bin/"
cp "$BUILD_DIR/bin/vcmieditor" "$APP_DIR/usr/bin/"

# Size Optimization: Strip binaries and libraries
echo "Stripping binaries and libraries to reduce size..."
find "$APP_DIR/usr/bin" "$APP_DIR/usr/lib" -type f -exec strip --strip-unneeded {} + 2>/dev/null || true

# Copy resources
echo "Copying resources..."
cp "$REPO_ROOT/clientapp/icons/vcmiclient.512x512.png" "$APP_DIR/usr/share/icons/hicolor/512x512/apps/vcmiclient.png"
cp "$REPO_ROOT/launcher/vcmilauncher.desktop" "$APP_DIR/usr/share/applications/"

# Copy config and Mods
# These are symlinks in the build dir, so we copy the actual folders from repo root
echo "Copying config and Mods..."
cp -r "$REPO_ROOT/config" "$APP_DIR/usr/bin/"
cp -r "$REPO_ROOT/Mods" "$APP_DIR/usr/bin/"

# Set up version
if [ -z "$VERSION" ]; then
    VERSION_FILE="$REPO_ROOT/cmake_modules/VersionDefinition.cmake"
    if [ -f "$VERSION_FILE" ]; then
        V_MAJOR=$(grep -m 1 "VCMI_VERSION_MAJOR" "$VERSION_FILE" | tr -d -c 0-9)
        V_MINOR=$(grep -m 1 "VCMI_VERSION_MINOR" "$VERSION_FILE" | tr -d -c 0-9)
        V_PATCH=$(grep -m 1 "VCMI_VERSION_PATCH" "$VERSION_FILE" | tr -d -c 0-9)
        export VERSION="${V_MAJOR}.${V_MINOR}.${V_PATCH}"
    else
        export VERSION="continuous"
    fi
fi

# Set up environment variables for linuxdeploy
export VERSION
export UPD_INFO="gh-releases-zsync|vcmi|vcmi|continuous|VCMI-*$ARCH.AppImage.zsync"

# Detect qmake
if [[ -z "$QMAKE" ]]; then
    if [[ "$USE_CONAN" -eq 1 ]]; then
        # --use-conan: require qmake from Conan2 cache, no system fallback
		qtPackage='qt'
		hexRegex='[[:xdigit:]]+'
		
		qtPackageRevision=$(conan list --format=compact "$qtPackage/*:*" \
		  | egrep --only-matching "$qtPackage/[[:alnum:]._-]+#$hexRegex:$hexRegex" \
		  | head -n1)
		
		qmakeDir="$(conan cache path "$qtPackageRevision")/bin"
		CONAN_QMAKE="$qmakeDir/qmake"

        if [[ -z "$qtPackageRevision" || ! -x "$CONAN_QMAKE" ]]; then
            echo "Error: --use-conan set but no qmake found in Conan cache" >&2
            exit 1
        fi

        export QMAKE="$CONAN_QMAKE"
        echo "Using Conan Qt qmake: $QMAKE"
    else
        # No Conan: use system Qt
        if command -v qmake &>/dev/null; then
            export QMAKE=$(command -v qmake)
            echo "Using system Qt qmake: $QMAKE"
        elif command -v qmake6 &>/dev/null; then
            export QMAKE=$(command -v qmake6)
            echo "Using system Qt6 qmake: $QMAKE"
        else
            echo "Error: No qmake found" >&2
            exit 1
        fi
    fi
fi

# Some Conan-built shared libs (e.g. SDL2_mixer) have no RPATH/RUNPATH, so
# linuxdeploy's dependency resolver cannot locate their transitive deps
# (e.g. libopus.so.0).  Adding all Conan lib directories to LD_LIBRARY_PATH
# makes them visible to linuxdeploy.
if [[ "$USE_CONAN" -eq 1 ]] && [[ -d "${HOME}/.conan2/p" ]]; then
    CONAN_LIB_DIRS=$(find "${HOME}/.conan2/p" \( -name "*.so" -o -name "*.so.*" \) 2>/dev/null \
                     | xargs -r dirname 2>/dev/null | sort -u | paste -sd:)
    export LD_LIBRARY_PATH="${CONAN_LIB_DIRS}${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
    echo "Added Conan lib dirs to LD_LIBRARY_PATH"
fi

# Run linuxdeploy
echo "Running linuxdeploy..."
download_tools

"./linuxdeploy-$LINUXDEPLOY_ARCH.AppImage" \
    --appdir "$APP_DIR" \
    --plugin qt \
    --output appimage \
    --desktop-file "$APP_DIR/usr/share/applications/vcmilauncher.desktop" \
    --icon-file "$APP_DIR/usr/share/icons/hicolor/512x512/apps/vcmiclient.png" \
    --executable "$APP_DIR/usr/bin/vcmilauncher"

# cleanup
rm -f linuxdeploy-*.AppImage

echo "AppImage creation complete!"

# Move to build directory if specified (for CI integration)
APPIMAGE_FILE=$(ls VCMI-*.AppImage 2>/dev/null | head -n 1)
if [ -n "$APPIMAGE_FILE" ] && [ -n "$BUILD_DIR" ]; then
    # Use VCMI_PACKAGE_FILE_NAME from CI if available, otherwise original name
    TARGET_NAME="${VCMI_PACKAGE_FILE_NAME:-${APPIMAGE_FILE%.*}}.AppImage"
    echo "Ensuring $BUILD_DIR exists and moving $APPIMAGE_FILE to $BUILD_DIR/$TARGET_NAME"
    mkdir -p "$BUILD_DIR"
    mv "$APPIMAGE_FILE" "$BUILD_DIR/$TARGET_NAME"
fi
