set(battleAI_SRCS
		AttackPossibility.cpp
		BattleAI.cpp
		BattleEvaluator.cpp
		PossibleSpellcast.cpp
		PotentialTargets.cpp
		StackWithBonuses.cpp
		SpellTargetsEvaluator.cpp
		ThreatMap.cpp
		BattleExchangeVariant.cpp
)

set(battleAI_HEADERS
		StdInc.h

		AttackPossibility.h
		BattleAI.h
		BattleEvaluator.h
		PotentialTargets.h
		PossibleSpellcast.h
		StackWithBonuses.h
		SpellTargetsEvaluator.h
		ThreatMap.h
		BattleExchangeVariant.h
)

assign_source_group(${battleAI_SRCS} ${battleAI_HEADERS})

add_library(BattleAI OBJECT ${battleAI_SRCS} ${battleAI_HEADERS})
target_compile_definitions(BattleAI PRIVATE VCMI_DLL=1)

target_include_directories(BattleAI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(BattleAI PRIVATE vcmiMain)

vcmi_set_output_dir(BattleAI "AI")
enable_pch(BattleAI)
