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
	//TODO: push to vector
}

AIhelper::~AIhelper()
{
}

bool AIhelper::notifyGoalCompleted(Goals::TSubgoal goal)
{
	return resourceManager->notifyGoalCompleted(goal);
}

void AIhelper::setCB(CPlayerSpecificInfoCallback * CB)
{
	//TODO: for
	resourceManager->setCB(CB);
	buildingManager->setCB(CB);
}

void AIhelper::setAI(VCAI * AI)
{
	//TODO: for loop
	resourceManager->setAI(AI);
	buildingManager->setAI(AI);
}

bool AIhelper::getBuildingOptions(const CGTownInstance * t)
{
	return buildingManager->getBuildingOptions(t);
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
