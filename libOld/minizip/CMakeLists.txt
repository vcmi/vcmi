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
if(MSVC)
    set_target_properties(minizip PROPERTIES COMPILE_DEFINITIONS "MINIZIP_DLL;ZLIB_DLL;ZLIB_INTERNAL")
elseif(APPLE)
    set_target_properties(minizip PROPERTIES XCODE_ATTRIBUTE_LD_DYLIB_INSTALL_NAME "@rpath/libminizip.dylib")
endif()

target_link_libraries(minizip ${ZLIB_LIBRARIES})

if (NOT APPLE) # Already inside vcmiclient bundle
    install(TARGETS minizip RUNTIME DESTINATION ${LIB_DIR} LIBRARY DESTINATION ${LIB_DIR})
endif()
