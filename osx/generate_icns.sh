#!/usr/bin/env bash

iconset='vcmi.iconset'
mkdir "$iconset"

cd "$iconset"
for multiplier in 1 2 ; do
	if [[ $multiplier != 1 ]] ; then
		suffix="@${multiplier}x"
	fi
	for size in 16 32 128 256 512 ; do
		realSize=$(( $size * $multiplier ))
		ln "../../clientapp/icons/vcmiclient.${realSize}x${realSize}.png" "icon_${size}x${size}${suffix}.png"
	done
done
cd ..

iconutil -c icns "$iconset"
rm -rf "$iconset"
