#!/usr/bin/env bash

scriptDir=$(dirname "${BASH_SOURCE[0]}")

. "$scriptDir/macos-common.sh"

# The xcode-27 runner image ships Xcode 27 as the default toolchain. Building with
# the iOS 27 SDK is required for the app to resize on iPhone Duo instead of running
# in compatibility mode.
