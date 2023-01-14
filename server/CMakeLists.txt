set(server_SRCS
		StdInc.cpp

		CGameHandler.cpp
		CQuery.cpp
		CVCMIServer.cpp
		NetPacksServer.cpp
		NetPacksLobbyServer.cpp
)

set(server_HEADERS
		StdInc.h

		CGameHandler.h
		CQuery.h
		CVCMIServer.h
)

assign_source_group(${server_SRCS} ${server_HEADERS})

if(ANDROID) # android needs client/server to be libraries, not executables, so we can't reuse the build part of this script
	return()
endif()

if(ENABLE_SINGLE_APP_BUILD)
	add_library(vcmiserver STATIC ${server_SRCS} ${server_HEADERS})
	target_compile_definitions(vcmiserver PUBLIC VCMI_DLL_STATIC=1)
	set(server_LIBS vcmi_lib_server)
else()
	add_executable(vcmiserver ${server_SRCS} ${server_HEADERS})
	set(server_LIBS vcmi)
endif()

if(CMAKE_SYSTEM_NAME MATCHES FreeBSD)
	set(server_LIBS execinfo ${server_LIBS})
endif()
target_link_libraries(vcmiserver PRIVATE ${server_LIBS} minizip::minizip ${ENET_LIBRARIES})

target_include_directories(vcmiserver
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	${ENET_INCLUDE_DIRS}
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

if(NOT ENABLE_SINGLE_APP_BUILD)
	install(TARGETS vcmiserver DESTINATION ${BIN_DIR})
endif()
