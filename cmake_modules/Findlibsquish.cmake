# - Find libsquish
#
#  LIBSQUISH_FOUND
#  LIBSQUISH_INCLUDE_DIR
#  LIBSQUISH_LIBRARIES
#
#  Imported target:
#     libsquish::libsquish

find_path(
    LIBSQUISH_INCLUDE_DIR
    squish.h
    PATH_SUFFIXES squish
    PATHS
        /usr/include
        /usr/local/include
)

find_library(
    LIBSQUISH_LIBRARY
    NAMES squish libsquish
    PATHS
        /usr/lib
        /usr/local/lib
    PATH_SUFFIXES
        lib
        lib64
)

set(LIBSQUISH_LIBRARIES ${LIBSQUISH_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
    LibSquish
    REQUIRED_VARS LIBSQUISH_LIBRARY LIBSQUISH_INCLUDE_DIR
)

if (LIBSQUISH_FOUND AND NOT TARGET libsquish::libsquish)
    add_library(libsquish::libsquish UNKNOWN IMPORTED)
    set_target_properties(libsquish::libsquish PROPERTIES
        IMPORTED_LOCATION "${LIBSQUISH_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBSQUISH_INCLUDE_DIR}"
    )
endif()
