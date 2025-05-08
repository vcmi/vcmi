set(serverapp_SRCS
		StdInc.cpp
		EntryPoint.cpp
)

set(serverapp_HEADERS
		StdInc.h
)

assign_source_group(${serverapp_SRCS} ${serverapp_HEADERS})
add_executable(vcmiserver ${serverapp_SRCS} ${serverapp_HEADERS})
vcmi_create_exe_shim(vcmiserver)

set(serverapp_LIBS vcmi)
if(CMAKE_SYSTEM_NAME MATCHES FreeBSD OR HAIKU)
	list(APPEND serverapp_LIBS execinfo)
endif()
target_link_libraries(vcmiserver PRIVATE ${serverapp_LIBS} minizip::minizip vcmiservercommon)

target_include_directories(vcmiserver
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
	set_target_properties(vcmiserver
		PROPERTIES
			OUTPUT_NAME "VCMI_server"
			PROJECT_LABEL "VCMI_server"
	)
endif()

vcmi_set_output_dir(vcmiserver "")
enable_pch(vcmiserver)

install(TARGETS vcmiserver DESTINATION ${BIN_DIR})
