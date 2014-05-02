if [ -f ../.osx_dependencies_installed ];
then
   echo "OS X prebuilt dependencies are already installled"
else
	curl -o ../xcode-pack.zip -L https://dl.dropboxusercontent.com/u/1777581/xcode-pack.zip
	unzip ../xcode-pack.zip -d ../
	touch ../.osx_dependencies_installed
fi