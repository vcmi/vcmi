set(vcmiservercommon_SRCS
		StdInc.cpp

		battles/BattleActionProcessor.cpp
		battles/BattleFlowProcessor.cpp
		battles/BattleProcessor.cpp
		battles/BattleResultProcessor.cpp

		queries/BattleQueries.cpp
		queries/CQuery.cpp
		queries/MapQueries.cpp
		queries/VisitQueries.cpp
		queries/QueriesProcessor.cpp

		processors/HeroPoolProcessor.cpp
		processors/NewTurnProcessor.cpp
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

set(vcmiservercommon_HEADERS
		StdInc.h

		battles/BattleActionProcessor.h
		battles/BattleFlowProcessor.h
		battles/BattleProcessor.h
		battles/BattleResultProcessor.h

		queries/BattleQueries.h
		queries/CQuery.h
		queries/MapQueries.h
		queries/VisitQueries.h
		queries/QueriesProcessor.h

		processors/HeroPoolProcessor.h
		processors/NewTurnProcessor.h
		processors/PlayerMessageProcessor.h
		processors/TurnOrderProcessor.h

		CGameHandler.h
		GlobalLobbyProcessor.h
		IGameServer.h
		ServerSpellCastEnvironment.h
		CVCMIServer.h
		LobbyNetPackVisitors.h
		ServerNetPackVisitors.h
		TurnTimerHandler.h
)

assign_source_group(${vcmiservercommon_SRCS} ${vcmiservercommon_HEADERS})

add_library(vcmiservercommon STATIC ${vcmiservercommon_SRCS} ${vcmiservercommon_HEADERS})
set(vcmiservercommon_LIBS vcmi)

if(CMAKE_SYSTEM_NAME MATCHES FreeBSD OR HAIKU)
	set(vcmiservercommon_LIBS execinfo ${vcmiservercommon_LIBS})
endif()

target_link_libraries(vcmiservercommon PRIVATE ${vcmiservercommon_LIBS} minizip::minizip)

target_include_directories(vcmiservercommon
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WIN32)
	set_target_properties(vcmiservercommon
		PROPERTIES
			OUTPUT_NAME "VCMI_vcmiservercommon"
			PROJECT_LABEL "VCMI_vcmiservercommon"
	)
endif()

vcmi_set_output_dir(vcmiservercommon "")
enable_pch(vcmiservercommon)
