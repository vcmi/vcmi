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
#   MINIZIP_LIBRARY, the name of the library to link against
#   MINIZIP_FOUND, if false, do not try to link to Minizip
#   MINIZIP_INCLUDE_DIR, where to find unzip.h
#   MINIZIP_VERSION_STRING, human-readable string containing the version of Minizip
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

if (NOT WIN32)
    find_package(PkgConfig)
    if (PKG_CONFIG_FOUND)
        pkg_check_modules(_MINIZIP minizip)
        set(MINIZIP_VERSION_STRING ${_MINIZIP_VERSION})
    endif ()
endif ()

find_path(MINIZIP_INCLUDE_DIR 
    minizip/unzip.h
  HINTS
    ${_MINIZIP_INCLUDEDIR}
    ENV MINIZIPDIR
  PATH_SUFFIXES 
    MINIZIP
    include
)

find_library(MINIZIP_LIBRARY
  NAMES 
    minizip
  HINTS
    ${_MINIZIP_LIBDIR}
    ENV MINIZIPDIR
  PATH_SUFFIXES 
    lib 
    ${VC_LIB_PATH_SUFFIX}
)

set(MINIZIP_LIBRARIES ${MINIZIP_LIBRARY})
set(MINIZIP_INCLUDE_DIRS ${MINIZIP_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(MINIZIP
                                  REQUIRED_VARS MINIZIP_LIBRARY MINIZIP_INCLUDE_DIR
                                  VERSION_VAR MINIZIP_VERSION_STRING)