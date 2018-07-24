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
	rm.reset(new ResourceManager);
}

AIhelper::~AIhelper()
{
}

bool AIhelper::notifyGoalCompleted(Goals::TSubgoal goal)
{
	return rm->notifyGoalCompleted(goal);
}

void AIhelper::setCB(CPlayerSpecificInfoCallback * CB)
{
	rm->setCB(CB);
}

void AIhelper::setAI(VCAI * AI)
{
	rm->setAI(AI);
}

Goals::TSubgoal AIhelper::whatToDo(TResources & res, Goals::TSubgoal goal)
{
	return rm->whatToDo(res, goal);
}

Goals::TSubgoal AIhelper::whatToDo() const
{
	return rm->whatToDo();
}

bool AIhelper::hasTasksLeft() const
{
	return rm->hasTasksLeft();
}

bool AIhelper::canAfford(const TResources & cost) const
{
	return rm->canAfford(cost);
}

TResources AIhelper::reservedResources() const
{
	return rm->reservedResources();
}

TResources AIhelper::freeResources() const
{
	return rm->freeResources();
}

TResource AIhelper::freeGold() const
{
	return rm->freeGold();
}