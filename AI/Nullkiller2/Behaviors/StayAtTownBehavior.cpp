/*
* StayAtTownBehavior.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "StayAtTownBehavior.h"
#include "../AIGateway.h"
#include "../AIUtility.h"
#include "../Goals/StayAtTown.h"
#include "../Goals/Composition.h"
#include "../Goals/ExecuteHeroChain.h"
#include "lib/mapObjects/MapObjects.h" //for victory conditions
#include "../Engine/Nullkiller.h"

namespace NK2AI
{

using namespace Goals;

std::string StayAtTownBehavior::toString() const
{
	return "StayAtTownBehavior";
}

Goals::TGoalVec StayAtTownBehavior::decompose(const Nullkiller * aiNk) const
{
	Goals::TGoalVec tasks;
	auto towns = aiNk->cc->getTownsInfo();

	if(!towns.size())
		return tasks;

	std::vector<AIPath> paths;

	for(auto town : towns)
	{
		aiNk->pathfinder->calculatePathInfo(paths, town->visitablePos());

		for(auto & path : paths)
		{
			if(town->getVisitingHero() && town->getVisitingHero() != path.targetHero)
				continue;

			if(!path.getFirstBlockedAction() && path.exchangeCount <= 1)
			{
				Composition stayAtTown;

				stayAtTown.addNextSequence({
						sptr(ExecuteHeroChain(path)),
						sptr(StayAtTown(town, path))
					});

				tasks.push_back(sptr(stayAtTown));
			}
		}
	}

	return tasks;
}

}
