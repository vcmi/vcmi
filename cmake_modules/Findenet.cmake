# - Try to find enet
# Once done this will define
#
#  ENET_FOUND - system has enet
#  ENET_INCLUDE_DIRS - the enet include directory
#  ENET_LIBRARIES - the libraries needed to use enet
#
# $ENETDIR is an environment variable used for finding enet.
#
#  Borrowed from The Mana World
#  http://themanaworld.org/
#
# Several changes and additions by Fabian 'x3n' Landau
# Lots of simplifications by Adrian Friedli
#                 > www.orxonox.net <

FIND_PATH(ENET_INCLUDE_DIRS enet/enet.h
    PATHS
    $ENV{ENETDIR}
    /usr/local
    /usr
    PATH_SUFFIXES include
)

FIND_LIBRARY(ENET_LIBRARY
    NAMES enet
    PATHS
    $ENV{ENETDIR}
    /usr/local
    /usr
    PATH_SUFFIXES lib
)

# handle the QUIETLY and REQUIRED arguments and set ENET_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ENet DEFAULT_MSG ENET_LIBRARY ENET_INCLUDE_DIRS)

IF (ENET_FOUND)
    IF(WIN32)
        SET(WINDOWS_ENET_DEPENDENCIES "ws2_32;winmm")
        SET(ENET_LIBRARIES ${ENET_LIBRARY} ${WINDOWS_ENET_DEPENDENCIES})
    ELSE(WIN32)
        SET(ENET_LIBRARIES ${ENET_LIBRARY})
    ENDIF(WIN32)
ENDIF (ENET_FOUND)

MARK_AS_ADVANCED(ENET_LIBRARY ENET_LIBRARIES ENET_INCLUDE_DIRS)