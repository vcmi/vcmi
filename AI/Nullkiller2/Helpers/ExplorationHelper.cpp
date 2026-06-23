/*
* ExplorationHelper.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "ExplorationHelper.h"
#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../Engine/Nullkiller.h"
#include "../Goals/Invalid.h"
#include "../Goals/AdventureSpellCast.h"
#include "../Goals/Composition.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Markers/ExplorationPoint.h"
#include "../../../lib/CPlayerState.h"
#include "../../../lib/IGameSettings.h"
#include "../../../lib/ScopeGuard.h"
#include "../Behaviors/CaptureObjectsBehavior.h"
#include "../Goals/ExploreNeighbourTile.h"
#include "../../../lib/spells/CSpell.h"
#include "../../../lib/spells/CSpellHandler.h"
#include "../../../lib/spells/Problem.h"
#include "../../../lib/spells/adventure/DimensionDoorEffect.h"

namespace NK2AI
{

using namespace Goals;

ExplorationHelper::ExplorationHelper(const CGHeroInstance * hero, const Nullkiller * aiNk, bool useCPathfinderAccessibility)
	:aiNk(aiNk), cc(aiNk->cc.get()), hero(hero), useCPathfinderAccessibility(useCPathfinderAccessibility)
{
	ts = cc->getPlayerTeam(aiNk->playerID);
	sightRadius = hero->getSightRadius();
	bestGoal = sptr(Goals::Invalid());
	bestValue = 0;
	bestTilesDiscovered = 0;
	ourPos = hero->visitablePos();
	allowDeadEndCancellation = true;
}

TSubgoal ExplorationHelper::makeComposition() const
{
	Composition c;
	c.addNext(ExplorationPoint(bestTile, bestTilesDiscovered));
	if(shouldExploreNeighbourAfterExplorationGoal(bestGoal->goalType))
		c.addNextSequence({bestGoal, sptr(ExploreNeighbourTile(hero, 5))});
	else
		c.addNext(bestGoal);
	return sptr(c);
}

bool shouldExploreNeighbourAfterExplorationGoal(Goals::EGoals goalType)
{
	return goalType != Goals::ADVENTURE_SPELL_CAST;
}

DimensionDoorExplorationEvaluation evaluateDimensionDoorExplorationCandidate(
	const DimensionDoorExplorationCandidate & candidate)
{
	DimensionDoorExplorationEvaluation evaluation;

	if(!candidate.tilesDiscovered && candidate.strategicScore <= 0.0f)
		return evaluation;

	if(candidate.dimensionDoorTriggersGuards)
	{
		if(!candidate.visible)
			return evaluation;

		if(candidate.guardedLandingDanger && !candidate.guardedLandingSafe)
			return evaluation;
	}

	const float movementLimit = std::max(1, candidate.movementPointsLimit);
	const float movementSpent = std::min(candidate.movementPointsRemaining, candidate.movementPointsTaken);
	const float movementCost = std::max(0.1f, movementSpent / movementLimit);
	const float value = (candidate.tilesDiscovered * candidate.tilesDiscovered + candidate.strategicScore * 25.0f) / movementCost;

	if(value <= candidate.currentBestValue)
		return evaluation;

	evaluation.accepted = true;
	evaluation.value = value;
	evaluation.tilesDiscovered = std::max(1, candidate.tilesDiscovered);
	return evaluation;
}


bool ExplorationHelper::scanSector(int scanRadius)
{
	int3 tile = int3(0, 0, ourPos.z);

	for(tile.x = ourPos.x - scanRadius; tile.x <= ourPos.x + scanRadius; tile.x++)
	{
		for(tile.y = ourPos.y - scanRadius; tile.y <= ourPos.y + scanRadius; tile.y++)
		{
			if(cc->isInTheMap(tile) && ts->fogOfWarMap[tile])
			{
				scanTile(tile);
			}
		}
	}

	return !bestGoal->invalid();
}

bool ExplorationHelper::scanMap()
{
	int3 mapSize = cc->getMapSize();
	int perimeter = 2 * sightRadius * (mapSize.x + mapSize.y);

	std::vector<int3> edgeTiles;
	edgeTiles.reserve(perimeter);

	foreach_tile_pos(*aiNk->cc, [&](const int3 & pos)
		{
			if(ts->fogOfWarMap[pos])
			{
				bool hasInvisibleNeighbor = false;
				foreach_neighbour(cc, pos, [&](CCallback * cbp, int3 neighbour)
					{
						if(!ts->fogOfWarMap[neighbour])
						{
							hasInvisibleNeighbor = true;
						}
					});

				if(hasInvisibleNeighbor)
					edgeTiles.push_back(pos);
			}
		});

	logAi->debug("Exploration scan visible area perimeter for hero %s", hero->getNameTranslated());

	for(const int3 & tile : edgeTiles)
	{
		scanTile(tile);
	}

	if(!bestGoal->invalid())
	{
		return true;
	}

	allowDeadEndCancellation = false;
	logAi->debug("Exploration scan all possible tiles for hero %s", hero->getNameTranslated());

	auto potentialTiles = ts->fogOfWarMap;
	std::vector<int3> tilesToExploreFrom = edgeTiles;

	// WARNING: POTENTIAL BUG
	// AI attempts to move to any tile within sight radius to reveal some new tiles
	// however sight radius is circular, while this method assumes square radius
	// standing on the edge of a square will NOT reveal tile in opposite corner
	for(int i = 0; i < sightRadius; i++)
	{
		std::vector<int3> newTilesToExploreFrom;

		for(const int3 & tile : tilesToExploreFrom)
		{
			foreach_neighbour(cc, tile, [&](CCallback * cbp, int3 neighbour)
			{
				if(potentialTiles[neighbour])
				{
					newTilesToExploreFrom.push_back(neighbour);
					potentialTiles[neighbour] = false;
				}
			});
		}
		for(const int3 & tile : newTilesToExploreFrom)
		{
			scanTile(tile);
		}

		std::swap(tilesToExploreFrom, newTilesToExploreFrom);
	}

	return !bestGoal->invalid();
}

bool ExplorationHelper::canUseDimensionDoor() const
{
	if(!hero || hero->movementPointsRemaining() <= 0)
		return false;

	for(const auto & spell : LIBRARY->spellh->objects)
	{
		if(!spell || !spell->isAdventure())
			continue;

		const auto & mechanics = spell->getAdventureMechanics();
		if(!mechanics.getEffectAs<DimensionDoorEffect>(hero))
			continue;

		spells::detail::ProblemImpl problem;
		if(mechanics.canBeCast(problem, cc, hero))
			return true;
	}

	return false;
}

bool ExplorationHelper::considerDimensionDoorExplorationTargets()
{
	if(!hero || hero->movementPointsRemaining() <= 0)
		return false;

	const bool allowDeadEndCancellationBefore = allowDeadEndCancellation;
	allowDeadEndCancellation = false;
	auto restoreAllowDeadEndCancellation = vstd::makeScopeGuard([&]()
	{
		allowDeadEndCancellation = allowDeadEndCancellationBefore;
	});

	for(const auto & spell : LIBRARY->spellh->objects)
	{
		if(!spell || !spell->isAdventure())
			continue;

		const auto & mechanics = spell->getAdventureMechanics();
		const auto * effect = mechanics.getEffectAs<DimensionDoorEffect>(hero);

		if(!effect)
			continue;

		spells::detail::ProblemImpl problem;
		if(!mechanics.canBeCast(problem, cc, hero))
			continue;

		const int3 mapSize = cc->getMapSize();
		const int3 source = hero->getSightCenter();
		const int minX = std::max(0, source.x - effect->getRangeX());
		const int maxX = std::min(mapSize.x - 1, source.x + effect->getRangeX());
		const int minY = std::max(0, source.y - effect->getRangeY());
		const int maxY = std::min(mapSize.y - 1, source.y + effect->getRangeY());

		for(int x = minX; x <= maxX; ++x)
		{
			for(int y = minY; y <= maxY; ++y)
			{
				scanDimensionDoorTile(spell.get(), effect, int3(x, y, source.z));
			}
		}
	}

	return !bestGoal->invalid();
}

void ExplorationHelper::scanTile(const int3 & tile)
{
	if(tile == ourPos
		|| !aiNk->cc->getTile(tile, false)
		|| !aiNk->pathfinder->isTileAccessible(HeroPtr(hero, aiNk->cc.get()), tile)) //shouldn't happen, but it does
		return;

	auto paths = aiNk->pathfinder->getPathInfo(tile);
	if(paths.empty())
		return;

	int tilesDiscovered = howManyTilesWillBeDiscovered(tile);
	if(!tilesDiscovered)
		return;
	
	auto waysToVisit = CaptureObjectsBehavior::getVisitGoals(paths, aiNk, aiNk->cc->getTopObj(tile));

	for(int i = 0; i != paths.size(); i++)
	{
		auto & path = paths[i];
		auto goal = waysToVisit[i];

		if(path.exchangeCount > 1 || path.targetHero != hero || path.movementCost() <= 0.0 || goal->invalid())
			continue;

		float ourValue = (float)tilesDiscovered * tilesDiscovered / path.movementCost();

		if(ourValue > bestValue) //avoid costly checks of tiles that don't reveal much
		{
			auto obj = cc->getTopObj(tile);

			// picking up resources does not yield any exploration at all.
			// if it blocks the way to some explorable tile AIPathfinder will take care of it
			if(obj && obj->isBlockedVisitable())
			{
				continue;
			}

			if(isSafeToVisit(hero, path.heroArmy, path.getTotalDanger(), aiNk->settings->getSafeAttackRatio()))
			{
				bestGoal = goal;
				bestValue = ourValue;
				bestTile = tile;
				bestTilesDiscovered = tilesDiscovered;
			}
		}
	}
}

void ExplorationHelper::scanDimensionDoorTile(const CSpell * spell, const DimensionDoorEffect * effect, const int3 & tile)
{
	if(tile == ourPos)
		return;

	spells::detail::ProblemImpl problem;
	if(!spell->getAdventureMechanics().canBeCastAt(problem, cc, hero, tile))
		return;

	const bool visible = ts->fogOfWarMap[tile];
	const int tilesDiscovered = howManyTilesWillBeDiscovered(tile);
	const float strategicScore = getDimensionDoorStrategicScore(tile);

	DimensionDoorExplorationCandidate candidate;
	candidate.visible = visible;
	candidate.tilesDiscovered = tilesDiscovered;
	candidate.strategicScore = strategicScore;
	candidate.dimensionDoorTriggersGuards = cc->getSettings().getBoolean(EGameSettings::SPELLS_DIMENSION_DOOR_TRIGGERS_GUARDS);
	candidate.movementPointsRemaining = hero->movementPointsRemaining();
	candidate.movementPointsLimit = hero->movementPointsLimit();
	candidate.movementPointsTaken = effect->getMovementPointsTaken();
	candidate.currentBestValue = bestValue;

	if(candidate.dimensionDoorTriggersGuards && visible)
	{
		candidate.guardedLandingDanger = aiNk->dangerEvaluator->evaluateDanger(tile, hero, true);
		candidate.guardedLandingSafe = !candidate.guardedLandingDanger
			|| isSafeToVisit(hero, candidate.guardedLandingDanger, aiNk->settings->getSafeAttackRatio());
	}

	auto evaluation = evaluateDimensionDoorExplorationCandidate(candidate);

	if(evaluation.accepted)
	{
		auto castGoal = AdventureSpellCast(hero, spell->id);
		castGoal.tile = tile;

		bestGoal = sptr(castGoal);
		bestValue = evaluation.value;
		bestTile = tile;
		bestTilesDiscovered = evaluation.tilesDiscovered;
	}
}

float ExplorationHelper::getDimensionDoorStrategicScore(const int3 & tile) const
{
	float score = 0.0f;

	auto addTargetScore = [&](const CGObjectInstance * obj, float weight)
	{
		if(!obj || obj->visitablePos().z != tile.z)
			return;

		const int progress = ourPos.dist2d(obj->visitablePos()) - tile.dist2d(obj->visitablePos());
		if(progress > 0)
			score += progress * weight;
	};

	for(const ObjectInstanceID objId : aiNk->memory->visitableObjs)
	{
		const CGObjectInstance * obj = cc->getObj(objId, false);
		if(!obj)
			continue;

		const auto relations = cc->getPlayerRelations(obj->tempOwner, hero->tempOwner);
		if(obj->ID == Obj::TOWN && relations == PlayerRelations::ENEMIES)
			addTargetScore(obj, 6.0f);
		else if(obj->ID == Obj::HERO && relations == PlayerRelations::ENEMIES)
			addTargetScore(obj, 4.0f);
		else if(shouldVisit(aiNk, hero, obj))
			addTargetScore(obj, 1.0f);
		else
			addTargetScore(obj, 0.25f);
	}

	for(const CGTownInstance * town : cc->getTownsInfo())
	{
		if(cc->getPlayerRelations(town->tempOwner, hero->tempOwner) == PlayerRelations::ENEMIES)
			addTargetScore(town, 6.0f);
	}

	return score;
}

int ExplorationHelper::howManyTilesWillBeDiscovered(const int3 & pos) const
{
	int ret = 0;
	int3 npos = int3(0, 0, pos.z);

	const auto & fow = ts->fogOfWarMap;

	for(npos.x = pos.x - sightRadius; npos.x <= pos.x + sightRadius; npos.x++)
	{
		for(npos.y = pos.y - sightRadius; npos.y <= pos.y + sightRadius; npos.y++)
		{
			if(cc->isInTheMap(npos)
				&& pos.dist2d(npos) - 0.5 < sightRadius
				&& !fow[npos])
			{
				if(allowDeadEndCancellation
					&& !hasReachableNeighbor(npos))
				{
					continue;
				}

				ret++;
			}
		}
	}

	return ret;
}

bool ExplorationHelper::hasReachableNeighbor(const int3 & pos) const
{
	for(const int3 & dir : int3::getDirs())
	{
		int3 tile = pos + dir;
		if(cc->isInTheMap(tile))
		{
			auto isAccessible = useCPathfinderAccessibility
				? aiNk->getPathsInfo(hero)->getPathInfo(tile)->reachable()
				: aiNk->pathfinder->isTileAccessible(HeroPtr(hero, aiNk->cc.get()), tile);

			if(isAccessible)
				return true;
		}
	}

	return false;
}

}
