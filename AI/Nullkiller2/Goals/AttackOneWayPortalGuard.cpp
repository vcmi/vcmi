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

	const auto * exitObject = aiNk->cc->tryGetObj(journey->second);
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

	if(!aiNk->cc->isVisible(trappingGuardPosition))
		return nullptr;

	return findObjectOfType(aiNk->cc->getVisitableObjs(trappingGuardPosition), Obj::MONSTER);
}

void AttackOneWayPortalGuard::accept(AIGateway * aiGw)
{
	const HeroPtr heroPtr(hero, aiGw->cc.get());
	if(!heroPtr.isVerified())
		throw cannotFulfillGoalException("One-way portal scout is no longer available.");

	const auto * currentGuard = findTrappingGuard(hero, aiGw->nullkiller.get());
	if(!currentGuard || currentGuard->id != ObjectInstanceID(objid))
		throw cannotFulfillGoalException("One-way portal exit is no longer blocked by the recorded guard.");

	logAi->info(
		"One-way portal intent: forced exit fight by hero %s against guard %d",
		hero->getNameTextID(),
		objid);

	if(aiGw->moveHeroToTile(tile, heroPtr) != HeroMovementResult::COMPLETE)
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
