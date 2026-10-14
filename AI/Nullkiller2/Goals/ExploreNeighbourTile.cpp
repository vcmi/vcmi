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
#include "../../../lib/pathfinder/CGPathNode.h"


namespace NK2AI
{

using namespace Goals;

namespace
{
float getNeighbourStrategicProgress(
	const CGHeroInstance * hero,
	const Nullkiller * aiNk,
	const std::shared_ptr<const CPathsInfo> & pathsInfo,
	const int3 & tile)
{
	float result = 0.0f;

	auto targetReachedThroughTile = [&pathsInfo, &tile](const CGObjectInstance * obj)
	{
		if(!obj || obj->visitablePos().z != tile.z)
			return false;

		CGPath path;
		if(!pathsInfo->getPath(path, obj->visitablePos()) || !path.hasNextNode())
			return false;

		return path.nextNode().coord == tile;
	};

	for(const ObjectInstanceID objId : aiNk->memory->visitableObjs)
	{
		const CGObjectInstance * obj = aiNk->cc->getObj(objId, false);
		if(!targetReachedThroughTile(obj))
			continue;

		const auto relations = aiNk->cc->getPlayerRelations(obj->tempOwner, hero->tempOwner);
		if(obj->ID == Obj::TOWN && relations == PlayerRelations::ENEMIES)
			result = std::max(result, 6.0f);
		else if(obj->ID == Obj::HERO && relations == PlayerRelations::ENEMIES)
			result = std::max(result, 4.0f);
		else if(aiNk->memory->wasVisited(obj))
			result = std::max(result, 0.25f);
		else
			result = std::max(result, 1.0f);
	}

	for(const CGTownInstance * town : aiNk->cc->getTownsInfo(false))
	{
		if(targetReachedThroughTile(town)
			&& aiNk->cc->getPlayerRelations(town->tempOwner, hero->tempOwner) == PlayerRelations::ENEMIES)
		{
			result = std::max(result, 6.0f);
		}
	}

	return result;
}

std::optional<NeighbourExplorationTarget> getNeighbourExplorationTarget(
	const CGHeroInstance * hero,
	const Nullkiller * aiNk,
	ExplorationHelper & helper,
	const int3 & tile)
{
	const auto pathsInfo = aiNk->getPathsInfo(hero);
	const auto pathInfo = pathsInfo->getPathInfo(tile);
	const uint64_t danger = aiNk->dangerEvaluator->evaluateDanger(tile, hero, true);

	NeighbourExplorationCandidate candidate;
	candidate.sameDay = pathInfo->turns == 0;
	candidate.accessible = pathInfo->accessible == EPathAccessibility::ACCESSIBLE;
	candidate.safe = danger == 0;
	candidate.tilesDiscovered = helper.howManyTilesWillBeDiscovered(tile);
	candidate.strategicProgress = getNeighbourStrategicProgress(hero, aiNk, pathsInfo, tile);
	candidate.movementCost = pathInfo->getCost();

	const auto evaluation = evaluateNeighbourExplorationCandidate(candidate);
	if(!evaluation.accepted)
		return std::nullopt;

	return NeighbourExplorationTarget{
		tile,
		candidate.tilesDiscovered,
		candidate.movementCost,
		candidate.strategicProgress,
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
		|| (candidate.tilesDiscovered <= 0 && candidate.strategicProgress <= 0.0f))
	{
		return result;
	}

	const float discoveryValue = candidate.tilesDiscovered * candidate.tilesDiscovered;

	result.accepted = true;
	result.value = (discoveryValue + candidate.strategicProgress)
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
	bool moved = false;

	for(int i = 0; i < tilesToExplore && aiGw->cc->getObj(hero->id, false) && hero->movementPointsRemaining() > 0; i++)
	{
		const auto target = findTarget(hero, aiGw->nullkiller.get());

		if(!target || !aiGw->moveHeroToTile(target->tile, HeroPtr(hero, aiGw->cc.get())))
			return;

		moved = true;
	}

	if(lockAfterMove && moved)
		aiGw->nullkiller->lockHero(hero, HeroLockedReason::HERO_CHAIN);
}

std::string ExploreNeighbourTile::toString() const
{
	return "Explore neighbour tiles by " + hero->getNameTextID();
}

}
