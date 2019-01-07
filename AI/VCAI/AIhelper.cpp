/*
* AIhelper.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"

#include "AIhelper.h"
#include "ResourceManager.h"
#include "BuildingManager.h"

AIhelper::AIhelper()
{
	resourceManager.reset(new ResourceManager());
	buildingManager.reset(new BuildingManager());
	pathfindingManager.reset(new PathfindingManager());
}

AIhelper::~AIhelper()
{
}

bool AIhelper::notifyGoalCompleted(Goals::TSubgoal goal)
{
	return resourceManager->notifyGoalCompleted(goal);
}

void AIhelper::init(CPlayerSpecificInfoCallback * CB)
{
	resourceManager->init(CB);
	buildingManager->init(CB);
	pathfindingManager->init(CB);
}

void AIhelper::setAI(VCAI * AI)
{
	resourceManager->setAI(AI);
	buildingManager->setAI(AI);
	pathfindingManager->setAI(AI);
}

bool AIhelper::getBuildingOptions(const CGTownInstance * t)
{
	return buildingManager->getBuildingOptions(t);
}

BuildingID AIhelper::getMaxPossibleGoldBuilding(const CGTownInstance * t)
{
	return buildingManager->getMaxPossibleGoldBuilding(t);
}

boost::optional<PotentialBuilding> AIhelper::immediateBuilding() const
{
	return buildingManager->immediateBuilding();
}

boost::optional<PotentialBuilding> AIhelper::expensiveBuilding() const
{
	return buildingManager->expensiveBuilding();
}

boost::optional<BuildingID> AIhelper::canBuildAnyStructure(const CGTownInstance * t, const std::vector<BuildingID> & buildList, unsigned int maxDays) const
{
	return buildingManager->canBuildAnyStructure(t, buildList, maxDays);
}

Goals::TSubgoal AIhelper::whatToDo(TResources & res, Goals::TSubgoal goal)
{
	return resourceManager->whatToDo(res, goal);
}

Goals::TSubgoal AIhelper::whatToDo() const
{
	return resourceManager->whatToDo();
}

bool AIhelper::containsObjective(Goals::TSubgoal goal) const
{
	return resourceManager->containsObjective(goal);
}

bool AIhelper::hasTasksLeft() const
{
	return resourceManager->hasTasksLeft();
}

bool AIhelper::removeOutdatedObjectives(std::function<bool(const Goals::TSubgoal&)> predicate)
{
	return resourceManager->removeOutdatedObjectives(predicate);
}

bool AIhelper::canAfford(const TResources & cost) const
{
	return resourceManager->canAfford(cost);
}

TResources AIhelper::reservedResources() const
{
	return resourceManager->reservedResources();
}

TResources AIhelper::freeResources() const
{
	return resourceManager->freeResources();
}

TResource AIhelper::freeGold() const
{
	return resourceManager->freeGold();
}

TResources AIhelper::allResources() const
{
	return resourceManager->allResources();
}

TResource AIhelper::allGold() const
{
	return resourceManager->allGold();
}

Goals::TGoalVec AIhelper::howToVisitTile(int3 tile)
{
	return pathfindingManager->howToVisitTile(tile);
}

Goals::TGoalVec AIhelper::howToVisitObj(ObjectIdRef obj)
{
	return pathfindingManager->howToVisitObj(obj);
}

Goals::TGoalVec AIhelper::howToVisitTile(HeroPtr hero, int3 tile, bool allowGatherArmy)
{
	return pathfindingManager->howToVisitTile(hero, tile, allowGatherArmy);
}

Goals::TGoalVec AIhelper::howToVisitObj(HeroPtr hero, ObjectIdRef obj, bool allowGatherArmy)
{
	return pathfindingManager->howToVisitObj(hero, obj, allowGatherArmy);
}

std::vector<AIPath> AIhelper::getPathsToTile(HeroPtr hero, int3 tile)
{
	return pathfindingManager->getPathsToTile(hero, tile);
}

void AIhelper::resetPaths()
{
	pathfindingManager->resetPaths();
}
