#!/usr/bin/env bash

if [[ "$PLATFORM_NAME" != "iphoneos" ]]; then
  exit 0
fi

echo 'codesign dylibs'
for lib in $(find "$CODESIGNING_FOLDER_PATH/Frameworks" -iname '*.dylib'); do
  codesign --force --timestamp=none --sign "$EXPANDED_CODE_SIGN_IDENTITY" "$lib"
done
