# Introduction to Qt Creator

Qt Creator is the recommended IDE for VCMI development on Linux distributions, but it may be used on other operating systems as well. It has the following advantages compared to other IDEs:

- Fast parser/indexer, stable.
- Almost no manual configuration when used with CMake. Project configuration is read from CMake text files,
- Easy to setup and use with multiple different compiler toolchains: GCC, Visual Studio, Clang

You can install Qt Creator from repository, but better to stick to latest version from Qt website: https://www.qt.io/download-open-source/

## Configuration

To open the project you have to click File -\> Open file or project... -\> Select /path/to/vcmi/src/CMakeLists.txt.

For the first time and for every CMake project configuration change you have to execute CMake. This step can be done when opening the project for the first time or alternatively via the left bar -\> Projects -\> Build Settings -\> Execute CMake. You have to specify CMake arguments and the build dir. CMake arguments can be the following: `-DCMAKE_BUILD_TYPE=Debug`

The build dir should be set to something like /trunk/build for the debug build and /trunk/buildrel for the release build. For cleaning the build dir a command like "make clean" may be not enough. Better way is to delete the build dir, re-create it and re-execute CMake. Steps for cleaning can be configured in the Projects tab as well.

## Debugging

There is a problem with QtCreator when debugging both vcmiclient and vcmiserver. If you debug the vcmiclient, start a game, attach the vcmiserver process to the gdb debugger(Debug \> Start Debugging \> Attach to Running External Application...) then breakpoints which are set for vcmiserver will be ignored. This looks like a bug, in any case it's not intuitively. Two workarounds are available luckily:

1. Run vcmiclient (no debug mode), then attach server process to the debugger
2. Open two instances of QtCreator and debug vcmiserver and vcmiclient separately(it works!)