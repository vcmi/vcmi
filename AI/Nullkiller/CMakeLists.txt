set(VCAI_SRCS
		StdInc.cpp

		Pathfinding/AIPathfinderConfig.cpp
		Pathfinding/AIPathfinder.cpp
		Pathfinding/AINodeStorage.cpp
		Pathfinding/Actors.cpp
		Pathfinding/Actions/SpecialAction.cpp
		Pathfinding/Actions/BattleAction.cpp
		Pathfinding/Actions/BoatActions.cpp
		Pathfinding/Actions/TownPortalAction.cpp
		Pathfinding/Rules/AILayerTransitionRule.cpp
		Pathfinding/Rules/AIMovementAfterDestinationRule.cpp
		Pathfinding/Rules/AIMovementToDestinationRule.cpp
		Pathfinding/Rules/AIPreviousNodeRule.cpp
		AIUtility.cpp
		Analyzers/ArmyManager.cpp
		Analyzers/HeroManager.cpp
		Engine/FuzzyEngines.cpp
		Engine/FuzzyHelper.cpp
		Engine/AIMemory.cpp
		Goals/AbstractGoal.cpp
		Goals/Composition.cpp
		Goals/BuildBoat.cpp
		Goals/BuildThis.cpp
		Goals/DismissHero.cpp
		Goals/GatherTroops.cpp
		Goals/BuyArmy.cpp
		Goals/AdventureSpellCast.cpp
		Goals/CollectRes.cpp
		Goals/Trade.cpp
		Goals/RecruitHero.cpp
		Goals/UnlockCluster.cpp
		Goals/DigAtTile.cpp
		Goals/GetArtOfType.cpp
		Goals/ExecuteHeroChain.cpp
		Goals/ExchangeSwapTownHeroes.cpp
		Engine/Nullkiller.cpp
		Engine/PriorityEvaluator.cpp
		Analyzers/DangerHitMapAnalyzer.cpp
		Analyzers/BuildAnalyzer.cpp
		Analyzers/ObjectClusterizer.cpp
		Behaviors/CaptureObjectsBehavior.cpp
		Behaviors/RecruitHeroBehavior.cpp
		Behaviors/BuyArmyBehavior.cpp
		Behaviors/DefenceBehavior.cpp
		Behaviors/StartupBehavior.cpp
		Behaviors/BuildingBehavior.cpp
		Behaviors/GatherArmyBehavior.cpp
		Behaviors/CompleteQuestBehavior.cpp
		Behaviors/ClusterBehavior.cpp
		main.cpp
		VCAI.cpp
)

set(VCAI_HEADERS
		StdInc.h
		
		Pathfinding/AIPathfinderConfig.h
		Pathfinding/AIPathfinder.h
		Pathfinding/AINodeStorage.h
		Pathfinding/Actors.h
		Pathfinding/Actions/SpecialAction.h
		Pathfinding/Actions/BattleAction.h
		Pathfinding/Actions/BoatActions.h
		Pathfinding/Actions/TownPortalAction.h
		Pathfinding/Rules/AILayerTransitionRule.h
		Pathfinding/Rules/AIMovementAfterDestinationRule.h
		Pathfinding/Rules/AIMovementToDestinationRule.h
		Pathfinding/Rules/AIPreviousNodeRule.h
		AIUtility.h
		Analyzers/ArmyManager.h
		Analyzers/HeroManager.h
		Engine/FuzzyEngines.h
		Engine/FuzzyHelper.h
		Engine/AIMemory.h
		Goals/AbstractGoal.h
		Goals/CGoal.h
		Goals/Composition.h
		Goals/Invalid.h
		Goals/BuildBoat.h
		Goals/BuildThis.h
		Goals/DismissHero.h
		Goals/GatherTroops.h
		Goals/BuyArmy.h
		Goals/AdventureSpellCast.h
		Goals/CollectRes.h
		Goals/Trade.h
		Goals/UnlockCluster.h
		Goals/RecruitHero.h
		Goals/DigAtTile.h
		Goals/GetArtOfType.h
		Goals/ExecuteHeroChain.h
		Goals/ExchangeSwapTownHeroes.h
		Goals/Goals.h
		Engine/Nullkiller.h
		Engine/PriorityEvaluator.h
		Analyzers/DangerHitMapAnalyzer.h
		Analyzers/BuildAnalyzer.h
		Analyzers/ObjectClusterizer.h
		Behaviors/CaptureObjectsBehavior.h
		Behaviors/RecruitHeroBehavior.h
		Behaviors/BuyArmyBehavior.h
		Behaviors/DefenceBehavior.h
		Behaviors/StartupBehavior.h
		Behaviors/BuildingBehavior.h
		Behaviors/GatherArmyBehavior.h
		Behaviors/CompleteQuestBehavior.h
		Behaviors/ClusterBehavior.h
		VCAI.h
)

assign_source_group(${VCAI_SRCS} ${VCAI_HEADERS})

if(ANDROID) # android compiles ai libs into main lib directly, so we skip this library and just reuse sources list
	return()
endif()

add_library(VCAI SHARED ${VCAI_SRCS} ${VCAI_HEADERS})

if(FL_FOUND)
	target_include_directories(VCAI PUBLIC ${FL_INCLUDE_DIRS})
else()
	target_include_directories(VCAI PUBLIC ${CMAKE_HOME_DIRECTORY}/AI/FuzzyLite/fuzzylite)
endif()
target_include_directories(VCAI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(FL_FOUND)
	target_link_libraries(VCAI PRIVATE ${FL_LIBRARIES} vcmi)
else()
	target_link_libraries(VCAI PRIVATE fl-static vcmi)
endif()

vcmi_set_output_dir(VCAI "AI")

set_target_properties(VCAI PROPERTIES ${PCH_PROPERTIES})
cotire(VCAI)

install(TARGETS VCAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR})
