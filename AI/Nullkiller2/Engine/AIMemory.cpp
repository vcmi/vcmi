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

namespace NK2AI
{

void AIMemory::removeFromMemory(const CGObjectInstance * obj)
{
	vstd::erase_if_present(visitableObjs, obj->id);
	vstd::erase_if_present(alreadyVisited, obj->id);

	//TODO: Find better way to handle hero boat removal
	if(const auto * hero = dynamic_cast<const CGHeroInstance *>(obj))
	{
		if(hero->inBoat())
		{
			vstd::erase_if_present(visitableObjs, hero->getBoat()->id);
			vstd::erase_if_present(alreadyVisited, hero->getBoat()->id);
		}
	}
}

void AIMemory::removeFromMemory(const ObjectIdRef obj)
{
	auto matchesId = [&](const ObjectInstanceID & objId) -> bool
	{
		return objId == obj.id;
	};

	vstd::erase_if(visitableObjs, matchesId);
	vstd::erase_if(alreadyVisited, matchesId);
}

void AIMemory::addSubterraneanGate(const CGObjectInstance * entrance, const CGObjectInstance * exit)
{
	knownSubterraneanGates[entrance] = exit;
	knownSubterraneanGates[exit] = entrance;

	logAi->trace("Found a pair of subterranean gates between %s and %s!", entrance->visitablePos().toString(), exit->visitablePos().toString());
}

void AIMemory::addVisitableObject(const CGObjectInstance * obj)
{
	visitableObjs.insert(obj->id);

	// All teleport objects seen automatically assigned to appropriate channels
	if(const auto * const teleportObj = dynamic_cast<const CGTeleport *>(obj))
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
		if(rewardable->configuration.getVisitMode() == Rewardable::VISIT_HERO) //we may want to visit it with another hero
			return;

		if(rewardable->configuration.getVisitMode() == Rewardable::VISIT_BONUS) //or another time
			return;
	}

	if(obj->ID == Obj::MONSTER)
		return;

	alreadyVisited.insert(obj->id);
}

void AIMemory::markObjectUnvisited(const CGObjectInstance * obj)
{
	vstd::erase_if_present(alreadyVisited, obj->id);
}

bool AIMemory::wasVisited(const CGObjectInstance * obj) const
{
	return vstd::contains(alreadyVisited, obj->id);
}

void AIMemory::removeInvisibleOrDeletedObjects(const CCallback & cc)
{
	auto shouldBeErased = [&](const ObjectInstanceID objId) -> bool
	{
		return !cc.getObj(objId, false);
	};

	vstd::erase_if(visitableObjs, shouldBeErased);
	vstd::erase_if(alreadyVisited, shouldBeErased);
}

std::vector<const CGObjectInstance *> AIMemory::visitableIdsToObjsVector(const CCallback & cc) const
{
	auto objs = std::vector<const CGObjectInstance *>();
	for(const ObjectInstanceID objId : visitableObjs)
	{
		if(const auto * obj = cc.getObjInstance(objId))
			objs.push_back(obj);
	}
	return objs;
}

std::set<const CGObjectInstance *> AIMemory::visitableIdsToObjsSet(const CCallback & cc) const
{
	auto objs = std::set<const CGObjectInstance *>();
	for(const ObjectInstanceID objId : visitableObjs)
	{
		if(const auto * obj = cc.getObjInstance(objId))
			objs.insert(obj);
	}
	return objs;
}

}
