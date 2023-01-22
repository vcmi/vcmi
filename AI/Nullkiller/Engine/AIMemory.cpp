/*
* AIMemory.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "../StdInc.h"
#include "AIMemory.h"
#include "../../../CCallback.h"

namespace NKAI
{

void AIMemory::removeFromMemory(const CGObjectInstance * obj)
{
	vstd::erase_if_present(visitableObjs, obj);
	vstd::erase_if_present(alreadyVisited, obj);

	//TODO: Find better way to handle hero boat removal
	if(auto hero = dynamic_cast<const CGHeroInstance *>(obj))
	{
		if(hero->boat)
		{
			vstd::erase_if_present(visitableObjs, hero->boat);
			vstd::erase_if_present(alreadyVisited, hero->boat);
		}
	}
}

void AIMemory::removeFromMemory(ObjectIdRef obj)
{
	auto matchesId = [&](const CGObjectInstance * hlpObj) -> bool
	{
		return hlpObj->id == obj.id;
	};

	vstd::erase_if(visitableObjs, matchesId);
	vstd::erase_if(alreadyVisited, matchesId);
}

void AIMemory::addSubterraneanGate(const CGObjectInstance * entrance, const CGObjectInstance * exit)
{
	knownSubterraneanGates[entrance] = exit;
	knownSubterraneanGates[exit] = entrance;

	logAi->trace(
		"Found a pair of subterranean gates between %s and %s!",
		entrance->visitablePos().toString(),
		exit->visitablePos().toString());
}

void AIMemory::addVisitableObject(const CGObjectInstance * obj)
{
	visitableObjs.insert(obj);

	// All teleport objects seen automatically assigned to appropriate channels
	auto teleportObj = dynamic_cast<const CGTeleport *>(obj);
	if(teleportObj)
	{
		CGTeleport::addToChannel(knownTeleportChannels, teleportObj);
	}
}

void AIMemory::markObjectVisited(const CGObjectInstance * obj)
{
	if(!obj)
		return;
	
	// TODO: maybe this logic belongs to CaptureObjects::shouldVisit
	if(const auto * rewardable = dynamic_cast<const CRewardableObject *>(obj))
	{
		if (rewardable->getVisitMode() == CRewardableObject::VISIT_HERO) //we may want to visit it with another hero
			return;

		if (rewardable->getVisitMode() == CRewardableObject::VISIT_BONUS) //or another time
			return;
	}

	if(dynamic_cast<const CGVisitableOPH *>(obj)) //we may want to visit it with another hero
		return;
	
	if(dynamic_cast<const CGBonusingObject *>(obj)) //or another time
		return;
	
	if(obj->ID == Obj::MONSTER)
		return;

	alreadyVisited.insert(obj);
}

void AIMemory::markObjectUnvisited(const CGObjectInstance * obj)
{
	vstd::erase_if_present(alreadyVisited, obj);
}

bool AIMemory::wasVisited(const CGObjectInstance * obj) const
{
	return vstd::contains(alreadyVisited, obj);
}

void AIMemory::removeInvisibleObjects(CCallback * cb)
{
	auto shouldBeErased = [&](const CGObjectInstance * obj) -> bool
	{
		if(obj)
			return !cb->getObj(obj->id, false); // no verbose output needed as we check object visibility
		else
			return true;
	};

	vstd::erase_if(visitableObjs, shouldBeErased);
	vstd::erase_if(alreadyVisited, shouldBeErased);
}

}
