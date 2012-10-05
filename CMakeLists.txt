project(vcmi)
cmake_minimum_required(VERSION 2.6)

# where to look for cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_HOME_DIRECTORY}/cmake_modules)

# enable Release mode but only if it was not set
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

option(DISABLE_ERM "Disable compilation of ERM scripting module" OFF)

############################################
#        Building section                  #
############################################

find_package(Boost 1.46.0 COMPONENTS program_options filesystem system thread REQUIRED)
find_package(SDL REQUIRED)
find_package(SDL_image REQUIRED)
find_package(SDL_mixer REQUIRED)
find_package(SDL_ttf REQUIRED)
find_package(ZLIB REQUIRED)

if(NOT WIN32)
	find_package(FFMPEG_swscale REQUIRED)

	INCLUDE(CheckLibraryExists)

	#check if some platform-specific libraries are needed for linking client and server
	CHECK_LIBRARY_EXISTS(rt shm_open "" HAVE_RT_LIB)
	if(HAVE_RT_LIB)
		set(RT_LIB -lrt)
	endif()

	CHECK_LIBRARY_EXISTS(dl dlopen "" HAVE_DL_LIB)
	if(HAVE_DL_LIB)
		set(DL_LIB -ldl)
	endif()
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR NOT WIN32) #so far all *nix compilers support such parameters
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -Wextra -Wpointer-arith -Wno-switch -Wno-sign-compare -Wno-unused-parameter -Wno-overloaded-virtual")
endif()

if(WIN32) # on Win everything goes into H3 root directory
	set(BIN_DIR "" CACHE STRING "Where to install binaries")
	set(LIB_DIR "" CACHE STRING "Where to install main library")
	set(DATA_DIR "" CACHE STRING "Where to install data files")
else()
	set(BIN_DIR "bin" CACHE STRING "Where to install binaries")
	set(LIB_DIR "lib/vcmi" CACHE STRING "Where to install main library")
	set(DATA_DIR "share/vcmi" CACHE STRING "Where to install data files")
endif()

set (AI_LIB_DIR "${LIB_DIR}/AI")
set (SCRIPTING_LIB_DIR "${LIB_DIR}/Scripting")

#define required constants
add_definitions(-DM_DATA_DIR="${CMAKE_INSTALL_PREFIX}/${DATA_DIR}")
add_definitions(-DM_BIN_DIR="${CMAKE_INSTALL_PREFIX}/${BIN_DIR}")
add_definitions(-DM_LIB_DIR="${CMAKE_INSTALL_PREFIX}/${LIB_DIR}")

add_subdirectory(lib)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(AI)
if (NOT DISABLE_ERM)
	add_subdirectory(Scripting/ERM)
endif()

#######################################
#    Installation section             #
#######################################

# copy whole directory but .svn control files and user-specific settings.json
install(DIRECTORY config DESTINATION ${DATA_DIR} PATTERN ".svn" EXCLUDE PATTERN "settings.json" EXCLUDE)
# copy vcmi mod along with all its content
install(DIRECTORY Mods/vcmi DESTINATION ${DATA_DIR}/Mods PATTERN ".svn" EXCLUDE)
# copy only fs.json for WoG
install(FILES Mods/WoG/filesystem.json DESTINATION ${DATA_DIR}/Mods/WoG)

if(WIN32)
	#TODO: install any additional dll's. This version (may be broken) will copy all dll's including H3 ones
	#FILE(GLOB dll_files "${CMAKE_BINARY_DIR}/*.dll")
	#INSTALL(FILES ${dll_files} DESTINATION ${BIN_DIR})
else()
	#install icons and desktop file on Linux
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.64x64.png" DESTINATION share/icons/hicolor/64x64/apps RENAME vcmiclient.png)

	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.48x48.png" DESTINATION share/icons/hicolor/48x48/apps RENAME vcmiclient.png)

	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.32x32.png" DESTINATION share/icons/hicolor/32x32/apps RENAME vcmiclient.png)

	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.desktop" DESTINATION share/applications)

endif()

#######################################
#       Packaging section             #
#######################################

set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 90)
set(CPACK_PACKAGE_VERSION_PATCH 0)

# vcmi does not have "patch version" in version string
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
#TODO: remove version from Global.h and use this one as define?

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)

if(WIN32)
	set(CPACK_GENERATOR ZIP) # just use zip? CPack has some GUI install as well
else()
	set(CPACK_GENERATOR TGZ)
endif()

INCLUDE(CPack)

