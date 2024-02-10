set(libserver_SRCS
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
		processors/TurnOrderProcessor.cpp

		CGameHandler.cpp
		GlobalLobbyProcessor.cpp
		ServerSpellCastEnvironment.cpp
		CVCMIServer.cpp
		NetPacksServer.cpp
		NetPacksLobbyServer.cpp
		TurnTimerHandler.cpp
)

set(libserver_HEADERS
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
		processors/TurnOrderProcessor.h

		CGameHandler.h
		GlobalLobbyProcessor.h
		ServerSpellCastEnvironment.h
		CVCMIServer.h
		LobbyNetPackVisitors.h
		ServerNetPackVisitors.h
		TurnTimerHandler.h
)

assign_source_group(${libserver_SRCS} ${libserver_HEADERS})

add_library(libserver STATIC ${libserver_SRCS} ${libserver_HEADERS})
set(libserver_LIBS vcmi)

if(CMAKE_SYSTEM_NAME MATCHES FreeBSD OR HAIKU)
	set(libserver_LIBS execinfo ${libserver_LIBS})
endif()

target_link_libraries(libserver PRIVATE ${libserver_LIBS} minizip::minizip)

target_include_directories(libserver
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
	set_target_properties(vcmiserver
		PROPERTIES
			OUTPUT_NAME "VCMI_libserver"
			PROJECT_LABEL "VCMI_libserver"
	)
endif()

vcmi_set_output_dir(libserver "")
enable_pch(libserver)
