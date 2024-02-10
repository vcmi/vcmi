set(appserver_SRCS
		StdInc.cpp
		EntryPoint.cpp
)

set(appserver_HEADERS
		StdInc.h
)

assign_source_group(${appserver_SRCS} ${appserver_HEADERS})
add_executable(vcmiserver ${appserver_SRCS} ${appserver_HEADERS})
set(appserver_LIBS vcmi)

if(CMAKE_SYSTEM_NAME MATCHES FreeBSD OR HAIKU)
	set(appserver_LIBS execinfo ${appserver_LIBS})
endif()
target_link_libraries(vcmiserver PRIVATE ${appserver_LIBS} minizip::minizip libserver)

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
