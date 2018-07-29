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

boost::thread_specific_ptr<AIhelper> ah;

AIhelper::AIhelper()
{
	resourceManager.reset(new ResourceManager());
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
	resourceManager->setCB(CB);
}

void AIhelper::setAI(VCAI * AI)
{
	resourceManager->setAI(AI);
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
