/*
* AttackOneWayPortalGuard.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"

#include "AttackOneWayPortalGuard.h"

#include "../AIGateway.h"
#include "../Engine/Nullkiller.h"

namespace NK2AI
{

using namespace Goals;

namespace
{
const CGObjectInstance * findMonsterAt(const Nullkiller * aiNk, const int3 & tile)
{
	if(!aiNk->cc->isVisible(tile))
		return nullptr;

	const auto objects = aiNk->cc->getVisitableObjs(tile, false);
	const auto guard = std::find_if(objects.begin(), objects.end(), [](const CGObjectInstance * object)
	{
		return object->ID == Obj::MONSTER;
	});

	return guard != objects.end() ? *guard : nullptr;
}

bool isDesperationHero(const CGHeroInstance * hero, const Nullkiller * aiNk)
{
	const auto heroes = aiNk->cc->getHeroesInfo();
	return aiNk->cc->getTownsInfo().empty()
		&& heroes.size() == 1
		&& heroes.front() == hero;
}
}

AttackOneWayPortalGuard::AttackOneWayPortalGuard(
	const CGHeroInstance * hero,
	const CGObjectInstance * guard,
	const CGObjectInstance * exit)
	: ElementarGoal(Goals::ATTACK_ONE_WAY_PORTAL_GUARD)
	, exit(exit->id)
{
	this->hero = hero;
	objid = guard->id.getNum();
	tile = guard->visitablePos();
}

const CGObjectInstance * AttackOneWayPortalGuard::findTrappingGuard(
	const CGHeroInstance * hero,
	const Nullkiller * aiNk)
{
	if(!hero || hero->movementPointsRemaining() == 0)
		return nullptr;

	const auto journey = aiNk->memory->getOneWayPortalJourney(hero->id);
	if(!journey)
		return nullptr;

	const auto * exitObject = aiNk->cc->getObj(journey->second, false);
	if(!exitObject
		|| exitObject->ID != Obj::MONOLITH_ONE_WAY_EXIT
		|| exitObject->visitablePos() != hero->visitablePos())
	{
		return nullptr;
	}

	const auto pathsInfo = aiNk->getPathsInfo(hero);
	int3 trappingGuardPosition(-1);
	bool foundUsableMove = false;

	for(const int3 & direction : int3::getDirs())
	{
		const int3 destination = hero->visitablePos() + direction;
		if(!aiNk->cc->isInTheMap(destination))
			continue;

		const auto * pathInfo = pathsInfo->getPathInfo(destination);
		if(!pathInfo->reachable() || pathInfo->turns != 0)
			continue;

		foundUsableMove = true;
		const int3 guardPosition = aiNk->cc->getGuardingCreaturePosition(destination);
		if(!guardPosition.isValid())
			return nullptr;

		if(!trappingGuardPosition.isValid())
			trappingGuardPosition = guardPosition;
		else if(trappingGuardPosition != guardPosition)
			return nullptr;
	}

	if(!foundUsableMove || !trappingGuardPosition.isValid())
		return nullptr;

	return findMonsterAt(aiNk, trappingGuardPosition);
}

void AttackOneWayPortalGuard::accept(AIGateway * aiGw)
{
	const HeroPtr heroPtr(hero, aiGw->cc.get());
	if(!heroPtr.isVerified())
		throw cannotFulfillGoalException("One-way portal scout is no longer available.");

	const auto journey = aiGw->nullkiller->memory->getOneWayPortalJourney(hero->id);
	if(!journey || journey->second != exit)
		throw cannotFulfillGoalException("Hero is no longer at the recorded one-way portal exit.");

	const auto * currentGuard = findTrappingGuard(hero, aiGw->nullkiller.get());
	if(!currentGuard || currentGuard->id != ObjectInstanceID(objid))
		throw cannotFulfillGoalException("One-way portal exit is no longer blocked by the recorded guard.");

	if(hero->isMissionCritical() && !isDesperationHero(hero, aiGw->nullkiller.get()))
		throw cannotFulfillGoalException("Mission-critical hero cannot make a forced portal-exit attack.");

	logAi->info(
		"One-way portal intent: forced exit fight by hero %s against guard %d",
		hero->getNameTextID(),
		objid);

	const uint64_t guardDanger = aiGw->nullkiller->dangerEvaluator->evaluateDanger(currentGuard);
	const uint64_t heroStrength = static_cast<uint64_t>(
		getNormalizedHeroStrength(hero) * hero->getArmyStrength());
	auto recordFailureIfHeroWasLost = [&]()
	{
		if(heroPtr.isVerified(false))
			return;

		aiGw->nullkiller->memory->recordOneWayPortalGuardFailure(
			journey->first,
			guardDanger,
			heroStrength);
		aiGw->oneWayPortalStateDirty = true;
	};

	HeroMovementResult movementResult = HeroMovementResult::BLOCKED;
	try
	{
		movementResult = aiGw->moveHeroToTile(tile, heroPtr);
	}
	catch(...)
	{
		recordFailureIfHeroWasLost();
		throw;
	}

	recordFailureIfHeroWasLost();
	if(movementResult != HeroMovementResult::COMPLETE)
		throw cannotFulfillGoalException("Unable to attack the one-way portal exit guard this turn.");
}

std::string AttackOneWayPortalGuard::toString() const
{
	return "Attack one-way portal exit guard at " + tile.toString()
		+ " by " + hero->getNameTextID();
}

bool AttackOneWayPortalGuard::operator==(const AttackOneWayPortalGuard & other) const
{
	return hero == other.hero && objid == other.objid && exit == other.exit;
}

std::vector<ObjectInstanceID> AttackOneWayPortalGuard::getAffectedObjects() const
{
	return {hero->id, ObjectInstanceID(objid), exit};
}

bool AttackOneWayPortalGuard::isObjectAffected(ObjectInstanceID id) const
{
	return hero->id == id || ObjectInstanceID(objid) == id || exit == id;
}

}
