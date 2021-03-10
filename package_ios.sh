#!/usr/bin/env bash

pushd bin/Debug
mkdir -p vcmiclient.app/Frameworks

productsDir=$(pwd)
sdl2Path=~/dev/ios/vcmi-ios-deps/SDL2-lib/lib
for app in vcmiclient vcmiserver; do
  install_name_tool -rpath "$sdl2Path" '@executable_path/Frameworks' "$productsDir/$app.app/$app"
done

cp *.dylib AI/*.dylib "$sdl2Path/libSDL2.dylib" vcmiclient.app
cd vcmiclient.app

for b in vcmiclient *.dylib; do
  for l in minizip vcmi; do
    libName="lib${l}.dylib"
    install_name_tool -change "$productsDir/$libName" "@rpath/$libName" "$b"
  done
  if [ "$b" != vcmiclient ]; then
    install_name_tool -id "@rpath/$b" "$b"
  fi
done

mv -f *.dylib Frameworks
popd

cp -R bin/Debug-iphoneos/* "$productsDir/vcmiclient.app"
cp -fR "$productsDir/vcmiclient.app/Frameworks" "$productsDir/vcmiserver.app"

for l in minizip vcmi; do
  libName="lib${l}.dylib"
  install_name_tool -change "$productsDir/$libName" "@rpath/$libName" "$productsDir/vcmiserver.app/vcmiserver"
done
