/*
* StartupBehavior.cpp, part of VCMI engine
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

namespace NKAI
{

using namespace Goals;

std::string StayAtTownBehavior::toString() const
{
	return "StayAtTownBehavior";
}

Goals::TGoalVec StayAtTownBehavior::decompose() const
{
	Goals::TGoalVec tasks;
	auto towns = cb->getTownsInfo();

	if(!towns.size())
		return tasks;

	std::vector<AIPath> paths;

	for(auto town : towns)
	{
		if(!town->hasBuilt(BuildingID::MAGES_GUILD_1))
			continue;

		ai->nullkiller->pathfinder->calculatePathInfo(paths, town->visitablePos());

		for(auto & path : paths)
		{
			if(town->visitingHero && town->visitingHero.get() != path.targetHero)
				continue;

			if(!path.targetHero->hasSpellbook() || path.targetHero->mana >= 0.75f * path.targetHero->manaLimit())
				continue;

			if(path.turn() == 0 && !path.getFirstBlockedAction() && path.exchangeCount <= 1)
			{
				if(path.targetHero->mana == path.targetHero->manaLimit())
					continue;

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
