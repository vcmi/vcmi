# Locate LuaJIT library
# This module defines
#  LUAJIT_FOUND, if false, do not try to link to Lua
#  luajit_INCLUDE_DIR, where to find lua.h
#  luajit_VERSION_STRING, the version of Lua found (since CMake 2.8.8)
#
# This module is similar to FindLua51.cmake except that it finds LuaJit instead.

FIND_PATH(luajit_INCLUDE_DIR luajit.h
	HINTS
	$ENV{luajit_DIR}
	PATH_SUFFIXES include/luajit include/luajit-2.1 include/luajit-2.0 include/luajit-5_1-2.1 include/luajit-5_1-2.0 include
	PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/sw # Fink
	/opt/local # DarwinPorts
	/opt/csw # Blastwave
	/opt
)

FIND_LIBRARY(luajit_LIBRARY
	NAMES luajit-5.1 lua51
	HINTS
	$ENV{luajit_DIR}
	PATH_SUFFIXES lib64 lib
	PATHS
	~/Library/Frameworks
	/Library/Frameworks
	/sw
	/opt/local
	/opt/csw
	/opt
)

IF(luajit_INCLUDE_DIR AND EXISTS "${luajit_INCLUDE_DIR}/luajit.h")
	FILE(STRINGS "${luajit_INCLUDE_DIR}/luajit.h" luajit_version_str REGEX "^#define[ \t]+luajit_RELEASE[ \t]+\"LuaJIT .+\"")

	STRING(REGEX REPLACE "^#define[ \t]+luajit_RELEASE[ \t]+\"LuaJIT ([^\"]+)\".*" "\\1" luajit_VERSION_STRING "${luajit_version_str}")
	UNSET(luajit_version_str)
ENDIF()

INCLUDE(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LUAJIT_FOUND to TRUE if
# all listed variables are TRUE
FIND_PACKAGE_HANDLE_STANDARD_ARGS(luajit
	REQUIRED_VARS luajit_LIBRARY luajit_INCLUDE_DIR
	VERSION_VAR luajit_VERSION_STRING)

if (NOT TARGET "luajit::luajit" AND luajit_FOUND)
	add_library(luajit::luajit UNKNOWN IMPORTED)
	set_target_properties(luajit::luajit PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${luajit_INCLUDE_DIR}")
	set_target_properties(luajit::luajit PROPERTIES
		IMPORTED_LOCATION "${luajit_LIBRARY}")
endif()

MARK_AS_ADVANCED(luajit_INCLUDE_DIR luajit_LIBRARY)
