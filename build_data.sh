#!/bin/sh

# Copyright (c) 2009,2010 Frank Zago
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.


# Extract game data from various archives and create a tree with the right files

# Tools needed:
#   unshield: sudo apt-get install unshield
#   rename (from perl)
#   unrar: sudo apt-get install unrar

# Data files needed:
#   data1.cab and data1.hdr from the original CDROM
#   the WoG release v3.58f: allinone_358f.zip
#   the VCMI distribution: vcmi_082.zip

# Usage: put this script and the 4 data files into the same directory
# and run the script.


DESTDIR=`pwd`/vcmi
rm -rf $DESTDIR
mkdir $DESTDIR

# Extract Data from original CD-ROM
# 376917499 2002-06-14 14:32 _setup/data1.cab
# 27308     2002-06-14 14:32 _setup/data1.hdr

rm -rf temp
mkdir temp
cd temp

echo Extracting data1.cab
unshield x ../data1.cab || exit 1
echo Done extracting data1.cab

rm -rf CommonFiles
rm -rf GameUpdate
rm -rf Support
rm -rf Program_Files/mplayer
rm -rf Program_Files/ONLINE

find . -name "*.DLL" | xargs rm -f
find . -name "*.dll" | xargs rm -f
find . -name "*.pdf" -print0  | xargs -0 rm -f
find . -name "*.HLP" | xargs rm -f
find . -name "*.TXT" | xargs rm -f
find . -name "*.cnt" | xargs rm -f
find . -name "*.exe" | xargs rm -f
find . -name "*.EXE" | xargs rm -f
find . -name "*.ASI" | xargs rm -f

# Tree is clean. Move extracted files to their final destination
mv Program_Files/* $DESTDIR
cd ..


# Extract Data from WoG
# 39753248 allinone_358f.zip

rm -rf temp
mkdir temp
cd temp

unzip ../allinone_358f.zip
cd WoG_Install
mkdir temp
cd temp
unrar x -o+ ../main1.wog
unrar x -o+ ../main2.wog
#unrar x -o+ ../main3.wog
unrar x -o+ ../main4.wog
#unrar x -o+ ../main5.wog
#unrar x -o+ ../main6_optional.wog
#unrar x -o+ ../main7_optional.wog
#unrar x -o+ ../main8_optional.wog
#unrar x -o+ ../main9_optional.wog

rm -rf picsall Documentation Data data update
rm -f action.txt h3bitmap.txt H3sprite.txt InstMult.txt
rm -f ACTION.TXT H3BITMAP.TXT H3SPRITE.TXT INSTMULT.TXT
rm -f *.DLL *.dll *.fnt readme.txt *.exe *.EXE *.lod *.vid h3ab_ahd.snd

rename 'y/a-z/A-Z/' *

# Tree is clean. Move extracted files to their final destination

mv MAPS/* $DESTDIR/Maps
rmdir MAPS
mkdir $DESTDIR/Sprites
mv *.MSK *.DEF $DESTDIR/Sprites
mv * $DESTDIR/Data/

cd ../../..


# Extract Data from VCMI release

rm -rf temp
mkdir temp
cd temp

unzip ../vcmi_082.zip

find . -name "*.dll" | xargs rm -f
find . -name "*.DLL" | xargs rm -f
find . -name "*.exe" | xargs rm -f
rm -rf AI
rm -f AUTHORS ChangeLog license.txt Microsoft.VC90.CRT.manifest
rm -rf MP3
rm -rf Games

mv sprites Sprites

# Tree is clean. Move extracted files to their final destination

cp -a . $DESTDIR

cd ..
rm -rf temp


# Done
echo
echo The complete game data is at $DESTDIR
echo You could move it to /usr/local/share/games/
echo and configure vcmi with:
echo   ./configure --datadir=/usr/local/share/games/ --bindir=\`pwd\` --libdir=\`pwd\`
echo
echo If you want to run from your VCMI tree, create those links
echo at the root of your VCMI tree:
echo   ln -s client/vcmiclient .
echo   ln -s server/vcmiserver .
echo   ln -s AI/GeniusAI/.libs/GeniusAI.so .



