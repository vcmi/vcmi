#.rst:
# FindFuzzylite
# --------
#
# Locate FuzzyLite library
#
#=============================================================================
# Copyright 2003-2009 Kitware, Inc.
# Copyright 2012 Benjamin Eikel
# Copyright 2014 Mikhail Paulyshka
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

find_path(fuzzylite_INCLUDE_DIR 
    fl/fuzzylite.h
  HINTS
    ENV FLDIR
  PATH_SUFFIXES
    fl
    include/fl 
    include
)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(VC_LIB_PATH_SUFFIX lib/x64)
else()
  set(VC_LIB_PATH_SUFFIX lib/x86)
endif()

find_library(fuzzylite_LIBRARY
  NAMES 
    fuzzylite
  HINTS
    ENV FLDIR
  PATH_SUFFIXES 
    dynamic
    lib
    ${VC_LIB_PATH_SUFFIX}
)

set(fuzzylite_LIBRARIES ${fuzzylite_LIBRARY})
set(fuzzylite_INCLUDE_DIRS ${fuzzylite_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(fuzzylite
                                  REQUIRED_VARS fuzzylite_LIBRARIES fuzzylite_INCLUDE_DIRS)

if (NOT TARGET "fuzzylite::fuzzylite" AND fuzzylite_FOUND)
	add_library(fuzzylite::fuzzylite UNKNOWN IMPORTED)
	set_target_properties(fuzzylite::fuzzylite PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES "${fuzzylite_INCLUDE_DIR}")
	set_target_properties(fuzzylite::fuzzylite PROPERTIES
		IMPORTED_LOCATION "${fuzzylite_LIBRARY}")
endif()

mark_as_advanced(fuzzylite_LIBRARY fuzzylite_INCLUDE_DIR)