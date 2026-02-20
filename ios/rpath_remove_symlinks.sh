#!/usr/bin/env bash

# AltStore has issues with symlinks to dylibs

cd "$CODESIGNING_FOLDER_PATH/$BUNDLE_FRAMEWORKS_FOLDER_PATH"

# find rpath entries that point to symlinks
rpathSymlinks=()
for binary in "../$EXECUTABLE_NAME" $(find . -type f -iname '*.dylib'); do
	echo "checking $binary"
	# dyld_info sample output: @rpath/libogg.0.dylib
	for lib in $(dyld_info -linked_dylibs "$binary" | awk -F / '/@rpath/ {print $2}'); do
		if [ -L "$lib" ]; then
			echo "- symlink: $lib"
			rpathSymlinks+=("$lib")
		fi
	done
done

# move real files to symlinks location
echo
for symlink in "${rpathSymlinks[@]}"; do
	mv -fv "$(readlink "$symlink")" "$symlink"
done

# remove the rest of the useless symlinks
find . -type l -delete
