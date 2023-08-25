/*
* BuildThis.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "Composition.h"
#include "../AIGateway.h"
#include "../AIUtility.h"
#include "../../../lib/constants/StringConstants.h"


namespace NKAI
{

using namespace Goals;

bool Composition::operator==(const Composition & other) const
{
	return false;
}

std::string Composition::toString() const
{
	std::string result = "Composition";

	for(auto step : subtasks)
	{
		result += "[";
		for(auto goal : step)
		{
			if(goal->isElementar())
				result +=  goal->toString() + " => ";
			else
				result += goal->toString() + ", ";
		}
		result += "] ";
	}

	return result;
}

void Composition::accept(AIGateway * ai)
{
	for(auto task : subtasks.back())
	{
		if(task->isElementar())
		{
			taskptr(*task)->accept(ai);
		}
		else
		{
			break;
		}
	}
}

TGoalVec Composition::decompose() const
{
	TGoalVec result;

	for(const TGoalVec & step : subtasks)
		vstd::concatenate(result, step);

	return result;
}

Composition & Composition::addNextSequence(const TGoalVec & taskSequence)
{
	subtasks.push_back(taskSequence);

	return *this;
}

Composition & Composition::addNext(TSubgoal goal)
{
	if(goal->goalType == COMPOSITION)
	{
		Composition & other = dynamic_cast<Composition &>(*goal);
		
		vstd::concatenate(subtasks, other.subtasks);
	}
	else
	{
		subtasks.push_back({goal});
	}

	return *this;
}

Composition & Composition::addNext(const AbstractGoal & goal)
{
	return addNext(sptr(goal));
}

bool Composition::isElementar() const
{
	return subtasks.back().front()->isElementar();
}

int Composition::getHeroExchangeCount() const
{
	auto result = 0;

	for(auto task : subtasks.back())
	{
		if(task->isElementar())
		{
			result += taskptr(*task)->getHeroExchangeCount();
		}
	}
	
	return result;
}

}
