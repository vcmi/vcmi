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

EscapePathCandidate makeEscapePathCandidate(
	const AIPath & path,
	const HitMapInfo & currentThreat,
	const HitMapInfo & destinationThreat,
	uint64_t destinationDanger,
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
	return candidate;
}

void considerEscapePath(
	const AIPath & path,
	const HeroMap<HitMapInfo> & threatenedHeroes,
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
	const auto candidate = makeEscapePathCandidate(
		path,
		currentThreat,
		destinationThreat,
		destinationDanger,
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
	const float safeAttackRatio = aiNk->settings->getSafeAttackRatio();

	for(const CGHeroInstance * candidateHero : aiNk->cc->getHeroesInfo())
	{
		if(aiNk->isHeroLocked(candidateHero))
			continue;

		const auto & threat = aiNk->dangerHitMap->getTileThreat(candidateHero->visitablePos()).fastestDanger;
		if(isHeroImmediatelyThreatened(candidateHero, threat, safeAttackRatio))
			threatenedHeroes[candidateHero] = threat;
	}

	if(threatenedHeroes.empty())
		return tasks;

	std::mutex sync;
	HeroMap<EscapePathChoice> bestPaths;
	const int3 mapSize = aiNk->cc->getMapSize();

	pforeachTilePaths(
		mapSize,
		aiNk,
		[&threatenedHeroes, aiNk, safeAttackRatio, &sync, &bestPaths](const int3 &, const std::vector<AIPath> & paths)
	{
		for(const AIPath & path : paths)
			considerEscapePath(path, threatenedHeroes, aiNk, safeAttackRatio, sync, bestPaths);
	});

	for(const auto & bestPath : bestPaths)
		tasks.push_back(sptr(ExecuteHeroChain(bestPath.second.path)));

	return tasks;
}

}
