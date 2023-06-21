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
#include "../../../lib/StringConstants.h"


namespace NKAI
{

extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<AIGateway> ai;

using namespace Goals;

bool Composition::operator==(const Composition & other) const
{
	return false;
}

std::string Composition::toString() const
{
	std::string result = "Composition";

	for(auto goal : subtasks)
	{
		result += " " + goal->toString();
	}

	return result;
}

void Composition::accept(AIGateway * ai)
{
	taskptr(*subtasks.back())->accept(ai);
}

TGoalVec Composition::decompose() const
{
	return subtasks;
}

Composition & Composition::addNext(const AbstractGoal & goal)
{
	return addNext(sptr(goal));
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
		subtasks.push_back(goal);
	}

	return *this;
}

bool Composition::isElementar() const
{
	return subtasks.back()->isElementar();
}

int Composition::getHeroExchangeCount() const
{
	return isElementar() ? taskptr(*subtasks.back())->getHeroExchangeCount() : 0;
}

}
