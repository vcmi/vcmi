project(vcmi)
cmake_minimum_required(VERSION 2.8.12)
# TODO:
# 1) Detection of Qt5 and compilation of launcher, unless explicitly disabled

# where to look for cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_HOME_DIRECTORY}/cmake_modules)

# enable Release mode but only if it was not set
if (NOT CMAKE_BUILD_TYPE)
		set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

# VCMI version
set(VCMI_VERSION_MAJOR 0)
set(VCMI_VERSION_MINOR 98)
set(VCMI_VERSION_PATCH 0)

option(ENABLE_ERM "Enable compilation of ERM scripting module" OFF)
option(ENABLE_EDITOR "Enable compilation of map editor" OFF)
option(ENABLE_LAUNCHER "Enable compilation of launcher" ON)
option(ENABLE_TEST "Enable compilation of unit tests" OFF)
option(ENABLE_PCH "Enable compilation using precompiled headers" ON)

############################################
#        Documentation section             #
############################################

include(UseDoxygen OPTIONAL)

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
	set(CMAKE_XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks @executable_path/")

	# Build with clang ang libc++
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")

	# On OS X we use Sparkle framework for updates
	find_path(SPARKLE_INCLUDE_DIR Sparkle.h)
	find_library(SPARKLE_FRAMEWORK NAMES Sparkle)

	# Xcode 5.0 fix
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftemplate-depth=256")

	# Link with iconv
	set(SYSTEM_LIBS ${SYSTEM_LIBS} libiconv.dylib)
endif()

if (WIN32)
	add_definitions(-DBOOST_THREAD_USE_LIB)
	add_definitions(-D_WIN32_WINNT=0x0501)
	set(SYSTEM_LIBS ${SYSTEM_LIBS} ole32 oleaut32 ws2_32 mswsock dbghelp)

	#delete lib prefix for dlls (libvcmi -> vcmi)
	set(CMAKE_SHARED_LIBRARY_PREFIX "") 

	if(MINGW)
		#MinGW: check for iconv (may be needed for boost.locale)
		include(CheckLibraryExists)
		check_library_exists(iconv libiconv_open "" ICONV_FOUND)
		if(ICONV_FOUND)
			set(SYSTEM_LIBS ${SYSTEM_LIBS} iconv)
		endif()

		#MinGW: copy runtime to VCMI location
		get_filename_component(MINGW_BIN_PATH ${CMAKE_CXX_COMPILER} PATH )
		set(dep_files ${dep_files} "${MINGW_BIN_PATH}/libwinpthread-*.dll") 
		set(dep_files ${dep_files} "${MINGW_BIN_PATH}/libgcc_s_*.dll") 
		set(dep_files ${dep_files} "${MINGW_BIN_PATH}/libstdc++-*.dll")

		#MinGW: use O1 to prevent compiler crash in some cases
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O1")
	endif()

	if(MSVC)
		#MSVC: Fix problems with linking
		add_definitions(-DBOOST_ALL_NO_LIB)
		set(Boost_USE_STATIC_LIBS ON)

		#MSVC: Don't link with SDLMain
		set(SDL2_BUILDING_LIBRARY ON)

		#MSVC: Suppress warnings
		add_definitions(-D_CRT_SECURE_NO_WARNINGS)
		add_definitions(-D_SCL_SECURE_NO_WARNINGS)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /wd4251")
	endif()
endif()

if(NOT WIN32)
	INCLUDE(CheckLibraryExists)

	#check if some platform-specific libraries are needed for linking
	CHECK_LIBRARY_EXISTS(rt shm_open "" HAVE_RT_LIB)
	if(HAVE_RT_LIB)
		set(SYSTEM_LIBS ${SYSTEM_LIBS} rt)
	endif()
endif()

set(SYSTEM_LIBS ${SYSTEM_LIBS} ${CMAKE_DL_LIBS})

set(FFmpeg_FIND_COMPONENTS AVFORMAT SWSCALE)
find_package(Boost 1.48.0 COMPONENTS date_time filesystem locale program_options system thread REQUIRED)
find_package(ZLIB REQUIRED)
find_package(FFmpeg REQUIRED)
find_package(Minizip)
if (MINIZIP_FOUND)
    add_definitions(-DUSE_SYSTEM_MINIZIP)
endif()

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_ttf REQUIRED)

set(SDL_INCLUDE_DIR "${SDL2_INCLUDE_DIR}")
set(SDLTTF_INCLUDE_DIR "${SDL2_TTF_INCLUDE_DIR}")
set(SDLIMAGE_INCLUDE_DIR "${SDL2_IMAGE_INCLUDE_DIR}")
set(SDLMIXER_INCLUDE_DIR "${SDL2_MIXER_INCLUDE_DIR}")
set(SDL_LIBRARY "${SDL2_LIBRARY}")
set(SDLTTF_LIBRARY "${SDL2_TTF_LIBRARY}")
set(SDLIMAGE_LIBRARY "${SDL2_IMAGE_LIBRARY}")
set(SDLMIXER_LIBRARY "${SDL2_MIXER_LIBRARY}")

include(cotire)

if (ENABLE_EDITOR OR ENABLE_LAUNCHER)
	# Widgets finds its own dependencies (QtGui and QtCore).
	find_package(Qt5Widgets REQUIRED)
endif()

if (ENABLE_LAUNCHER)
	find_package(Qt5Network REQUIRED)
endif()

if(ENABLE_TEST)
	# find_package overwrites BOOST_* variables which are already set, so all components have to be included again
	find_package(Boost 1.48.0 COMPONENTS date_time program_options filesystem system thread locale unit_test_framework REQUIRED)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR NOT WIN32) #so far all *nix compilers support such parameters
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(CLANG_SPECIFIC_FLAGS "-Wno-mismatched-tags -Wno-unknown-warning-option")
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -Wextra -Wpointer-arith -Wno-strict-aliasing -Wno-switch -Wno-sign-compare -Wno-unused-local-typedefs  -Wno-unused-parameter -Wuninitialized -Wno-overloaded-virtual -Wno-type-limits -Wno-unknown-pragmas -Wno-reorder ${CLANG_SPECIFIC_FLAGS}")

	if(UNIX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
	endif()

endif()

if(WIN32) # on Win everything goes into H3 root directory
	set(BIN_DIR "." CACHE STRING "Where to install binaries")
	set(LIB_DIR "." CACHE STRING "Where to install main library")
	set(DATA_DIR "." CACHE STRING "Where to install data files")
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

# precompiled header configuration
SET(PCH_PROPERTIES
	COTIRE_ENABLE_PRECOMPILED_HEADER ${ENABLE_PCH}
	COTIRE_ADD_UNITY_BUILD FALSE 
	COTIRE_CXX_PREFIX_HEADER_INIT "StdInc.h"
)

if (ENABLE_ERM)
		add_subdirectory(scripting/erm)
endif()
if (NOT MINIZIP_FOUND)
	add_subdirectory(lib/minizip)
	set(MINIZIP_LIBRARIES minizip)
endif()
add_subdirectory(lib)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory(AI)
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

	install(FILES vcmibuilder DESTINATION ${BIN_DIR} PERMISSIONS
		OWNER_WRITE OWNER_READ OWNER_EXECUTE
		            GROUP_READ GROUP_EXECUTE
		            WORLD_READ WORLD_EXECUTE)
endif()

if(WIN32)
	file(GLOB dep_files 
		${dep_files} 
		"${CMAKE_FIND_ROOT_PATH}/bin/*.dll")

	#Copy debug versions of libraries if build type is debug. Doesn't work in MSVC!
	if(CMAKE_BUILD_TYPE MATCHES DEBUG)
		set(debug_postfix d)
	endif(CMAKE_BUILD_TYPE MATCHES DEBUG)
	
	if (ENABLE_EDITOR OR ENABLE_LAUNCHER)
		get_target_property(QtCore_location Qt5::Core LOCATION)
		get_filename_component(Qtbin_folder ${QtCore_location} PATH)
		file(GLOB dep_files 
			${dep_files}
			${Qtbin_folder}/Qt5Core${debug_postfix}.dll
			${Qtbin_folder}/Qt5Gui${debug_postfix}.dll
			${Qtbin_folder}/Qt5Widgets${debug_postfix}.dll
			${Qtbin_folder}/icu*.dll)
		file(GLOB dep_qwindows
			${Qtbin_folder}/../plugins/platforms/qwindows${debug_postfix}.dll)
		if(MSVC)
			file(GLOB dep_files 
				${dep_files}
				${Qtbin_folder}/libEGL.dll
				${Qtbin_folder}/libGLESv2.dll)
		endif()
	endif()

	if (ENABLE_LAUNCHER)
		file(GLOB dep_files 
			${dep_files} 
			${Qtbin_folder}/Qt5Network${debug_postfix}.dll)
	endif()

	if(MSVC)
		#install MSVC runtime
		include(InstallRequiredSystemLibraries)
		install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION ${BIN_DIR})
	endif()
	install(FILES ${dep_files} DESTINATION ${BIN_DIR})
	install(FILES ${dep_qwindows} DESTINATION ${BIN_DIR}/platforms)
elseif(NOT APPLE)
	#install icons and desktop file on Linux
	#FIXME: move to client makefile?
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.64x64.png"   DESTINATION share/icons/hicolor/64x64/apps RENAME vcmiclient.png)
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.48x48.png"   DESTINATION share/icons/hicolor/48x48/apps RENAME vcmiclient.png)
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.32x32.png"   DESTINATION share/icons/hicolor/32x32/apps RENAME vcmiclient.png)
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.256x256.png" DESTINATION share/icons/hicolor/256x256/apps RENAME vcmiclient.png)
	install(FILES "${CMAKE_SOURCE_DIR}/client/icons/vcmiclient.desktop"     DESTINATION share/applications)
	if (ENABLE_LAUNCHER) #FIXME: move to launcher makefile?
		install(FILES "${CMAKE_SOURCE_DIR}/launcher/vcmilauncher.desktop"     DESTINATION share/applications)
	endif()
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
	set(CPACK_MONOLITHIC_INSTALL 1)
	set(CPACK_PACKAGE_NAME "VCMI")
	set(CPACK_PACKAGE_VENDOR "VCMI team")
	set(CPACK_PACKAGE_FILE_NAME "vcmi-${CPACK_PACKAGE_VERSION}-win32")
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/license.txt")
	set(CPACK_PACKAGE_EXECUTABLES "VCMI_launcher;VCMI")
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")
	set(CPACK_NSIS_PACKAGE_NAME "VCMI ${CPACK_PACKAGE_VERSION}")
	set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
	set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS " CreateShortCut \\\"$DESKTOP\\\\VCMI.lnk\\\" \\\"$INSTDIR\\\\VCMI_launcher.exe\\\"")
    set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS " Delete \\\"$DESKTOP\\\\VCMI.lnk\\\" ")
	
	configure_file("${CMAKE_SOURCE_DIR}/cmake_modules/CMakeCPackOptions.cmake.in" "${CMAKE_BINARY_DIR}/CMakeCPackOptions.cmake" @ONLY)
	set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_BINARY_DIR}/CMakeCPackOptions.cmake")
elseif(APPLE)
	set(CPACK_GENERATOR DragNDrop)
	set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/osx/dmg_background.png")
	set(CPACK_DMG_DS_STORE "${CMAKE_SOURCE_DIR}/osx/dmg_DS_Store")
else()
	set(CPACK_GENERATOR TGZ)
endif()

INCLUDE(CPack)

