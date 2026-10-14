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
#include "DimensionDoorUtils.h"
#include "../Goals/ExploreNeighbourTile.h"
#include "../../../lib/spells/CSpell.h"
#include "../../../lib/spells/Problem.h"
#include "../../../lib/mapping/TerrainTile.h"

namespace NK2AI
{

using namespace Goals;

namespace
{
template<typename Function>
void forEachTileInMapSquare(const CCallback * cc, const int3 & center, int rangeX, int rangeY, Function function)
{
	const int3 mapSize = cc->getMapSize();
	const int minX = std::max(0, center.x - rangeX);
	const int maxX = std::min(mapSize.x - 1, center.x + rangeX);
	const int minY = std::max(0, center.y - rangeY);
	const int maxY = std::min(mapSize.y - 1, center.y + rangeY);

	for(int x = minX; x <= maxX; ++x)
	{
		for(int y = minY; y <= maxY; ++y)
			function(int3(x, y, center.z));
	}
}

template<typename Function>
void forEachHiddenTileInSightRange(
	const CCallback * cc,
	const TeamState * ts,
	const int3 & center,
	int sightRadius,
	Function function)
{
	int3 tile(0, 0, center.z);
	const auto & fow = ts->fogOfWarMap;

	for(tile.x = center.x - sightRadius; tile.x <= center.x + sightRadius; ++tile.x)
	{
		for(tile.y = center.y - sightRadius; tile.y <= center.y + sightRadius; ++tile.y)
		{
			if(cc->isInTheMap(tile)
				&& center.dist2d(tile) - 0.5 < sightRadius
				&& !fow[tile])
			{
				function(tile);
			}
		}
	}
}
}

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

	if(!candidate.tilesDiscovered
		&& !candidate.continuationTilesDiscovered
		&& !candidate.chainTilesDiscovered
		&& candidate.strategicScore <= 0.0f)
		return evaluation;

	if(candidate.reachableWithoutDimensionDoor)
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
	const float discoveryValue = candidate.tilesDiscovered * candidate.tilesDiscovered;
	const float continuationValue = candidate.continuationTilesDiscovered * candidate.continuationTilesDiscovered;
	const float chainValue = candidate.chainTilesDiscovered * candidate.chainTilesDiscovered;
	const float value = (discoveryValue + continuationValue + chainValue + candidate.strategicScore * 25.0f) / movementCost;

	if(value <= candidate.currentBestValue)
		return evaluation;

	evaluation.accepted = true;
	evaluation.value = value;
	evaluation.tilesDiscovered = std::max(1, candidate.tilesDiscovered + candidate.chainTilesDiscovered);
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

	logAi->debug("Exploration scan visible area perimeter for hero %s", hero->getNameTextID());

	for(const int3 & tile : edgeTiles)
	{
		scanTile(tile);
	}

	if(!bestGoal->invalid())
	{
		return true;
	}

	allowDeadEndCancellation = false;
	logAi->debug("Exploration scan all possible tiles for hero %s", hero->getNameTextID());

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

	bool result = false;
	forEachDimensionDoorSpell(hero, [this, &result](const CSpell *, const auto & mechanics, const DimensionDoorEffect *)
	{
		spells::detail::ProblemImpl problem;
		if(mechanics.canBeCast(problem, cc, hero))
			result = true;
	});

	return result;
}

bool ExplorationHelper::considerDimensionDoorExplorationTargets()
{
	if(!hero || hero->movementPointsRemaining() <= 0)
		return false;

	const bool allowDeadEndCancellationBefore = allowDeadEndCancellation;
	allowDeadEndCancellation = false;
	auto restoreAllowDeadEndCancellation = vstd::makeScopeGuard([this, allowDeadEndCancellationBefore]()
	{
		allowDeadEndCancellation = allowDeadEndCancellationBefore;
	});

	forEachDimensionDoorSpell(hero, [this](const CSpell * spell, const auto & mechanics, const DimensionDoorEffect * effect)
	{
		spells::detail::ProblemImpl problem;
		if(!mechanics.canBeCast(problem, cc, hero))
			return;

		const int3 source = hero->getSightCenter();
		forEachTileInMapSquare(cc, source, effect->getRangeX(), effect->getRangeY(), [this, spell, effect](const int3 & tile)
		{
			scanDimensionDoorTile(spell, effect, tile);
		});
	});

	return !bestGoal->invalid();
}

void ExplorationHelper::scanTile(const int3 & tile)
{
	if(tile == ourPos
		|| !aiNk->cc->getTile(tile, false)
		|| !aiNk->pathfinder->isTileAccessible(HeroPtr(hero, aiNk->cc.get()), tile)) //shouldn't happen, but it does
		return;

	std::vector<AIPathSummary> summaries;
	aiNk->pathfinder->calculatePathSummaries(summaries, tile);
	if(summaries.empty())
		return;

	const int tilesDiscovered = howManyTilesWillBeDiscovered(tile);
	if(!tilesDiscovered)
		return;

	const CGObjectInstance * object = cc->getTopObj(tile);
	if(object && object->isBlockedVisitable())
		return;

	std::vector<size_t> candidateIndices;
	for(size_t index = 0; index != summaries.size(); ++index)
	{
		const auto & summary = summaries[index];
		if(summary.exchangeCount > 1
			|| summary.targetHero != hero
			|| summary.cost <= 0.f)
		{
			continue;
		}

		const float value = static_cast<float>(tilesDiscovered) * tilesDiscovered / summary.cost;
		if(value > bestValue)
			candidateIndices.push_back(index);
	}

	if(candidateIndices.empty())
		return;

	std::vector<std::optional<AIPath>> reconstructedPaths(summaries.size());
	std::vector<bool> reconstructionAttempted(summaries.size(), false);
	auto reconstruct = [&](const size_t index) -> const AIPath *
	{
		if(!reconstructionAttempted[index])
		{
			reconstructionAttempted[index] = true;
			AIPath path;
			if(aiNk->pathfinder->calculatePathInfo(path, summaries[index]))
				reconstructedPaths[index] = std::move(path);
		}

		return reconstructedPaths[index] ? &*reconstructedPaths[index] : nullptr;
	};

	const HeroRole heroRole = aiNk->heroManager->getHeroRoleOrDefaultInefficient(hero);
	float closestWayCost = std::numeric_limits<float>::max();
	for(size_t index = 0; index != summaries.size(); ++index)
	{
		if(aiNk->heroManager->getHeroRoleOrDefaultInefficient(summaries[index].targetHero) != heroRole)
			continue;

		const AIPath * path = reconstruct(index);
		if(!path || path->getFirstBlockedAction())
			continue;

		const auto goals = CaptureObjectsBehavior::getVisitGoals({ *path }, aiNk, object);
		if(vstd::contains_if(goals, [](const TSubgoal & goal)
			{
				return !goal->invalid();
			}))
		{
			closestWayCost = std::min(closestWayCost, path->movementCost());
		}
	}

	for(const size_t index : candidateIndices)
	{
		const AIPath * path = reconstruct(index);
		if(!path)
			continue;

		auto waysToVisit = CaptureObjectsBehavior::getVisitGoals({ *path }, aiNk, object);
		auto goal = waysToVisit.front();
		if(goal->invalid())
			continue;

		const float ourValue = static_cast<float>(tilesDiscovered) * tilesDiscovered / path->movementCost();
		if(ourValue <= bestValue)
			continue;

		if(auto * execute = dynamic_cast<ExecuteHeroChain *>(goal.get());
			execute && closestWayCost < std::numeric_limits<float>::max())
		{
			execute->closestWayRatio = closestWayCost / path->movementCost();
		}

		if(isSafeToVisit(hero, path->heroArmy, path->getTotalDanger(), aiNk->settings->getSafeAttackRatio()))
		{
			bestGoal = goal;
			bestValue = ourValue;
			bestTile = tile;
			bestTilesDiscovered = tilesDiscovered;
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
	candidate.reachableWithoutDimensionDoor = visible && hasNormalSameDayPath(tile);
	if(candidate.reachableWithoutDimensionDoor)
		return;

	const int remainingCasts = getRemainingDimensionDoorCasts(spell);
	const int movementPointsTaken = std::min(hero->movementPointsRemaining(), effect->getMovementPointsTaken());
	const int movementPointsAfterCast = std::max(0, hero->movementPointsRemaining() - movementPointsTaken);
	candidate.continuationTilesDiscovered = estimateDimensionDoorContinuationValue(tile, effect, movementPointsAfterCast);
	if(remainingCasts > 1)
	{
		std::set<int3> revealedTiles;
		markHiddenTilesAround(tile, revealedTiles);
		std::set<int3> visitedLandings = { hero->visitablePos(), tile };
		candidate.chainTilesDiscovered = estimateDimensionDoorChainValue(
			effect,
			tile,
			remainingCasts - 1,
			movementPointsAfterCast,
			revealedTiles,
			visitedLandings);
	}
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

bool ExplorationHelper::hasNormalSameDayPath(const int3 & tile) const
{
	const auto paths = aiNk->pathfinder->getPathInfo(tile);
	for(const AIPath & path : paths)
	{
		if(path.exchangeCount > 1
			|| path.targetHero != hero
			|| path.turn() != 0
			|| path.movementCost() <= 0.0f
			|| path.getFirstBlockedAction())
		{
			continue;
		}

		const bool hasSpecialAction = std::any_of(path.nodes.begin(), path.nodes.end(), [](const AIPathNodeInfo & node)
		{
			return node.specialAction != nullptr;
		});
		if(hasSpecialAction)
			continue;

		if(isSafeToVisit(hero, path.heroArmy, path.getTotalDanger(), aiNk->settings->getSafeAttackRatio()))
			return true;
	}

	return false;
}

int ExplorationHelper::getRemainingDimensionDoorCasts(const CSpell * spell) const
{
	const int manaCost = hero->getSpellCost(spell);
	if(manaCost <= 0)
		return 0;

	const auto & mechanics = spell->getAdventureMechanics();
	const int castsByMana = hero->mana / manaCost;
	const int castsLimit = mechanics.getCastsLimit(hero, cc->getMapSize());
	const int castsAlreadyPerformed = mechanics.getCastsAlreadyPerformed(hero);
	const int castsByLimit = castsLimit > 0
		? std::max(0, castsLimit - castsAlreadyPerformed)
		: castsByMana;

	return std::min(castsByMana, castsByLimit);
}

bool ExplorationHelper::isDimensionDoorLandingSafe(const int3 & tile) const
{
	if(!cc->getSettings().getBoolean(EGameSettings::SPELLS_DIMENSION_DOOR_TRIGGERS_GUARDS))
		return true;

	if(!ts->fogOfWarMap[tile])
		return false;

	const uint64_t guardedLandingDanger = aiNk->dangerEvaluator->evaluateDanger(tile, hero, true);
	return guardedLandingDanger == 0
		|| isSafeToVisit(hero, guardedLandingDanger, aiNk->settings->getSafeAttackRatio());
}

int ExplorationHelper::estimateDimensionDoorChainValue(
	const DimensionDoorEffect * effect,
	const int3 & source,
	int remainingCasts,
	int movementPointsRemaining,
	const std::set<int3> & revealedTiles,
	const std::set<int3> & visitedLandings) const
{
	if(remainingCasts <= 0 || movementPointsRemaining <= effect->getMovementPointsRequired())
		return 0;

	struct ChainCandidate
	{
		int3 tile;
		int value = 0;
	};

	constexpr size_t maxCandidatesPerDepth = 12;
	std::vector<ChainCandidate> candidates;

	forEachTileInMapSquare(
		cc,
		source,
		effect->getRangeX(),
		effect->getRangeY(),
		[this, effect, source, movementPointsRemaining, &revealedTiles, &visitedLandings, &candidates](const int3 & destination)
	{
		if(destination == source || visitedLandings.contains(destination))
			return;

		if(!effect->isValidTargetFrom(cc, hero, source, destination))
			return;

		if(!isDimensionDoorLandingSafe(destination))
			return;

		const int movementTaken = std::min(movementPointsRemaining, effect->getMovementPointsTaken());
		const int movementPointsAfterCast = std::max(0, movementPointsRemaining - movementTaken);
		const int newTiles = countHiddenTilesAround(destination, revealedTiles);
		const int continuation = estimateDimensionDoorContinuationValue(destination, effect, movementPointsAfterCast, revealedTiles);
		if(newTiles <= 0 && continuation <= 0)
			return;

		candidates.push_back({ destination, newTiles + continuation });
	});

	std::sort(candidates.begin(), candidates.end(), [](const ChainCandidate & lhs, const ChainCandidate & rhs)
	{
		if(lhs.value != rhs.value)
			return lhs.value > rhs.value;

		return lhs.tile < rhs.tile;
	});

	if(candidates.size() > maxCandidatesPerDepth)
		candidates.resize(maxCandidatesPerDepth);

	int bestChainValue = 0;
	for(const ChainCandidate & candidate : candidates)
	{
		auto branchRevealedTiles = revealedTiles;
		const int newTiles = markHiddenTilesAround(candidate.tile, branchRevealedTiles);
		const int movementTaken = std::min(movementPointsRemaining, effect->getMovementPointsTaken());
		const int movementPointsAfterCast = std::max(0, movementPointsRemaining - movementTaken);
		const int continuation = estimateDimensionDoorContinuationValue(candidate.tile, effect, movementPointsAfterCast, branchRevealedTiles);

		auto branchVisitedLandings = visitedLandings;
		branchVisitedLandings.insert(candidate.tile);

		const int futureValue = estimateDimensionDoorChainValue(
			effect,
			candidate.tile,
			remainingCasts - 1,
			movementPointsAfterCast,
			branchRevealedTiles,
			branchVisitedLandings);

		bestChainValue = std::max(bestChainValue, newTiles + continuation + futureValue);
	}

	return bestChainValue;
}

float ExplorationHelper::getDimensionDoorStrategicScore(const int3 & tile) const
{
	float score = 0.0f;

	auto addTargetScore = [this, &score, &tile](const CGObjectInstance * obj, float weight)
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

int ExplorationHelper::estimateDimensionDoorContinuationValue(
	const int3 & tile,
	const DimensionDoorEffect * effect,
	int movementPointsRemaining) const
{
	return estimateDimensionDoorContinuationValue(tile, effect, movementPointsRemaining, {});
}

int ExplorationHelper::estimateDimensionDoorContinuationValue(
	const int3 & tile,
	const DimensionDoorEffect * effect,
	int movementPointsRemaining,
	const std::set<int3> & revealedTiles) const
{
	if(movementPointsRemaining <= 0)
		return 0;

	const TerrainTile * sourceTile = cc->getTileUnchecked(tile);
	if(!sourceTile)
		return 0;

	int bestContinuation = 0;

	for(const int3 & direction : int3::getDirs())
	{
		const int3 neighbour = tile + direction;
		if(!cc->isInTheMap(neighbour))
			continue;

		const TerrainTile * neighbourTile = cc->getTileUnchecked(neighbour);
		if(!neighbourTile || !neighbourTile->isClear(sourceTile))
			continue;

		bestContinuation = std::max(bestContinuation, countHiddenTilesAround(neighbour, revealedTiles));
	}

	return bestContinuation;
}

int ExplorationHelper::countHiddenTilesAround(const int3 & pos, const std::set<int3> & excludedTiles) const
{
	int result = 0;

	forEachHiddenTileInSightRange(cc, ts, pos, sightRadius, [&excludedTiles, &result](const int3 & tile)
	{
		if(!excludedTiles.contains(tile))
			result++;
	});

	return result;
}

int ExplorationHelper::markHiddenTilesAround(const int3 & pos, std::set<int3> & revealedTiles) const
{
	int result = 0;

	forEachHiddenTileInSightRange(cc, ts, pos, sightRadius, [&revealedTiles, &result](const int3 & tile)
	{
		if(revealedTiles.insert(tile).second)
			result++;
	});

	return result;
}

int ExplorationHelper::howManyTilesWillBeDiscovered(const int3 & pos) const
{
	int ret = 0;

	forEachHiddenTileInSightRange(cc, ts, pos, sightRadius, [this, &ret](const int3 & tile)
	{
		if(allowDeadEndCancellation
			&& !hasReachableNeighbor(tile))
		{
			return;
		}

		ret++;
	});

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
