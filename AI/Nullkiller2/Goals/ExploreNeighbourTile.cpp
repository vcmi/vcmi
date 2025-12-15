/*
* ExploreNeighbourTile.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "ExploreNeighbourTile.h"
#include "../AIGateway.h"
#include "../AIUtility.h"
#include "../Helpers/ExplorationHelper.h"


namespace NK2AI
{

using namespace Goals;

bool ExploreNeighbourTile::operator==(const ExploreNeighbourTile & other) const
{
	return false;
}

void ExploreNeighbourTile::accept(AIGateway * aiGw)
{
	ExplorationHelper h(hero, aiGw->nullkiller.get(), true);

	for(int i = 0; i < tilesToExplore && aiGw->cc->getObj(hero->id, false) && hero->movementPointsRemaining() > 0; i++)
	{
		int3 pos = hero->visitablePos();
		float value = 0;
		int3 target = int3(-1);
		foreach_neighbour(
			*aiGw->cc,
			pos,
			[&](int3 tile)
			{
				const auto pathInfo = aiGw->nullkiller->getPathsInfo(hero)->getPathInfo(tile);
				if(pathInfo->turns > 0)
					return;

				if(pathInfo->accessible == EPathAccessibility::ACCESSIBLE)
				{
					float newValue = h.howManyTilesWillBeDiscovered(tile);
					newValue /= std::min(0.1f, pathInfo->getCost());

					if(newValue > value)
					{
						value = newValue;
						target = tile;
					}
				}
			}
		);

		if(!target.isValid())
		{
			return;
		}

		auto danger = aiGw->nullkiller->dangerEvaluator->evaluateDanger(target, hero, true);

		if(danger > 0 || !aiGw->moveHeroToTile(target, HeroPtr(hero, aiGw->cc.get())))
		{
			return;
		}
	}
}

std::string ExploreNeighbourTile::toString() const
{
	return "Explore neighbour tiles by " + hero->getNameTranslated();
}

}
