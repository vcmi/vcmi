/*
* EscapeBehavior.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "EscapeBehavior.h"

#include "../AIUtility.h"
#include "../Analyzers/DangerHitMapAnalyzer.h"
#include "../Engine/Nullkiller.h"
#include "../Goals/ExecuteHeroChain.h"
#include "../Pathfinding/Actions/DimensionDoorAction.h"
#include "../pforeach.h"

namespace NK2AI
{

using namespace Goals;

namespace
{
bool actionUsesDimensionDoor(const std::shared_ptr<const SpecialAction> & action)
{
	if(!action)
		return false;

	if(dynamic_cast<const AIPathfinding::DimensionDoorAction *>(action.get()))
		return true;

	for(const auto & part : action->getParts())
	{
		if(actionUsesDimensionDoor(part))
			return true;
	}

	return false;
}

bool pathUsesDimensionDoor(const AIPath & path)
{
	return std::any_of(path.nodes.begin(), path.nodes.end(), [](const AIPathNodeInfo & node)
	{
		return actionUsesDimensionDoor(node.specialAction);
	});
}

bool isHeroImmediatelyThreatened(
	const CGHeroInstance * hero,
	const HitMapInfo & threat,
	const float safeAttackRatio)
{
	return threat.turn < 1
		&& threat.danger
		&& !isSafeToVisit(hero, threat.danger, safeAttackRatio);
}

struct EscapePathChoice
{
	AIPath path;
	float score = 0.0f;
};

bool isBetterEscapePath(const AIPath & path, float score, const EscapePathChoice & previous)
{
	if(score > previous.score)
		return true;

	if(vstd::isAlmostEqual(score, previous.score))
		return path.movementCost() < previous.path.movementCost();

	return false;
}

const CGTownInstance * findNearestOwnedTown(const CGHeroInstance * hero, const Nullkiller * aiNk)
{
	const CGTownInstance * bestTown = nullptr;
	const CGTownInstance * bestTownAnyLevel = nullptr;
	ui32 bestDistance = std::numeric_limits<ui32>::max();
	ui32 bestDistanceAnyLevel = std::numeric_limits<ui32>::max();

	for(const auto * town : aiNk->cc->getTownsInfo())
	{
		const auto distance = hero->visitablePos().dist2dSQ(town->visitablePos());
		if(town->visitablePos().z == hero->visitablePos().z && distance < bestDistance)
		{
			bestTown = town;
			bestDistance = distance;
		}
		if(distance < bestDistanceAnyLevel)
		{
			bestTownAnyLevel = town;
			bestDistanceAnyLevel = distance;
		}
	}

	return bestTown ? bestTown : bestTownAnyLevel;
}

EscapePathCandidate makeEscapePathCandidate(
	const AIPath & path,
	const HitMapInfo & currentThreat,
	const HitMapInfo & destinationThreat,
	uint64_t destinationDanger,
	const CGTownInstance * nearestOwnedTown,
	float safeAttackRatio)
{
	const auto destination = path.targetTile();

	EscapePathCandidate candidate;
	candidate.currentTileThreatensHero = true;
	candidate.sameDay = path.turn() == 0;
	candidate.sameTile = destination == path.targetHero->visitablePos();
	candidate.blockedAction = path.getFirstBlockedAction() != nullptr;
	candidate.singleHeroPath = path.exchangeCount <= 1;
	candidate.destinationSafe = isSafeToVisit(path.targetHero, path.heroArmy, destinationDanger, safeAttackRatio);
	candidate.destinationIsSafer = destinationThreat.threat < currentThreat.threat;
	candidate.usesDimensionDoor = pathUsesDimensionDoor(path);
	candidate.threatReduction = currentThreat.threat - destinationThreat.threat;
	candidate.movementCost = path.movementCost();
	if(nearestOwnedTown)
	{
		candidate.hasOwnedTown = true;
		candidate.currentTownDistance = path.targetHero->visitablePos().dist2dSQ(nearestOwnedTown->visitablePos());
		candidate.destinationTownDistance = destination.dist2dSQ(nearestOwnedTown->visitablePos());
		candidate.destinationGetsCloserToOwnedTown = candidate.destinationTownDistance < candidate.currentTownDistance;
	}
	return candidate;
}

void considerEscapePath(
	const AIPath & path,
	const HeroMap<HitMapInfo> & threatenedHeroes,
	const HeroMap<const CGTownInstance *> & nearestOwnedTowns,
	const Nullkiller * aiNk,
	float safeAttackRatio,
	std::mutex & sync,
	HeroMap<EscapePathChoice> & bestPaths)
{
	const auto threatenedHero = threatenedHeroes.find(path.targetHero);
	if(threatenedHero == threatenedHeroes.end())
		return;

	const auto destination = path.targetTile();
	const auto & currentThreat = threatenedHero->second;
	const auto & destinationThreat = aiNk->dangerHitMap->getTileThreat(destination).fastestDanger;
	const uint64_t immediateDestinationDanger = destinationThreat.turn < 1 ? destinationThreat.danger : 0;
	const uint64_t destinationDanger = std::max(path.getTotalDanger(), immediateDestinationDanger);
	const auto nearestOwnedTown = nearestOwnedTowns.find(path.targetHero);
	const auto candidate = makeEscapePathCandidate(
		path,
		currentThreat,
		destinationThreat,
		destinationDanger,
		nearestOwnedTown == nearestOwnedTowns.end() ? nullptr : nearestOwnedTown->second,
		safeAttackRatio);

	const auto evaluation = evaluateEscapePathCandidate(candidate);
	if(!evaluation.accepted)
		return;

	std::lock_guard lock(sync);
	const auto previous = bestPaths.find(path.targetHero);
	if(previous == bestPaths.end() || isBetterEscapePath(path, evaluation.score, previous->second))
		bestPaths[path.targetHero] = {path, evaluation.score};
}
}

EscapePathEvaluation evaluateEscapePathCandidate(const EscapePathCandidate & candidate)
{
	EscapePathEvaluation result;

	if(!candidate.currentTileThreatensHero
		|| !candidate.sameDay
		|| candidate.sameTile
		|| candidate.blockedAction
		|| !candidate.singleHeroPath
		|| !candidate.destinationSafe
		|| !candidate.destinationIsSafer)
	{
		return result;
	}

	if(candidate.hasOwnedTown && !candidate.destinationGetsCloserToOwnedTown)
		return result;

	result.accepted = true;
	result.score = candidate.threatReduction / std::max(0.1f, candidate.movementCost);
	return result;
}

std::string EscapeBehavior::toString() const
{
	return "Escape";
}

Goals::TGoalVec EscapeBehavior::decompose(const Nullkiller * aiNk) const
{
	Goals::TGoalVec tasks;
	HeroMap<HitMapInfo> threatenedHeroes;
	HeroMap<const CGTownInstance *> nearestOwnedTowns;
	const float safeAttackRatio = aiNk->settings->getSafeAttackRatio();

	for(const CGHeroInstance * candidateHero : aiNk->cc->getHeroesInfo())
	{
		if(aiNk->isHeroLocked(candidateHero))
			continue;

		const auto & threat = aiNk->dangerHitMap->getTileThreat(candidateHero->visitablePos()).fastestDanger;
		if(isHeroImmediatelyThreatened(candidateHero, threat, safeAttackRatio))
		{
			threatenedHeroes[candidateHero] = threat;
			nearestOwnedTowns[candidateHero] = findNearestOwnedTown(candidateHero, aiNk);
		}
	}

	if(threatenedHeroes.empty())
		return tasks;

	std::mutex sync;
	HeroMap<EscapePathChoice> bestPaths;
	const int3 mapSize = aiNk->cc->getMapSize();

	pforeachTilePaths(
		mapSize,
		aiNk,
		[&threatenedHeroes, &nearestOwnedTowns, aiNk, safeAttackRatio, &sync, &bestPaths](const int3 &, const std::vector<AIPath> & paths)
	{
		for(const AIPath & path : paths)
			considerEscapePath(path, threatenedHeroes, nearestOwnedTowns, aiNk, safeAttackRatio, sync, bestPaths);
	});

	for(const auto & bestPath : bestPaths)
		tasks.push_back(sptr(ExecuteHeroChain(bestPath.second.path)));

	return tasks;
}

}
