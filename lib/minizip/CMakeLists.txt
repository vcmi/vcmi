project(minizip)
cmake_minimum_required(VERSION 2.6)

include_directories(${ZLIB_INCLUDE_DIR})

#NOTE: full library consists from several more files
# but right now VCMI does not need any extra functionality
set(lib_SRCS
		unzip.c
		zip.c
		ioapi.c
)

add_library(minizip SHARED ${lib_SRCS})
set_target_properties(minizip PROPERTIES XCODE_ATTRIBUTE_LD_DYLIB_INSTALL_NAME "@executable_path/libminizip.dylib")
target_link_libraries(minizip ${ZLIB_LIBRARIES})

if (NOT APPLE) # Already inside vcmiclient bundle
    install(TARGETS minizip DESTINATION ${LIB_DIR})
endif()
