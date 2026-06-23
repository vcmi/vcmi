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
#include "../Engine/Nullkiller.h"
#include "../Helpers/ExplorationHelper.h"


namespace NK2AI
{

using namespace Goals;

namespace
{
std::optional<NeighbourExplorationTarget> getNeighbourExplorationTarget(
	const CGHeroInstance * hero,
	const Nullkiller * aiNk,
	ExplorationHelper & helper,
	const int3 & tile)
{
	const auto pathInfo = aiNk->getPathsInfo(hero)->getPathInfo(tile);
	const uint64_t danger = aiNk->dangerEvaluator->evaluateDanger(tile, hero, true);

	NeighbourExplorationCandidate candidate;
	candidate.sameDay = pathInfo->turns == 0;
	candidate.accessible = pathInfo->accessible == EPathAccessibility::ACCESSIBLE;
	candidate.safe = danger == 0;
	candidate.tilesDiscovered = helper.howManyTilesWillBeDiscovered(tile);
	candidate.movementCost = pathInfo->getCost();

	const auto evaluation = evaluateNeighbourExplorationCandidate(candidate);
	if(!evaluation.accepted)
		return std::nullopt;

	return NeighbourExplorationTarget{
		tile,
		candidate.tilesDiscovered,
		candidate.movementCost,
		evaluation.value
	};
}
}

NeighbourExplorationEvaluation evaluateNeighbourExplorationCandidate(
	const NeighbourExplorationCandidate & candidate)
{
	NeighbourExplorationEvaluation result;

	if(!candidate.sameDay
		|| !candidate.accessible
		|| !candidate.safe
		|| candidate.tilesDiscovered <= 0)
	{
		return result;
	}

	result.accepted = true;
	result.value = candidate.tilesDiscovered * candidate.tilesDiscovered
		/ std::max(0.1f, candidate.movementCost);
	return result;
}

bool ExploreNeighbourTile::operator==(const ExploreNeighbourTile & other) const
{
	return false;
}

std::optional<NeighbourExplorationTarget> ExploreNeighbourTile::findTarget(
	const CGHeroInstance * hero,
	const Nullkiller * aiNk)
{
	ExplorationHelper helper(hero, aiNk, true);
	const int3 pos = hero->visitablePos();
	std::optional<NeighbourExplorationTarget> result;

	foreach_neighbour(
		*aiNk->cc,
		pos,
		[&result, hero, aiNk, &helper](int3 tile)
		{
			const auto target = getNeighbourExplorationTarget(hero, aiNk, helper, tile);
			if(!target)
				return;

			if(!result || target->value > result->value)
				result = target;
		}
	);

	return result;
}

void ExploreNeighbourTile::accept(AIGateway * aiGw)
{
	for(int i = 0; i < tilesToExplore && aiGw->cc->getObj(hero->id, false) && hero->movementPointsRemaining() > 0; i++)
	{
		const auto target = findTarget(hero, aiGw->nullkiller.get());

		if(!target || !aiGw->moveHeroToTile(target->tile, HeroPtr(hero, aiGw->cc.get())))
			return;
	}
}

std::string ExploreNeighbourTile::toString() const
{
	return "Explore neighbour tiles by " + hero->getNameTranslated();
}

}
