#!/usr/bin/env bash

cd "$CODESIGNING_FOLDER_PATH/Frameworks"
tbbFilename=$(otool -L libNullkiller.dylib | egrep --only-matching 'libtbb\S+')
if [[ -L "$tbbFilename" ]]; then
	mv -f "$(readlink "$tbbFilename")" "$tbbFilename"
fi
