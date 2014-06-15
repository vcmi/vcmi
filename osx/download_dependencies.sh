if [ -f ../.osx_dependencies_installed ];
then
   echo "OS X prebuilt dependencies are already installled"
else
	# Download and unpack OS X prebuilt dependencies
	curl -o ../xcode-pack.zip -L http://download.vcmi.eu/xcode-pack.zip
	unzip ../xcode-pack.zip -d ../
	rm -rf ../__MACOSX

	# Build vcmibuilder
	xcodebuild -project osx/osx-vcmibuilder/vcmibuilder.xcodeproj/ -configuration Release
	mv osx/osx-vcmibuilder/build/Release/vcmibuilder.app osx/vcmibuilder.app

	touch ../.osx_dependencies_installed
fi