#!/usr/bin/env bash

MSVC_INSTALL_PATH=$(vswhere -latest -property installationPath)
echo "MSVC_INSTALL_PATH = $MSVC_INSTALL_PATH"
echo "Installed toolset versions:"
ls -vr "$MSVC_INSTALL_PATH/VC/Tools/MSVC"

TOOLS_DIR=$(ls -vr "$MSVC_INSTALL_PATH/VC/Tools/MSVC/" | head -1)
DUMPBIN_PATH="$MSVC_INSTALL_PATH/VC/Tools/MSVC/$TOOLS_DIR/bin/Hostx64/x64/dumpbin.exe"

# This command should work as well, but for some reason it is *extremely* slow on the Github CI (~7 minutes)
#DUMPBIN_PATH=$(vswhere -latest -find **/dumpbin.exe | head -n 1)

echo "TOOLS_DIR = $TOOLS_DIR"
echo "DUMPBIN_PATH = $DUMPBIN_PATH"

dirname "$DUMPBIN_PATH" > "$GITHUB_PATH"
