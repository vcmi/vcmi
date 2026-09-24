set(vcmiservercommon_SRCS
		StdInc.cpp

		battles/BattleActionProcessor.cpp
		battles/BattleFlowProcessor.cpp
		battles/BattleProcessor.cpp
		battles/BattleResultProcessor.cpp

		activities/BattleActivities.cpp
		activities/Activity.cpp
		activities/LuaScriptActivity.cpp
		activities/MapActivities.cpp
		activities/VisitActivities.cpp
		activities/ActivityProcessor.cpp

		processors/HeroPoolProcessor.cpp
		processors/NewTurnProcessor.cpp
		processors/PlayerMessageProcessor.cpp
		processors/TurnOrderProcessor.cpp

		CGameHandler.cpp
		CreatureValueEstimator.cpp
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

		activities/BattleActivities.h
		activities/Activity.h
		activities/LuaScriptActivity.h
		activities/MapActivities.h
		activities/VisitActivities.h
		activities/ActivityProcessor.h

		processors/HeroPoolProcessor.h
		processors/NewTurnProcessor.h
		processors/PlayerMessageProcessor.h
		processors/TurnOrderProcessor.h

		CGameHandler.h
		CreatureValueEstimator.h
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
# Tests can embed AI object libraries directly, so server common must consume
# the core library interface without also pulling the facade and its objects.
set(vcmiservercommon_LIBS
	$<TARGET_PROPERTY:vcmiMain,INTERFACE_LINK_LIBRARIES>
)

if(CMAKE_SYSTEM_NAME MATCHES FreeBSD OR HAIKU)
	list(APPEND vcmiservercommon_LIBS execinfo)
endif()

target_link_libraries(vcmiservercommon PRIVATE ${vcmiservercommon_LIBS} minizip::minizip)

target_include_directories(vcmiservercommon
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
	PRIVATE $<TARGET_PROPERTY:vcmiMain,INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(vcmiservercommon
	PRIVATE $<TARGET_PROPERTY:vcmiMain,INTERFACE_COMPILE_DEFINITIONS>
)

if(WIN32)
	set_target_properties(vcmiservercommon
		PROPERTIES
			OUTPUT_NAME "VCMI_vcmiservercommon"
			PROJECT_LABEL "VCMI_vcmiservercommon"
	)
	if (MINGW)
		# Link Windows system libs: WinSock2 (ws2_32)
		target_link_libraries(vcmiservercommon PRIVATE ws2_32)
	endif()
endif()

vcmi_set_output_dir(vcmiservercommon "")
enable_pch(vcmiservercommon)
