set(battleML_SRCS
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

set(battleML_HEADERS
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

assign_source_group(${battleML_SRCS} ${battleML_HEADERS})

if(ANDROID) # android compiles ai libs into main lib directly, so we skip this library and just reuse sources list
	return()
endif()

add_library(BattleML SHARED ${battleML_SRCS} ${battleML_HEADERS})
target_include_directories(BattleML PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(BattleML PRIVATE vcmi)

vcmi_set_output_dir(BattleML "AI")

set_target_properties(BattleML PROPERTIES ${PCH_PROPERTIES})
cotire(BattleML)

install(TARGETS BattleML RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR})
