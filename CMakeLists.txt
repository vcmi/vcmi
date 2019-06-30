# Minimum required version greatly affect CMake behavior
# So cmake_minimum_required must be called before the project()
# 2.8.12 is used since it's present in Ubuntu 14.04 and Cotire require it
cmake_minimum_required(VERSION 2.8.12)

project(VCMI)
# TODO
# macOS:
# - There is problem with running fixup_bundle in main project after subdirectories.
# Cmake put them after all install code of main CMakelists in cmake_install.cmake
# Currently I just added extra add_subdirectory and CMakeLists.txt in osx directory to bypass that.
# - Try to fix build with RPATH.
# Currently if CMAKE_MACOSX_RPATH=1 then AI libs unable to find @rpath/libvcmi.dylib
# I tried to set few different INSTALL_RPATH for all targets in AI directory, but nothing worked.
#
# MXE:
# - Try to implement MXE support into BundleUtilities so we can deploy deps automatically
#
# Vckpg:
# - Improve install code once there is better way to deploy DLLs and Qt plugins
# - Move Vcpkg install BundleUtilities code from osx/CMakeLists.txt
#
# Other:
# - Cleanup remove_directory copy_directory if performance will be a problem.
# We can use some macro over copy_if_different since it's only work for single file.
# - Find a way to move add_custom_command for assets deploy out of "lib/CMakeLists.txt"
# - Consider to remove M_DATA_DIR, DM_BIN_DIR, DM_LIB_DIR and not use them in code as well
# - Try to get rid of FOLDER override with define_property
# It's used currently to make sure that 3rd-party dependencies in git submodules get proper FOLDER property
# - Make FindFuzzyLite check for the right version and disable FORCE_BUNDLED_FL by default

############################################
#        User-provided options             #
############################################

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING
		"Choose the type of build, options are: Debug Release RelWithDebInfo. Default is RelWithDebInfo."
		FORCE)
	set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo)
endif()

set(VCMI_VERSION_MAJOR 0)
set(VCMI_VERSION_MINOR 99)
set(VCMI_VERSION_PATCH 0)

option(ENABLE_ERM "Enable compilation of ERM scripting module" ON)
option(ENABLE_LUA "Enable compilation of LUA scripting module" ON)
option(ENABLE_LAUNCHER "Enable compilation of launcher" ON)
option(ENABLE_TEST "Enable compilation of unit tests" ON)
option(ENABLE_PCH "Enable compilation using precompiled headers" ON)
option(ENABLE_GITVERSION "Enable Version.cpp with Git commit hash" ON)
option(ENABLE_DEBUG_CONSOLE "Enable debug console for Windows builds" ON)
option(ENABLE_MULTI_PROCESS_BUILDS "Enable /MP flag for MSVS solution" ON)

# Used for Snap packages and also useful for debugging
option(ENABLE_MONOLITHIC_INSTALL "Install everything in single directory on Linux and Mac" OFF)

# Allow to pass package name from Travis CI
set(PACKAGE_NAME_SUFFIX "" CACHE STRING "Suffix for CPack package name")
set(PACKAGE_FILE_NAME "" CACHE STRING "Override for CPack package filename")

############################################
#        Miscellaneous options             #
############################################

set(CMAKE_MODULE_PATH ${CMAKE_HOME_DIRECTORY}/cmake_modules)
# Contains custom functions and macros, but don't altering any options
include(VCMIUtils)
vcmi_print_important_variables()

# Options to enable folders in CMake generated projects for Visual Studio, Xcode, etc
# Very useful to put 3rd-party libraries such as Minizip, GoogleTest and FuzzyLite in their own folders
set_property(GLOBAL PROPERTY USE_FOLDERS TRUE)
# Make FOLDER property inheritable
# So when we set FOLDER property on AI directory all and targets inside will inherit it
#
# Important! This trick depend on undefined behavior since we override CMake own property.
# In same time define_property documentation states it's function for custom properties.
define_property(
	TARGET
	PROPERTY FOLDER
	INHERITED
	BRIEF_DOCS "Set the folder name."
	FULL_DOCS  "Use to organize targets in an IDE."
)

# Generate Version.cpp
if(ENABLE_GITVERSION)
	add_custom_target(update_version ALL
		COMMAND ${CMAKE_COMMAND} -DGIT_SHA1="${GIT_SHA1}" -P "${CMAKE_MODULE_PATH}/Version.cmake"
	)
else()
	add_definitions(-DVCMI_NO_EXTRA_VERSION)
endif(ENABLE_GITVERSION)

# Precompiled header configuration
if(ENABLE_PCH)
	include(cotire)
	set(PCH_PROPERTIES
		COTIRE_ENABLE_PRECOMPILED_HEADER ${ENABLE_PCH}
		COTIRE_ADD_UNITY_BUILD FALSE
		COTIRE_CXX_PREFIX_HEADER_INIT "StdInc.h"
	)
else()
	set(PCH_PROPERTIES COTIRE_ADD_UNITY_BUILD FALSE)
	macro(cotire ignore)
	endmacro(cotire)
endif(ENABLE_PCH)

############################################
#        Documentation section             #
############################################

include(UseDoxygen OPTIONAL)

############################################
#        Compile and linking options       #
############################################

if(APPLE)
	set(CMAKE_MACOSX_RPATH 0)
endif(APPLE)

if(WIN32)
	add_definitions(-DBOOST_THREAD_USE_LIB)
	# Windows Vista or newer for FuzzyLite 6 to compile
	add_definitions(-D_WIN32_WINNT=0x0600)

	#delete lib prefix for dlls (libvcmi -> vcmi)
	set(CMAKE_SHARED_LIBRARY_PREFIX "")

	if(MSVC)
		add_definitions(-DBOOST_ALL_NO_LIB)
		add_definitions(-DBOOST_ALL_DYN_LINK)
		set(Boost_USE_STATIC_LIBS OFF)

		# Don't link with SDLMain
		if(ENABLE_DEBUG_CONSOLE)
			set(SDL2_BUILDING_LIBRARY ON)
			add_definitions(-DVCMI_WITH_DEBUG_CONSOLE)
		endif()

		# Suppress warnings
		add_definitions(-D_CRT_SECURE_NO_WARNINGS)
		add_definitions(-D_SCL_SECURE_NO_WARNINGS)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /wd4251")

		if(ENABLE_MULTI_PROCESS_BUILDS)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
		endif()

		# Workaround: Visual Studio has issues with exports of classes that inherit templates
		# https://stackoverflow.com/questions/44960760/msvc-dll-exporting-class-that-inherits-from-template-cause-lnk2005-already-defin
		# Reported to Microsoft here:
		# https://developercommunity.visualstudio.com/content/problem/224597/linker-failing-because-of-multiple-definitions-of.html
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /FORCE:MULTIPLE")

		# Required at least for compatibility with Boost 1.68 on Vcpkg
		set(SYSTEM_LIBS ${SYSTEM_LIBS} bcrypt)
	endif(MSVC)

	if(MINGW)
		set(SYSTEM_LIBS ${SYSTEM_LIBS} ole32 oleaut32 ws2_32 mswsock dbghelp)

		# Check for iconv (may be needed for Boost.Locale)
		include(CheckLibraryExists)
		check_library_exists(iconv libiconv_open "" ICONV_FOUND)
		if(ICONV_FOUND)
			set(SYSTEM_LIBS ${SYSTEM_LIBS} iconv)
		endif()

		# Prevent compiler issues when building Debug
		# Assembler might fail with "too many sections"
		# With big-obj or 64-bit build will take hours
		if(CMAKE_BUILD_TYPE MATCHES Debug)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Og")
		endif()
	endif(MINGW)
endif(WIN32)

if(CMAKE_COMPILER_IS_GNUCXX OR NOT WIN32) #so far all *nix compilers support such parameters
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall -Wextra -Wpointer-arith -Wuninitialized")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-strict-aliasing -Wno-switch -Wno-sign-compare -Wno-unused-local-typedefs")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-parameter -Wno-overloaded-virtual -Wno-type-limits -Wno-unknown-pragmas")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-reorder")

	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-mismatched-tags -Wno-unknown-warning-option -Wno-missing-braces")
	endif()

	if(UNIX)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
		set(SYSTEM_LIBS ${SYSTEM_LIBS} ${CMAKE_DL_LIBS})
	endif()
endif()

# Check if some platform-specific libraries are needed for linking
if(NOT WIN32)
	include(CheckLibraryExists)

	# Shared memory functions used by Boost.Interprocess
	# FindBoost handle linking with pthreads, but doesn't handle this
	CHECK_LIBRARY_EXISTS(rt shm_open "" HAVE_RT_LIB)
	if(HAVE_RT_LIB)
		set(SYSTEM_LIBS ${SYSTEM_LIBS} rt)
	endif()
endif()

############################################
#        Finding packages                  #
############################################

set(FFmpeg_FIND_COMPONENTS AVFORMAT SWSCALE)
find_package(Boost 1.48.0 COMPONENTS date_time filesystem locale program_options system thread REQUIRED)
find_package(ZLIB REQUIRED)
find_package(FFmpeg REQUIRED)
find_package(Minizip)
if(MINIZIP_FOUND)
	add_definitions(-DUSE_SYSTEM_MINIZIP)
endif()

find_package(SDL2 REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_ttf REQUIRED)

if(ENABLE_LAUNCHER)
	# Widgets finds its own dependencies (QtGui and QtCore).
	find_package(Qt5Widgets REQUIRED)
	find_package(Qt5Network REQUIRED)
endif()

if(ENABLE_LUA)
	# MXE paths hardcoded for current dependencies pack - tried and could not make it work another way
	if((MINGW) AND (${CMAKE_CROSSCOMPILING}) AND (DEFINED MSYS))
		set(LUA_INCLUDE_DIR "/usr/lib/mxe/usr/i686-w64-mingw32.static/include/luajit-2.0")
		set(LUA_LIBRARY "/usr/lib/mxe/usr/i686-w64-mingw32.static/lib/libluajit-5.1.a")
	endif()
	find_package(LuaJIT)
	if(LUAJIT_FOUND)
		message(STATUS "Using LuaJIT provided by system")
	else()
		message(STATUS "Cannot find LuaJIT! Fallback to using usual Lua.")
		find_package(Lua REQUIRED)
	endif()
endif()

############################################
#        Output directories                #
############################################

if(WIN32) # on Win everything goes into H3 root directory
	set(BIN_DIR "." CACHE STRING "Where to install binaries")
	set(LIB_DIR "." CACHE STRING "Where to install main library")
	set(DATA_DIR "." CACHE STRING "Where to install data files")
elseif(APPLE)
	# includes lib path which determines where to install shared libraries (either /lib or /lib64)
	include(GNUInstallDirs)

	if(ENABLE_MONOLITHIC_INSTALL)
		set(BIN_DIR "." CACHE STRING "Where to install binaries")
		set(LIB_DIR "." CACHE STRING "Where to install main library")
		set(DATA_DIR "." CACHE STRING "Where to install data files")
	else()
		set(APP_BUNDLE_DIR "${CMAKE_PROJECT_NAME}.app")
		set(APP_BUNDLE_CONTENTS_DIR "${APP_BUNDLE_DIR}/Contents")
		set(APP_BUNDLE_BINARY_DIR "${APP_BUNDLE_CONTENTS_DIR}/MacOS")
		set(APP_BUNDLE_RESOURCES_DIR "${APP_BUNDLE_CONTENTS_DIR}/Resources")

		set(BIN_DIR "${APP_BUNDLE_BINARY_DIR}" CACHE STRING "Where to install binaries")
		set(LIB_DIR "${APP_BUNDLE_BINARY_DIR}" CACHE STRING "Where to install main library")
		set(DATA_DIR "${APP_BUNDLE_RESOURCES_DIR}/Data" CACHE STRING "Where to install data files")
	endif()
else()
	# includes lib path which determines where to install shared libraries (either /lib or /lib64)
	include(GNUInstallDirs)

	if(ENABLE_MONOLITHIC_INSTALL)
		set(CMAKE_INSTALL_RPATH "$ORIGIN/")
		set(BIN_DIR "." CACHE STRING "Where to install binaries")
		set(LIB_DIR "." CACHE STRING "Where to install main library")
		set(DATA_DIR "." CACHE STRING "Where to install data files")
	else()
		if(NOT BIN_DIR)
			set(BIN_DIR "bin" CACHE STRING "Where to install binaries")
		endif()
		if(NOT LIB_DIR)
			set(LIB_DIR "${CMAKE_INSTALL_LIBDIR}/vcmi" CACHE STRING "Where to install main library")
		endif()
		if(NOT DATA_DIR)
			set(DATA_DIR "share/vcmi" CACHE STRING "Where to install data files")
		endif()
	endif()

	# following constants only used for platforms using XDG (Linux, BSD, etc)
	add_definitions(-DM_DATA_DIR="${CMAKE_INSTALL_PREFIX}/${DATA_DIR}")
	add_definitions(-DM_BIN_DIR="${CMAKE_INSTALL_PREFIX}/${BIN_DIR}")
	add_definitions(-DM_LIB_DIR="${CMAKE_INSTALL_PREFIX}/${LIB_DIR}")
endif()

set(AI_LIB_DIR "${LIB_DIR}/AI")
set(SCRIPTING_LIB_DIR "${LIB_DIR}/scripting")

#######################################
#        Add subdirectories           #
#######################################

if(ENABLE_ERM)
	add_subdirectory(scripting/erm)
endif()
if(ENABLE_LUA)
	add_subdirectory(scripting/lua)
endif()
if(NOT MINIZIP_FOUND)
	add_subdirectory_with_folder("3rdparty" lib/minizip)
	set(MINIZIP_LIBRARIES minizip)
endif()
add_subdirectory(lib)
add_subdirectory(client)
add_subdirectory(server)
add_subdirectory_with_folder("AI" AI)
if(ENABLE_LAUNCHER)
	add_subdirectory(launcher)
endif()
if(ENABLE_TEST)
	enable_testing()
	add_subdirectory(test)
endif()

#######################################
#        Installation section         #
#######################################

install(DIRECTORY config DESTINATION ${DATA_DIR})
install(DIRECTORY scripts DESTINATION ${DATA_DIR})
install(DIRECTORY Mods DESTINATION ${DATA_DIR})

# that script is useless for Windows
if(NOT WIN32)
	install(FILES vcmibuilder DESTINATION ${BIN_DIR} PERMISSIONS
		OWNER_WRITE OWNER_READ OWNER_EXECUTE
					GROUP_READ GROUP_EXECUTE
					WORLD_READ WORLD_EXECUTE)
endif()

if(MINGW)
	file(GLOB dep_files
		${dep_files}
		"${CMAKE_FIND_ROOT_PATH}/bin/*.dll")

	if((${CMAKE_CROSSCOMPILING}) AND (DEFINED MSYS))
		message(STATUS "Detected MXE build")
	elseif(CMAKE_BUILD_TYPE MATCHES Debug)
		# Copy debug versions of libraries if build type is debug
		set(debug_postfix d)
	endif()

	if(ENABLE_LAUNCHER)
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
	endif()

	if (ENABLE_LAUNCHER)
		file(GLOB dep_files
			${dep_files}
			${Qtbin_folder}/Qt5Network${debug_postfix}.dll)
	endif()

	install(FILES ${dep_files} DESTINATION ${BIN_DIR})
	install(FILES ${dep_qwindows} DESTINATION ${BIN_DIR}/platforms)
endif(MINGW)

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

if("${PACKAGE_NAME_SUFFIX}" STREQUAL "")
	set(CPACK_PACKAGE_NAME "VCMI")
else()
	set(CPACK_PACKAGE_NAME "VCMI ${PACKAGE_NAME_SUFFIX}")
endif()
if("${PACKAGE_FILE_NAME}" STREQUAL "")
	set(CPACK_PACKAGE_FILE_NAME "vcmi-${CPACK_PACKAGE_VERSION}")
else()
	set(CPACK_PACKAGE_FILE_NAME "${PACKAGE_FILE_NAME}")
endif()
set(CPACK_PACKAGE_VENDOR "VCMI team")

if(WIN32)
	# Note: due to NSI script generation process all of the backward slashes here are useful
	set(CPACK_MONOLITHIC_INSTALL 1)
	set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/license.txt")
	set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CPACK_PACKAGE_NAME}")
	if("${PACKAGE_NAME_SUFFIX}" STREQUAL "")
		set(CPACK_NSIS_PACKAGE_NAME "VCMI ${CPACK_PACKAGE_VERSION}")
	else()
		set(CPACK_NSIS_PACKAGE_NAME "VCMI ${CPACK_PACKAGE_VERSION} ${PACKAGE_NAME_SUFFIX} ")
	endif()
	set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
	if(ENABLE_LAUNCHER)
		set(CPACK_PACKAGE_EXECUTABLES "VCMI_launcher;VCMI")
		set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS " CreateShortCut \\\"$DESKTOP\\\\VCMI.lnk\\\" \\\"$INSTDIR\\\\VCMI_launcher.exe\\\"")
	else()
		set(CPACK_PACKAGE_EXECUTABLES "VCMI_client;VCMI")
		set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS " CreateShortCut \\\"$DESKTOP\\\\VCMI.lnk\\\" \\\"$INSTDIR\\\\VCMI_client.exe\\\"")
	endif()
	set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS " Delete \\\"$DESKTOP\\\\VCMI.lnk\\\" ")

	# set the install/unistall icon used for the installer itself
	# There is a bug in NSI that does not handle full unix paths properly.
	set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/client\\\\vcmi.ico")
	set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/client\\\\vcmi.ico")
	# set the package header icon for MUI
	set(CPACK_PACKAGE_ICON "${CMAKE_SOURCE_DIR}/client\\\\vcmi.ico")

	set(CPACK_NSIS_MENU_LINKS "http://vcmi.eu/" "VCMI Web Site")

	set(CPACK_NSIS_INSTALLED_ICON_NAME "VCMI_client.exe")
	set(CPACK_NSIS_COMPRESSOR "/SOLID lzma")
	set(CPACK_NSIS_DISPLAY_NAME "${CPACK_NSIS_PACKAGE_NAME}, open-source engine for Heroes of Might and Magic III ")
	set(CPACK_NSIS_HELP_LINK "http://vcmi.eu/")
	set(CPACK_NSIS_URL_INFO_ABOUT "http://vcmi.eu/")
	set(CPACK_NSIS_CONTACT @CPACK_PACKAGE_CONTACT@)
	set(CPACK_NSIS_EXECUTABLES_DIRECTORY ".")
	# Use BundleUtilities to fix build when Vcpkg is used and disable it for MXE
	if(NOT (${CMAKE_CROSSCOMPILING}))
		add_subdirectory(osx)
	endif()
elseif(APPLE AND NOT ENABLE_MONOLITHIC_INSTALL)
	set(CPACK_MONOLITHIC_INSTALL 1)
	set(CPACK_GENERATOR "DragNDrop")
	set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_SOURCE_DIR}/osx/dmg_background.png")
	# CMake code for CPACK_DMG_DS_STORE executed before DS_STORE_SETUP_SCRIPT
	# So we can keep both enabled and this shouldn't hurt
	# set(CPACK_DMG_DS_STORE "${CMAKE_SOURCE_DIR}/osx/dmg_DS_Store")
	set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${CMAKE_SOURCE_DIR}/osx/DS_Store_Setup.scpt")

	# Always use "VCMI" as volume name so full path will be /Volumes/VCMI/
	# Otherwise DMG background image wouldn't work
	# Pre-generated DS_Store use absolute path to background image
	set(CPACK_DMG_VOLUME_NAME "${CMAKE_PROJECT_NAME}")

	set(MACOSX_BUNDLE_NAME "${CMAKE_PROJECT_NAME}")
	set(MACOSX_BUNDLE_BUNDLE_NAME "${CMAKE_PROJECT_NAME}")
	if(ENABLE_LAUNCHER)
		set(MACOSX_BUNDLE_EXECUTABLE_NAME "vcmilauncher")
	else()
		set(MACOSX_BUNDLE_EXECUTABLE_NAME "vcmiclient")
	endif()
	set(MACOSX_BUNDLE_ICON_FILE "vcmi.icns")

	install(FILES "${CMAKE_SOURCE_DIR}/osx/vcmi.icns" DESTINATION ${APP_BUNDLE_RESOURCES_DIR})

	configure_file("${CMAKE_SOURCE_DIR}/osx/Info.plist.in" "${CMAKE_BINARY_DIR}/Info.plist")
	install(FILES "${CMAKE_BINARY_DIR}/Info.plist" DESTINATION ${APP_BUNDLE_CONTENTS_DIR})

	# Bundle fixing code must be in separate directory to be executed after all other install code
	add_subdirectory(osx)
else()
	set(CPACK_GENERATOR TGZ)
endif()

include(CPack)
