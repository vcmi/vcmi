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
#include "../VCAI.h"
#include "../AIUtility.h"
#include "../AIhelper.h"
#include "../FuzzyHelper.h"
#include "../../../lib/mapping/CMap.h" //for victory conditions
#include "../../../lib/CPathfinder.h"
#include "../../../lib/StringConstants.h"


extern boost::thread_specific_ptr<CCallback> cb;
extern boost::thread_specific_ptr<VCAI> ai;
extern FuzzyHelper * fh;

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

void Composition::accept(VCAI * ai)
{
	taskptr(*subtasks.back())->accept(ai);
}

TGoalVec Composition::decompose() const
{
	if(isElementar())
		return subtasks;

	auto tasks = subtasks;
	tasks.pop_back();

	TSubgoal last = subtasks.back();
	auto decomposed = last->decompose();
	TGoalVec result;

	for(TSubgoal goal : decomposed)
	{
		if(goal->invalid() || goal == last || vstd::contains(tasks, goal))
			continue;

		auto newComposition = Composition(tasks);

		if(goal->goalType == COMPOSITION)
		{
			Composition & other = dynamic_cast<Composition &>(*goal);
			bool cancel = false;

			for(auto subgoal : other.subtasks)
			{
				if(subgoal == last || vstd::contains(tasks, subgoal))
				{
					cancel = true;

					break;
				}

				newComposition.addNext(subgoal);
			}

			if(cancel)
				continue;
		}
		else
		{
			newComposition.addNext(goal);
		}

		result.push_back(sptr(newComposition));
	}

	return result;
}

Composition & Composition::addNext(const AbstractGoal & goal)
{
	return addNext(sptr(goal));
}

Composition & Composition::addNext(TSubgoal goal)
{
	subtasks.push_back(goal);

	return *this;
}

bool Composition::isElementar() const
{
	return subtasks.back()->isElementar();
}