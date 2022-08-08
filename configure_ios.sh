#!/usr/bin/env bash

prefixPath=~/dev/vcmi/vcmi-ios-depends/build/iphoneos
if [[ "$1" ]]; then
	prefixPath=~/dev/vcmi/vcmi-ios-depends/build/iphonesimulator
fi

# -DCMAKE_OSX_SYSROOT=iphonesimulator

cmake ../vcmi -G Xcode \
  -DCMAKE_SYSTEM_NAME=iOS \
  -DFORCE_BUNDLED_MINIZIP=ON \
  -DBUNDLE_IDENTIFIER_PREFIX=com.kambala \
  -Wno-dev \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  -DCMAKE_PREFIX_PATH="$prefixPath" \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY='Apple Development' \
  -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM='4XHN44TEVG'
  # -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
