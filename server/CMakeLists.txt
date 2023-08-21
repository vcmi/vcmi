set(server_SRCS
		StdInc.cpp

		battles/BattleActionProcessor.cpp
		battles/BattleFlowProcessor.cpp
		battles/BattleProcessor.cpp
		battles/BattleResultProcessor.cpp

		queries/BattleQueries.cpp
		queries/CQuery.cpp
		queries/MapQueries.cpp
		queries/QueriesProcessor.cpp

		processors/HeroPoolProcessor.cpp
		processors/PlayerMessageProcessor.cpp

		CGameHandler.cpp
		ServerSpellCastEnvironment.cpp
		CVCMIServer.cpp
		NetPacksServer.cpp
		NetPacksLobbyServer.cpp
)

set(server_HEADERS
		StdInc.h

		battles/BattleActionProcessor.h
		battles/BattleFlowProcessor.h
		battles/BattleProcessor.h
		battles/BattleResultProcessor.h

		queries/BattleQueries.h
		queries/CQuery.h
		queries/MapQueries.h
		queries/QueriesProcessor.h

		processors/HeroPoolProcessor.h
		processors/PlayerMessageProcessor.h

		CGameHandler.h
		ServerSpellCastEnvironment.h
		CVCMIServer.h
		LobbyNetPackVisitors.h
		ServerNetPackVisitors.h
)

assign_source_group(${server_SRCS} ${server_HEADERS})

if(ENABLE_SINGLE_APP_BUILD)
	add_library(vcmiserver STATIC ${server_SRCS} ${server_HEADERS})
	target_compile_definitions(vcmiserver PUBLIC VCMI_DLL_STATIC=1)
	set(server_LIBS vcmi_lib_server)
else()
	if(ANDROID)
		add_library(vcmiserver SHARED ${server_SRCS} ${server_HEADERS})
	else()
		add_executable(vcmiserver ${server_SRCS} ${server_HEADERS})
	endif()
	set(server_LIBS vcmi)
endif()

if(CMAKE_SYSTEM_NAME MATCHES FreeBSD OR HAIKU)
	set(server_LIBS execinfo ${server_LIBS})
endif()
target_link_libraries(vcmiserver PRIVATE ${server_LIBS} minizip::minizip)

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

if(NOT ENABLE_SINGLE_APP_BUILD)
	if(ANDROID)
		install(TARGETS vcmiserver DESTINATION ${LIB_DIR})
	else()
		install(TARGETS vcmiserver DESTINATION ${BIN_DIR})
	endif()
endif()
