#!/usr/bin/env bash

platform=OS64
globalPrefix=~/dev/vcmi/vcmi-ios-depends/build/iphoneos
qtDir=~/dev/Qt-libs/5.15.5/ios10-widgets
if [[ "$1" ]]; then
	platform=SIMULATOR64
	globalPrefix=~/dev/vcmi/vcmi-ios-depends/build/iphonesimulator
fi
prefixPath="$globalPrefix;$qtDir"

# prefixPath="$boostPrefix;$sdlLibsDir"
# xcodeMajorVersion=$(xcodebuild -version | fgrep Xcode | cut -d ' ' -f 2 | cut -d . -f 1)
# if [[ $xcodeMajorVersion -ge 12 ]]; then
#   extraVars=-DCMAKE_FRAMEWORK_PATH=~/dev/ios/vcmi-ios-deps/mobile-ffmpeg-min-gpl-4.4-xc12-frameworks
# else
#   prefixPath+=;~/dev/ios/vcmi-ios-deps/mobile-ffmpeg-min-universal
# fi

srcDir="../vcmi"
# cmake "$srcDir" -G Xcode -T buildsystem=1 \
cmake "$srcDir" -G Xcode \
  -DFORCE_BUNDLED_MINIZIP=ON \
  -DBUNDLE_IDENTIFIER_PREFIX=com.kambala \
  -Wno-dev \
  -DCMAKE_CXX_COMPILER_LAUNCHER=ccache \
  --toolchain "$srcDir/ios.toolchain.cmake" \
  -DPLATFORM=$platform \
  -DDEPLOYMENT_TARGET=12.0 \
  -DENABLE_BITCODE=OFF \
  -DCMAKE_BINARY_DIR=$(pwd) \
  -DCMAKE_PREFIX_PATH="$prefixPath" \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY='Apple Development' \
  -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM='4XHN44TEVG'
  # -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
