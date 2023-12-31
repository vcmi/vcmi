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

AIhelper::AIhelper()
{
	resourceManager.reset(new ResourceManager());
	buildingManager.reset(new BuildingManager());
	pathfindingManager.reset(new PathfindingManager());
	armyManager.reset(new ArmyManager());
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
	armyManager->init(CB);
}

void AIhelper::setAI(VCAI * AI)
{
	resourceManager->setAI(AI);
	buildingManager->setAI(AI);
	pathfindingManager->setAI(AI);
	armyManager->setAI(AI);
}

bool AIhelper::getBuildingOptions(const CGTownInstance * t)
{
	return buildingManager->getBuildingOptions(t);
}

BuildingID AIhelper::getMaxPossibleGoldBuilding(const CGTownInstance * t)
{
	return buildingManager->getMaxPossibleGoldBuilding(t);
}

std::optional<PotentialBuilding> AIhelper::immediateBuilding() const
{
	return buildingManager->immediateBuilding();
}

std::optional<PotentialBuilding> AIhelper::expensiveBuilding() const
{
	return buildingManager->expensiveBuilding();
}

std::optional<BuildingID> AIhelper::canBuildAnyStructure(const CGTownInstance * t, const std::vector<BuildingID> & buildList, unsigned int maxDays) const
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

Goals::TGoalVec AIhelper::howToVisitTile(const int3 & tile) const
{
	return pathfindingManager->howToVisitTile(tile);
}

Goals::TGoalVec AIhelper::howToVisitObj(ObjectIdRef obj) const
{
	return pathfindingManager->howToVisitObj(obj);
}

Goals::TGoalVec AIhelper::howToVisitTile(const HeroPtr & hero, const int3 & tile, bool allowGatherArmy) const
{
	return pathfindingManager->howToVisitTile(hero, tile, allowGatherArmy);
}

Goals::TGoalVec AIhelper::howToVisitObj(const HeroPtr & hero, ObjectIdRef obj, bool allowGatherArmy) const
{
	return pathfindingManager->howToVisitObj(hero, obj, allowGatherArmy);
}

std::vector<AIPath> AIhelper::getPathsToTile(const HeroPtr & hero, const int3 & tile) const
{
	return pathfindingManager->getPathsToTile(hero, tile);
}

void AIhelper::updatePaths(std::vector<HeroPtr> heroes)
{
	pathfindingManager->updatePaths(heroes);
}

bool AIhelper::canGetArmy(const CArmedInstance * army, const CArmedInstance * source) const
{
	return armyManager->canGetArmy(army, source);
}

ui64 AIhelper::howManyReinforcementsCanBuy(const CCreatureSet * h, const CGDwelling * t) const
{
	return armyManager->howManyReinforcementsCanBuy(h, t);
}

ui64 AIhelper::howManyReinforcementsCanGet(const CCreatureSet * target, const CCreatureSet * source) const
{
	return armyManager->howManyReinforcementsCanGet(target, source);
}

std::vector<SlotInfo> AIhelper::getBestArmy(const CCreatureSet * target, const CCreatureSet * source) const
{
	return armyManager->getBestArmy(target, source);
}

std::vector<SlotInfo>::iterator AIhelper::getWeakestCreature(std::vector<SlotInfo> & army) const
{
	return armyManager->getWeakestCreature(army);
}

std::vector<SlotInfo> AIhelper::getSortedSlots(const CCreatureSet * target, const CCreatureSet * source) const
{
	return armyManager->getSortedSlots(target, source);
}
