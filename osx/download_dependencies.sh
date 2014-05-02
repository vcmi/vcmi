if [ -f ../.osx_dependencies_installed ];
then
   echo "OS X prebuilt dependencies are already installled"
else
	curl -o ../xcode-pack.zip -L http://download.vcmi.eu/xcode-pack.zip
	unzip ../xcode-pack.zip -d ../
	touch ../.osx_dependencies_installed
fi