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
#include "Pathfinding/PathfindingManager.h"

class ResourceManager;
class BuildingManager;


//indirection interface for various modules
class DLL_EXPORT AIhelper : public IResourceManager, public IBuildingManager, public IPathfindingManager
{
	friend class VCAI;
	friend struct SetGlobalState; //mess?

	std::shared_ptr<ResourceManager> resourceManager;
	std::shared_ptr<BuildingManager> buildingManager;
	std::shared_ptr<PathfindingManager> pathfindingManager;
	//TODO: vector<IAbstractManager>
public:
	AIhelper();
	~AIhelper();

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
	Goals::TGoalVec howToVisitTile(const int3 & tile, bool allowGatherArmy = true) const override;
	Goals::TGoalVec howToVisitObj(ObjectIdRef obj, bool allowGatherArmy = true) const override;
	std::vector<AIPath> getPathsToTile(const HeroPtr & hero, const int3 & tile) const override;
	void updatePaths(std::vector<HeroPtr> heroes, bool useHeroChain = false) override;

	STRONG_INLINE
	bool isTileAccessible(const HeroPtr & hero, const int3 & tile) const
	{
		return pathfindingManager->isTileAccessible(hero, tile);
	}

private:
	bool notifyGoalCompleted(Goals::TSubgoal goal) override;

	void init(CPlayerSpecificInfoCallback * CB) override;
	void setAI(VCAI * AI) override;
};

