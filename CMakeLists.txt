project(vcmi)
cmake_minimum_required(VERSION 2.6)

#needed?
INCLUDE(CheckLibraryExists)

# where to look for cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_HOME_DIRECTORY}/cmake_modules)

# enable Release mode but only if it was not set
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Boost 1.46.0 COMPONENTS program_options filesystem system thread iostreams REQUIRED)
find_package(SDL REQUIRED)
find_package(SDL_image REQUIRED)
find_package(SDL_mixer REQUIRED)
find_package(SDL_ttf REQUIRED)
find_package(ZLIB REQUIRED)
find_package(FFMPEG_swscale REQUIRED)

#check if some platform-specific libraries are needed for linking client and server
CHECK_LIBRARY_EXISTS(rt shm_open "" HAVE_RT_LIB)
if(HAVE_RT_LIB)
    set(RT_LIB -lrt)
endif()

CHECK_LIBRARY_EXISTS(dl dlopen "" HAVE_DL_LIB)
if(HAVE_DL_LIB)
    set(DL_LIB -ldl)
endif()

if(CMAKE_CXX_COMPILER MATCHES ".*clang")
    set(CMAKE_COMPILER_IS_CLANGXX ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
    set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -Wextra -Wpointer-arith -Wno-switch -Wno-sign-compare -Wno-unused-parameter -Wno-overloaded-virtual")
endif()

#define required constants
add_definitions(-DM_DATA_DIR="${CMAKE_INSTALL_PREFIX}/share/vcmi")
add_definitions(-DM_BIN_DIR="${CMAKE_INSTALL_PREFIX}/bin")
add_definitions(-DM_LIB_DIR="${CMAKE_INSTALL_PREFIX}/lib/vcmi")

add_subdirectory(lib)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(AI)
add_subdirectory(Scripting/ERM)

