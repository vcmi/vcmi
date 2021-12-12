set(battleAI_SRCS
		StdInc.cpp

		AttackPossibility.cpp
		BattleAI.cpp
		common.cpp
		EnemyInfo.cpp
		main.cpp
		PossibleSpellcast.cpp
		PotentialTargets.cpp
		StackWithBonuses.cpp
		ThreatMap.cpp
)

set(battleAI_HEADERS
		StdInc.h

		AttackPossibility.h
		BattleAI.h
		common.h
		EnemyInfo.h
		PotentialTargets.h
		PossibleSpellcast.h
		StackWithBonuses.h
		ThreatMap.h
)

assign_source_group(${battleAI_SRCS} ${battleAI_HEADERS})

if(ANDROID) # android compiles ai libs into main lib directly, so we skip this library and just reuse sources list
	return()
endif()

add_library(BattleAI SHARED ${battleAI_SRCS} ${battleAI_HEADERS})
target_include_directories(BattleAI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(BattleAI PRIVATE vcmi)

vcmi_set_output_dir(BattleAI "AI")
enable_pch(BattleAI)

install(TARGETS BattleAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR} COMPONENT AI)
