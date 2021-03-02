#!/usr/bin/env bash

pushd bin/Debug
mkdir -p vcmiclient.app/Frameworks
cp *.dylib AI/*.dylib vcmiclient.app
badInstallNamePrefix=$(pwd)
cd vcmiclient.app

sdl2Path=~/dev/ios/vcmi-ios-deps/SDL2-lib/lib
cp "$sdl2Path/libSDL2.dylib" .
install_name_tool -rpath "$sdl2Path" '@executable_path/Frameworks' vcmiclient

for b in vcmiclient *.dylib; do
  for l in minizip vcmi; do
    libName="lib${l}.dylib"
    install_name_tool -change "$badInstallNamePrefix/$libName" "@rpath/$libName" "$b"
  done
  if [ "$b" != vcmiclient ]; then
    install_name_tool -id "@rpath/$b" "$b"
  fi
done

mv -f *.dylib Frameworks
popd

cp -f ../vcmi/Info.plist "$badInstallNamePrefix/vcmiclient.app"
cp -R bin/Debug-iphoneos/* "$badInstallNamePrefix/vcmiclient.app"
