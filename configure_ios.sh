#!/usr/bin/env bash

boostPrefix=~/dev/other/Apple-Boost-BuildScript/build/boost/1.75.0/ios/debug/prefix
ffmpegDir=~/dev/ios/vcmi-ios-deps/mobile-ffmpeg-min-universal
sdlLibsDir=~/dev/ios/vcmi-ios-deps/SDL2-lib

srcDir="../vcmi"
cmake "$srcDir" -G Xcode \
  -DBUNDLE_IDENTIFIER_PREFIX=com.kambala \
  -Wno-dev \
  -DCMAKE_TOOLCHAIN_FILE="$srcDir/ios.toolchain.cmake" \
  -DPLATFORM=${1:-OS64} \
  -DDEPLOYMENT_TARGET=11.0 \
  -DENABLE_BITCODE=0 \
  -DCMAKE_BINARY_DIR=$(pwd) \
  -DCMAKE_PREFIX_PATH="$boostPrefix;$ffmpegDir;$sdlLibsDir" \
  -DSDL2_INCLUDE_DIR=~/dev/ios/vcmi-ios-deps/SDL/include \
  -DSDL2_IMAGE_INCLUDE_DIR=~/dev/ios/vcmi-ios-deps/SDL_image-release-2.0.5 \
  -DSDL2_MIXER_INCLUDE_DIR=~/dev/ios/vcmi-ios-deps/SDL_mixer-release-2.0.4 \
  -DSDL2_TTF_INCLUDE_DIR=~/dev/ios/vcmi-ios-deps/SDL_ttf-release-2.0.15 \
  -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY='Apple Development' \
  -DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM='4XHN44TEVG'
  # -DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=NO
