#.rst:
# FindMINIZIP
# --------
#
# Locate Minizip library
#
# This module defines
#
# ::
#
#   minizip_LIBRARY, the name of the library to link against
#   minizip_FOUND, if false, do not try to link to Minizip
#   minizip_INCLUDE_DIR, where to find unzip.h
#   minizip_VERSION_STRING, human-readable string containing the version of Minizip
#
#=============================================================================
# Copyright 2003-2009 Kitware, Inc.
# Copyright 2012 Benjamin Eikel
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file kitware license.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(VC_LIB_PATH_SUFFIX lib/x64)
else()
  set(VC_LIB_PATH_SUFFIX lib/x86)
endif()

if (NOT WIN32 AND NOT APPLE_IOS)
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(_MINIZIP minizip)
        set(minizip_VERSION_STRING ${_minizip_VERSION})
    endif ()
endif ()

find_path(minizip_INCLUDE_DIR 
    minizip/unzip.h
  HINTS
    ${_minizip_INCLUDEDIR}
    ENV MINIZIPDIR
  PATH_SUFFIXES 
    MINIZIP
    include
)

find_library(minizip_LIBRARY
  NAMES 
    minizip
  HINTS
    ${_minizip_LIBDIR}
    ENV MINIZIPDIR
  PATH_SUFFIXES 
    lib 
    ${VC_LIB_PATH_SUFFIX}
)

set(minizip_LIBRARIES ${minizip_LIBRARY})
set(minizip_INCLUDE_DIRS ${minizip_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(minizip
                                  REQUIRED_VARS minizip_LIBRARY minizip_INCLUDE_DIR
                                  VERSION_VAR minizip_VERSION_STRING)

if (NOT TARGET "minizip::minizip" AND minizip_FOUND)
	add_library(minizip::minizip UNKNOWN IMPORTED)
	set_target_properties(minizip::minizip PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${minizip_INCLUDE_DIR}")
	set_target_properties(minizip::minizip PROPERTIES
		IMPORTED_LOCATION "${minizip_LIBRARY}")
endif()