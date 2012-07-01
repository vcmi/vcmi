project(vcmi)
cmake_minimum_required(VERSION 2.6)

# where to look for cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_HOME_DIRECTORY}/cmake_modules)

set(CMAKE_BUILD_TYPE Debug)

find_package(Boost COMPONENTS program_options filesystem system thread iostreams REQUIRED)
find_package(SDL REQUIRED)
find_package(SDL_image REQUIRED)
find_package(SDL_mixer REQUIRED)
find_package(SDL_ttf REQUIRED)
find_package(ZLIB REQUIRED)
find_package(FFMPEG_swscale REQUIRED)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-std=c++0x -Wall -Wclobbered -Wempty-body -Wignored-qualifiers -Wmissing-field-initializers -Wsign-compare -Wtype-limits -Wuninitialized")
endif()

add_subdirectory(lib)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(AI)
add_subdirectory(Scripting/ERM)