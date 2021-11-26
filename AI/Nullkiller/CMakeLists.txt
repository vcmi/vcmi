set(Nullkiller_SRCS
		StdInc.cpp

		Pathfinding/AIPathfinderConfig.cpp
		Pathfinding/AIPathfinder.cpp
		Pathfinding/AINodeStorage.cpp
		Pathfinding/Actors.cpp
		Pathfinding/Actions/SpecialAction.cpp
		Pathfinding/Actions/BattleAction.cpp
		Pathfinding/Actions/QuestAction.cpp
		Pathfinding/Actions/BuyArmyAction.cpp
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
		Goals/SaveResources.cpp
		Goals/BuildBoat.cpp
		Goals/BuildThis.cpp
		Goals/DismissHero.cpp
		Goals/BuyArmy.cpp
		Goals/AdventureSpellCast.cpp
		Goals/Trade.cpp
		Goals/CaptureObject.cpp
		Goals/RecruitHero.cpp
		Goals/DigAtTile.cpp
		Goals/ExecuteHeroChain.cpp
		Goals/ExchangeSwapTownHeroes.cpp
		Goals/CompleteQuest.cpp
		Markers/ArmyUpgrade.cpp
		Markers/HeroExchange.cpp
		Markers/UnlockCluster.cpp
		Markers/DefendTown.cpp
		Engine/Nullkiller.cpp
		Engine/DeepDecomposer.cpp
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
		Behaviors/ClusterBehavior.cpp
		main.cpp
		AIGateway.cpp
)

set(Nullkiller_HEADERS
		StdInc.h
		
		Pathfinding/AIPathfinderConfig.h
		Pathfinding/AIPathfinder.h
		Pathfinding/AINodeStorage.h
		Pathfinding/Actors.h
		Pathfinding/Actions/SpecialAction.h
		Pathfinding/Actions/BattleAction.h
		Pathfinding/Actions/QuestAction.h
		Pathfinding/Actions/BuyArmyAction.h
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
		Goals/SaveResources.h
		Goals/BuildThis.h
		Goals/DismissHero.h
		Goals/BuyArmy.h
		Goals/AdventureSpellCast.h
		Goals/Trade.h
		Goals/CaptureObject.h
		Goals/RecruitHero.h
		Goals/DigAtTile.h
		Goals/ExecuteHeroChain.h
		Goals/ExchangeSwapTownHeroes.h
		Goals/CompleteQuest.h
		Goals/Goals.h
		Markers/ArmyUpgrade.h
		Markers/HeroExchange.h
		Markers/UnlockCluster.h
		Markers/DefendTown.h
		Engine/Nullkiller.h
		Engine/DeepDecomposer.h
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
		Behaviors/ClusterBehavior.h
		AIGateway.h
)

assign_source_group(${Nullkiller_SRCS} ${Nullkiller_HEADERS})

if(ANDROID) # android compiles ai libs into main lib directly, so we skip this library and just reuse sources list
	return()
endif()

add_library(Nullkiller SHARED ${Nullkiller_SRCS} ${Nullkiller_HEADERS})

target_include_directories(Nullkiller PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(Nullkiller PRIVATE vcmi fuzzylite::fuzzylite)

target_link_libraries(Nullkiller PRIVATE TBB::tbb)

vcmi_set_output_dir(Nullkiller "AI")
enable_pch(Nullkiller)

install(TARGETS Nullkiller RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR} OPTIONAL)
