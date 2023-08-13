set(battleAI_SRCS
		AttackPossibility.cpp
		BattleAI.cpp
		common.cpp
		EnemyInfo.cpp
		PossibleSpellcast.cpp
		PotentialTargets.cpp
		StackWithBonuses.cpp
		ThreatMap.cpp
		BattleExchangeVariant.cpp
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
		BattleExchangeVariant.h
)

if(NOT ENABLE_STATIC_AI_LIBS)
	list(APPEND battleAI_SRCS main.cpp StdInc.cpp)
endif()
assign_source_group(${battleAI_SRCS} ${battleAI_HEADERS})

if(ENABLE_STATIC_AI_LIBS)
	add_library(BattleAI STATIC ${battleAI_SRCS} ${battleAI_HEADERS})
else()
	add_library(BattleAI SHARED ${battleAI_SRCS} ${battleAI_HEADERS})
	install(TARGETS BattleAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR})
endif()

target_include_directories(BattleAI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(BattleAI PRIVATE ${VCMI_LIB_TARGET} TBB::tbb)

vcmi_set_output_dir(BattleAI "AI")
enable_pch(BattleAI)

if(APPLE_IOS AND NOT USING_CONAN)
	install(IMPORTED_RUNTIME_ARTIFACTS TBB::tbb LIBRARY DESTINATION ${LIB_DIR}) # CMake 3.21+
endif()
