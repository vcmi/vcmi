# Clean previous build
rm -rf build
rm -rf bin
rm -rf osx/osx-vcmibuilder/build
rm -rf osx/vcmibuilder.app

# Build vcmibuilder
xcodebuild -project osx/osx-vcmibuilder/vcmibuilder.xcodeproj/ -configuration Release
mv osx/osx-vcmibuilder/build/Release/vcmibuilder.app osx/vcmibuilder.app

# Build vcmi
mkdir build
cd build
cmake -G Xcode .. -DENABLE_LAUNCHER=OFF
xcodebuild -project vcmi.xcodeproj/ -configuration Release -target package