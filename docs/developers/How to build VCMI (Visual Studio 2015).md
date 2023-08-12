**Warning!** VCMI project only officially support building with CMake:
[How to build VCMI
(Windows/Vcpkg)](How_to_build_VCMI_(Windows/Vcpkg) "wikilink").

Guide below and VS solution found in source tree might be broken and not
automatically tested. Use at your own risk only if you know what you're
doing.

# Prerequisites

-   Installed Heroes3 (can be bought for $10 at
    [gog.com](http://www.gog.com/en/gamecard/heroes_of_might_and_magic_3_complete_edition/))
-   Optionally, you can have also WoG or ERA installed.
-   IDE:
    -   Microsoft Visual Studio 2015 (Community Edition recommended - it
        is free for non-commercial use). Earlier version support is
        deprecated and project may not compile anymore with them.
-   Git client:
    -   Visual Studio 2015 has built-in Git support. It isn't complete,
        but having most important Git commands integrated in IDE is very
        convienent. Additional GitHub extension for Visual Studio
        [1](https://visualstudio.github.com/) gives few more features
        that may be useful.
    -   Alternatively, you can use another Git client with better Git
        support, for example Sourcetree
        [2](https://www.sourcetreeapp.com/)
-   Libraries used by VCMI - where to get them will be explained later
    in this tutorial
    -   Boost
    -   SDL2
    -   SDL2_image
    -   SDL2_mixer
    -   SDL2_TTF
    -   FFmpeg
    -   zlib
    -   QT 5 (only used to compile VCMI launcher, which does not have to
        be rebuilt to test another VCMI parts)

# Preparing place

## Initial directory structure

Create a directory for VCMI development, eg. C:\VCMI. It will be base
directory for VCMI source code.

It is recommended to avoid non-ascii characters in the path to your VCMI
development folder. The folder should not be write-protected by system.
Good location:

-   C:\VCMI

Bad locations:

-   C:\Users\Michał\VCMI (non-ascii character)
-   C:\Program Files (x86)\VCMI (write protection)

## Obtaining VCMI sources

### Using Sourcetree (version 1.9.6.1)

-   Click "Clone / New" button in upper left part of the program
-   Make sure you are in first tab (Clone Repository). In "Source Path /
    URL:" type following link: <https://github.com/vcmi/vcmi.git>
-   Set "Destination Path" field to directory desired for VCMI
    development and press "Clone" button.

Now you have the local copy of the repository on the disk. It should
appear in the Team Explorer under the "Local Git Repositories" section.
Double click it and then select a solution to open.

# Getting Libraries

This section will show ways to get up-to-date versions of libraries
required to build VCMI. When you have a choice if you want 32 or 64 bit
version of library - choosing 32 bit will make things easier as you will
not have to do additional configuration in Visual Studio to switch to 64
bit etc.

## Boost

-   Header files with extra things can be obtained from boost website
    download: [3](http://www.boost.org/users/download/)
-   To avoid rebuilding boost from sources, you can download prebuilt
    windows binaries (lib files) from there as well. MSVC 14 binaries
    are for Visual Studio 2015.

UPDATE: Somebody reported problem with boost newer than 1.62 when trying
to build VCMI. To get all boost 1.62 required files use these links:
[4](https://sourceforge.net/projects/boost/files/boost/1.62.0/boost_1_62_0.7z/download)
[5](https://sourceforge.net/projects/boost/files/boost-binaries/1.62.0/boost_1_62_0-msvc-14.0-32.exe/download)

## SDL2 and sublibraries

-   You can get SDL2 from [6](http://libsdl.org/download-2.0.php) -
    download development libraries to get lib and header files
-   Similar to SDL2 package you can get SDL2_image from
    [7](https://www.libsdl.org/projects/SDL_image/), SDL2_mixer from
    [8](https://www.libsdl.org/projects/SDL_mixer/), SDL2_ttf from
    [9](https://www.libsdl.org/projects/SDL_ttf/).

## FFmpeg

-   You can get this library from
    [10](https://ffmpeg.zeranoe.com/builds/). Both header and lib files
    are available if you choose "dev" download version.

## zlib

-   most tricky to get - use NuGet package manager that is part of
    Visual Studio 2015 to get it. Link in NuGet gallery:
    [11](https://www.nuget.org/packages/zlib/)

## QT 5

-   (TO BE ADDED)

# Compiling VCMI

To compile VCMI you need to be able to compile some of projects that
VCMI solution provides, more information about them is in next part of
this tutorial. Default build configuration for VCMI solution is RD, what
means "release with debug information".

You need to resolve header and library directories to point to all
required libraries in project settings to properly build VCMI. This is
out of scope of this tutorial.

# VCMI solution details

VCMI solution consists of few separate projects that get compiled to
separate output. Many of them rely on VCMI_lib, so making changes to
VCMI_lib requires recompiling another projects. Some of the projects are
dead or not updated for very long time, for example Editor or ERM.
FuzzyLite and minizip are utility projects and are not meant to be
currently changed. Projects in development and their purpose are:

-   BattleAI - as name says this project is for artificial intelligence
    during battles. Gets compiled to DLL module.
-   VCAI - adventure map artificial intelligence. Gets compiled to DLL
    module.
-   VCMI_client - game client, "game display" and user interface related
    code is here. One of two main game game executables
-   VCMI_launcher - game launcher with ability to change some available
    game options, disable / enable VCMI mods etc.
-   VCMI_lib - contains shared code for client and server. Large part of
    game mechanics can be found here. Compilable to DLL module.
-   VCMI_server - game server. Server starts everytime a scenario is
    launched, and performs network-based communication with game client.
    Some parts of game mechanics can be found here. One of two main game
    executables.

Projects mentioned above need to be compiled "together" to work because
of VCMI_lib dependency - for example compiling VCMI_lib and VCMI_client
requires recompiling server, AI projects etc. because their equivalents
from other sources such as VCMI windows package will not work with new
VCMI_lib and raise "cannot find entry point..." error *(TODO: Maybe this
can be worked around using another project code generation settings)*.
Working launcher is not needed to run the game though, as mentioned in
first part of this tutorial (prerequisites). Additional workaround to
skip compiling launcher and have access to launcher settings is to use
VCMI windows package as separate vcmi copy - changes made in launcher
there affect all vcmi copies.

**There is also Test project for development purposes that can be used
for unit tests and similar things that developers may want to do.**