#!/bin/bash

function errorcheck(){
    if [$? -gt 0]; then
	echo "Error during $1"
	quit
    else
	echo "$1 successful"
    fi
}

svn co https://vcmi.svn.sourceforge.net/svnroot/vcmi/branches/programmingChallenge/ vcmi
errorcheck "fetching sources"
cd vcmi
autoreconf -i
errorcheck "autoreconf -i"
cd ..
vcmi/configure --datadir=`pwd` --bindir=`pwd`vcmi --libdir=`pwd`
errorcheck "configure"
make
errorcheck "make"
unzip vcmipack.zip -d vcmi
errorcheck "pack unzip"
ln -s "vcmi/b1.json"
errorckeck "b1.json symlink"
ln -s "AI/StupidAI/.libs/libStupidAI.so"
errorcheck "StupidAI symlink"
ln -s "Odpalarka/odpalarka"
errorcheck "Odpalarka symlink"
ln -s "VCMI_BattleAiHost/vcmirunner"
errorckeck "runner symlink"
ln -s "server/vcmiserver"
errorckeck "server symlink"
