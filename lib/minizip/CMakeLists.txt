project(minizip)

include_directories(${ZLIB_INCLUDE_DIR})

#NOTE: full library consists from several more files
# but right now VCMI does not need any extra functionality
set(lib_SRCS
		unzip.c
		zip.c
		ioapi.c
)

if(EMSCRIPTEN)
	add_library(minizip STATIC ${lib_SRCS})
else()
	add_library(minizip SHARED ${lib_SRCS})
endif()
if(MSVC)
    set_target_properties(minizip PROPERTIES COMPILE_DEFINITIONS "MINIZIP_DLL;ZLIB_DLL;ZLIB_INTERNAL")
endif()

vcmi_set_output_dir(minizip "")

target_link_libraries(minizip ${ZLIB_LIBRARIES})

install(TARGETS minizip RUNTIME DESTINATION ${LIB_DIR} LIBRARY DESTINATION ${LIB_DIR})
