project(vcmi)
cmake_minimum_required(VERSION 2.6)
# TODO:
# 1) Detection of system version of minizip and use it instead of local
# 2) Detection of Qt5 and compilation of launcher, unless explicitly disabled

# where to look for cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_HOME_DIRECTORY}/cmake_modules)

# enable Release mode but only if it was not set
if (NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

# VCMI version
set(VCMI_VERSION_MAJOR 0)
set(VCMI_VERSION_MINOR 94)
set(VCMI_VERSION_PATCH 0)

option(ENABLE_ERM "Enable compilation of ERM scripting module" OFF)
option(ENABLE_EDITOR "Enable compilation of map editor" OFF)
option(ENABLE_LAUNCHER "Enable compilation of launcher" OFF)
option(ENABLE_TEST "Enable compilation of unit tests" OFF)

############################################
#        Building section                  #
############################################

if (APPLE)
	# Default location for thirdparty libs
	set(CMAKE_INCLUDE_PATH "../include" "${CMAKE_OSX_SYSROOT}/usr/include")
	set(CMAKE_LIBRARY_PATH "../lib")
	set(CMAKE_FRAMEWORK_PATH "../Frameworks")
	set(BOOST_ROOT "../")

	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin")
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin")
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_HOME_DIRECTORY}/bin")

	set(CMAKE_XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "${CMAKE_HOME_DIRECTORY}/bin/$(CONFIGURATION)")
	set(CMAKE_XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks")

	# Build with clang ang libc++
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")

	# On OS X we use Sparkle framework for updates
	find_path(SPARKLE_INCLUDE_DIR Sparkle.h)
	find_library(SPARKLE_FRAMEWORK NAMES Sparkle)

	# Xcode 5.0 fix
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=256")
endif()

find_package(Boost 1.46.0 COMPONENTS program_options filesystem system thread REQUIRED)
find_package(SDL REQUIRED)
find_package(SDL_image REQUIRED)
find_package(SDL_mixer REQUIRED)
find_package(SDL_ttf REQUIRED)
find_package(ZLIB REQUIRED)

if (ENABLE_EDITOR OR ENABLE_LAUNCHER)
	# Widgets finds its own dependencies (QtGui and QtCore).
	find_package(Qt5Widgets REQUIRED)
endif()

if (ENABLE_LAUNCHER)
	find_package(Qt5Network REQUIRED)
endif()

if(ENABLE_TEST)
	# find_package overwrites BOOST_* variables which are already set, so all components have to be
	# included again
	find_package(Boost 1.46.0 COMPONENTS program_options filesystem system thread unit_test_framework REQUIRED)
endif()

if(NOT WIN32)
	set(FFmpeg_FIND_COMPONENTS AVFORMAT SWSCALE)
	find_package(FFmpeg REQUIRED)

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
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(CLANG_SPECIFIC_FLAGS "-Wno-mismatched-tags")
	endif()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -Wextra -Wpointer-arith -Wno-switch -Wno-sign-compare -Wno-unused-parameter -Wno-overloaded-virtual ${CLANG_SPECIFIC_FLAGS}")
endif()

if(WIN32) # on Win everything goes into H3 root directory
	set(BIN_DIR "" CACHE STRING "Where to install binaries")
	set(LIB_DIR "" CACHE STRING "Where to install main library")
	set(DATA_DIR "" CACHE STRING "Where to install data files")
elseif(APPLE)
	# includes lib path which determines where to install shared libraries (either /lib or /lib64)
	include(GNUInstallDirs)

	set(BIN_DIR "." CACHE STRING "Where to install binaries")
	set(LIB_DIR "." CACHE STRING "Where to install main library")
	set(DATA_DIR "../h3" CACHE STRING "Where to install data files")
else()
	# includes lib path which determines where to install shared libraries (either /lib or /lib64)
	include(GNUInstallDirs)

	if (NOT BIN_DIR)
			set(BIN_DIR "bin" CACHE STRING "Where to install binaries")
	endif()
	if (NOT LIB_DIR)
		set(LIB_DIR "${CMAKE_INSTALL_LIBDIR}/vcmi" CACHE STRING "Where to install main library")
	endif()
	if (NOT DATA_DIR)
		set(DATA_DIR "share/vcmi" CACHE STRING "Where to install data files")
	endif()
endif()

set (AI_LIB_DIR "${LIB_DIR}/AI")
set (SCRIPTING_LIB_DIR "${LIB_DIR}/scripting")

#define required constants
add_definitions(-DM_DATA_DIR="${CMAKE_INSTALL_PREFIX}/${DATA_DIR}")
add_definitions(-DM_BIN_DIR="${CMAKE_INSTALL_PREFIX}/${BIN_DIR}")
add_definitions(-DM_LIB_DIR="${CMAKE_INSTALL_PREFIX}/${LIB_DIR}")

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/vcmi")
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

add_subdirectory(lib)
add_subdirectory(lib/minizip)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(AI)
if (ENABLE_ERM)
	add_subdirectory(scripting/erm)
endif()
if (ENABLE_EDITOR)
	add_subdirectory(editor)
endif()
if (ENABLE_LAUNCHER)
	add_subdirectory(launcher)
endif()
if(ENABLE_TEST)
	add_subdirectory(test)
endif()

#######################################
#    Installation section             #
#######################################

# For apple this files will be already inside vcmiclient bundle
if (NOT APPLE)
	# copy whole directory but .svn control files
	install(DIRECTORY config DESTINATION ${DATA_DIR} PATTERN ".svn" EXCLUDE)
	# copy vcmi mod along with all its content
	install(DIRECTORY Mods/vcmi DESTINATION ${DATA_DIR}/Mods PATTERN ".svn" EXCLUDE)
	# copy only files added by vcmi for WoG
	install(FILES Mods/WoG/mod.json DESTINATION ${DATA_DIR}/Mods/WoG)
	install(DIRECTORY Mods/WoG/config DESTINATION ${DATA_DIR}/Mods/WoG PATTERN ".svn" EXCLUDE)

	install(FILES vcmibuilder DESTINATION ${BIN_DIR} PERMISSIONS
		OWNER_WRITE OWNER_READ OWNER_EXECUTE
		            GROUP_READ GROUP_EXECUTE
		            WORLD_READ WORLD_EXECUTE)
endif()

if(WIN32)
	#TODO: install any additional dll's. This version (may be broken) will copy all dll's including H3 ones
	#FILE(GLOB dll_files "${CMAKE_BINARY_DIR}/*.dll")
	#INSTALL(FILES ${dll_files} DESTINATION ${BIN_DIR})
elseif(APPLE)
else()
	#install icons and desktop file on Linux
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.64x64.png"   DESTINATION share/icons/hicolor/64x64/apps RENAME vcmiclient.png)
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.48x48.png"   DESTINATION share/icons/hicolor/48x48/apps RENAME vcmiclient.png)
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.32x32.png"   DESTINATION share/icons/hicolor/32x32/apps RENAME vcmiclient.png)
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.256x256.png" DESTINATION share/icons/hicolor/256x256/apps RENAME vcmiclient.png)
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.desktop"     DESTINATION share/applications)
endif()

#######################################
#       Packaging section             #
#######################################

set(CPACK_PACKAGE_VERSION_MAJOR ${VCMI_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VCMI_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VCMI_VERSION_PATCH})

# vcmi does not have "patch version" in version string
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
#TODO: remove version from Global.h and use this one as define?

set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)

if(WIN32)
	set(CPACK_GENERATOR ZIP) # just use zip? CPack has some GUI install as well
elseif(APPLE)
	set(CPACK_GENERATOR DragNDrop)
	set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/osx/dmg_background.png")
	set(CPACK_DMG_DS_STORE "${CMAKE_SOURCE_DIR}/osx/dmg_DS_Store")
else()
	set(CPACK_GENERATOR TGZ)
endif()

INCLUDE(CPack)

