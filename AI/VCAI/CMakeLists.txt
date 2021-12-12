set(VCAI_SRCS
		StdInc.cpp

		Pathfinding/AIPathfinderConfig.cpp
		Pathfinding/AIPathfinder.cpp
		Pathfinding/AINodeStorage.cpp
		Pathfinding/PathfindingManager.cpp
		Pathfinding/Actions/BattleAction.cpp
		Pathfinding/Actions/BoatActions.cpp
		Pathfinding/Actions/TownPortalAction.cpp
		Pathfinding/Rules/AILayerTransitionRule.cpp
		Pathfinding/Rules/AIMovementAfterDestinationRule.cpp
		Pathfinding/Rules/AIMovementToDestinationRule.cpp
		Pathfinding/Rules/AIPreviousNodeRule.cpp
		AIUtility.cpp
		AIhelper.cpp
		ArmyManager.cpp
		ResourceManager.cpp
		BuildingManager.cpp
		MapObjectsEvaluator.cpp
		FuzzyEngines.cpp
		FuzzyHelper.cpp
		Goals/AbstractGoal.cpp
		Goals/BuildBoat.cpp
		Goals/Build.cpp
		Goals/BuildThis.cpp
		Goals/Explore.cpp
		Goals/GatherArmy.cpp
		Goals/GatherTroops.cpp
		Goals/BuyArmy.cpp
		Goals/AdventureSpellCast.cpp
		Goals/Win.cpp
		Goals/VisitTile.cpp
		Goals/VisitObj.cpp
		Goals/VisitHero.cpp
		Goals/CollectRes.cpp
		Goals/Trade.cpp
		Goals/RecruitHero.cpp
		Goals/Conquer.cpp
		Goals/ClearWayTo.cpp
		Goals/DigAtTile.cpp
		Goals/GetArtOfType.cpp
		Goals/FindObj.cpp
		Goals/CompleteQuest.cpp
		main.cpp
		VCAI.cpp
)

set(VCAI_HEADERS
		StdInc.h
		
		Pathfinding/AIPathfinderConfig.h
		Pathfinding/AIPathfinder.h
		Pathfinding/AINodeStorage.h
		Pathfinding/PathfindingManager.h
		Pathfinding/Actions/ISpecialAction.h
		Pathfinding/Actions/BattleAction.h
		Pathfinding/Actions/BoatActions.h
		Pathfinding/Actions/TownPortalAction.h
		Pathfinding/Rules/AILayerTransitionRule.h
		Pathfinding/Rules/AIMovementAfterDestinationRule.h
		Pathfinding/Rules/AIMovementToDestinationRule.h
		Pathfinding/Rules/AIPreviousNodeRule.h
		AIUtility.h
		AIhelper.h
		ArmyManager.h
		ResourceManager.h
		BuildingManager.h
		MapObjectsEvaluator.h
		FuzzyEngines.h
		FuzzyHelper.h
		Goals/AbstractGoal.h
		Goals/CGoal.h
		Goals/Invalid.h
		Goals/BuildBoat.h
		Goals/Build.h
		Goals/BuildThis.h
		Goals/Explore.h
		Goals/GatherArmy.h
		Goals/GatherTroops.h
		Goals/BuyArmy.h
		Goals/AdventureSpellCast.h
		Goals/Win.h
		Goals/VisitTile.h
		Goals/VisitObj.h
		Goals/VisitHero.h
		Goals/CollectRes.h
		Goals/Trade.h
		Goals/RecruitHero.h
		Goals/Conquer.h
		Goals/ClearWayTo.h
		Goals/DigAtTile.h
		Goals/GetArtOfType.h
		Goals/FindObj.h
		Goals/CompleteQuest.h
		Goals/Goals.h
		VCAI.h
)

assign_source_group(${VCAI_SRCS} ${VCAI_HEADERS})

if(ANDROID) # android compiles ai libs into main lib directly, so we skip this library and just reuse sources list
	return()
endif()

add_library(VCAI SHARED ${VCAI_SRCS} ${VCAI_HEADERS})

target_include_directories(VCAI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(VCAI PRIVATE vcmi fuzzylite::fuzzylite)

vcmi_set_output_dir(VCAI "AI")
enable_pch(VCAI)

install(TARGETS VCAI RUNTIME DESTINATION ${AI_LIB_DIR} LIBRARY DESTINATION ${AI_LIB_DIR} COMPONENT AI)
