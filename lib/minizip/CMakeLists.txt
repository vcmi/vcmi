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
target_link_libraries(minizip ${ZLIB_LIBRARIES})

install(TARGETS minizip DESTINATION ${LIB_DIR})
