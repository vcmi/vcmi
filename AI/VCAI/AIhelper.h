/*
* AIhelper.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

/*
	!!!	Note: Include THIS file at the end of include list to avoid "undefined base class" error
*/

#include "ResourceManager.h"
#include "BuildingManager.h"
#include "ArmyManager.h"
#include "Pathfinding/PathfindingManager.h"

class ResourceManager;
class BuildingManager;


//indirection interface for various modules
class DLL_EXPORT AIhelper : public IResourceManager, public IBuildingManager, public IPathfindingManager, public IArmyManager
{
	friend class VCAI;
	friend struct SetGlobalState; //mess?

	std::shared_ptr<ResourceManager> resourceManager;
	std::shared_ptr<BuildingManager> buildingManager;
	std::shared_ptr<PathfindingManager> pathfindingManager;
	std::shared_ptr<ArmyManager> armyManager;
	//TODO: vector<IAbstractManager>
public:
	AIhelper();

	bool canAfford(const TResources & cost) const;
	TResources reservedResources() const override;
	TResources freeResources() const override;
	TResource freeGold() const override;
	TResources allResources() const override;
	TResource allGold() const override;

	Goals::TSubgoal whatToDo(TResources &res, Goals::TSubgoal goal) override;
	Goals::TSubgoal whatToDo() const override;
	bool containsObjective(Goals::TSubgoal goal) const override;
	bool hasTasksLeft() const override;
	bool removeOutdatedObjectives(std::function<bool(const Goals::TSubgoal &)> predicate) override;

	bool getBuildingOptions(const CGTownInstance * t) override;
	BuildingID getMaxPossibleGoldBuilding(const CGTownInstance * t);
	boost::optional<PotentialBuilding> immediateBuilding() const override;
	boost::optional<PotentialBuilding> expensiveBuilding() const override;
	boost::optional<BuildingID> canBuildAnyStructure(const CGTownInstance * t, const std::vector<BuildingID> & buildList, unsigned int maxDays = 7) const override;

	Goals::TGoalVec howToVisitTile(const HeroPtr & hero, const int3 & tile, bool allowGatherArmy = true) const override;
	Goals::TGoalVec howToVisitObj(const HeroPtr & hero, ObjectIdRef obj, bool allowGatherArmy = true) const override;
	Goals::TGoalVec howToVisitTile(const int3 & tile) const override;
	Goals::TGoalVec howToVisitObj(ObjectIdRef obj) const override;
	std::vector<AIPath> getPathsToTile(const HeroPtr & hero, const int3 & tile) const override;
	void updatePaths(std::vector<HeroPtr> heroes) override;

	STRONG_INLINE
	bool isTileAccessible(const HeroPtr & hero, const int3 & tile) const
	{
		return pathfindingManager->isTileAccessible(hero, tile);
	}

	bool canGetArmy(const CArmedInstance * target, const CArmedInstance * source) const override;
	ui64 howManyReinforcementsCanBuy(const CCreatureSet * target, const CGDwelling * source) const override;
	ui64 howManyReinforcementsCanGet(const CCreatureSet * target, const CCreatureSet * source) const override;
	std::vector<SlotInfo> getBestArmy(const CCreatureSet * target, const CCreatureSet * source) const override;
	std::vector<SlotInfo>::iterator getWeakestCreature(std::vector<SlotInfo> & army) const override;
	std::vector<SlotInfo> getSortedSlots(const CCreatureSet * target, const CCreatureSet * source) const override;

private:
	bool notifyGoalCompleted(Goals::TSubgoal goal) override;

	void init(CPlayerSpecificInfoCallback * CB) override;
	void setAI(VCAI * AI) override;
};

