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
		removeOneWayPortalHero(hero->id);

		if(hero->inBoat())
		{
			vstd::erase_if_present(visitableObjs, hero->getBoat()->id);
			vstd::erase_if_present(alreadyVisited, hero->getBoat()->id);
		}
	}
	else
	{
		removeOneWayPortalObject(obj->id);
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
	removeOneWayPortalHero(obj.id);
	removeOneWayPortalObject(obj.id);
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

	vstd::erase_if(oneWayPortalReservations, [&](const auto & reservation)
	{
		return !cc.getHero(reservation.second);
	});
	vstd::erase_if(oneWayPortalJourneys, [&](const auto & journey)
	{
		return !cc.getHero(journey.first);
	});
	vstd::erase_if(oneWayPortalUnreturnedEntrances, [&](const auto & journey)
	{
		return !cc.getHero(journey.first);
	});
}

bool AIMemory::reserveOneWayPortal(ObjectInstanceID entrance, ObjectInstanceID hero)
{
	const auto reservation = oneWayPortalReservations.find(entrance);
	if(reservation != oneWayPortalReservations.end() && reservation->second != hero)
		return false;

	oneWayPortalReservations[entrance] = hero;
	logAi->debug("Reserved one-way portal %d for hero %d", entrance.getNum(), hero.getNum());
	return true;
}

void AIMemory::clearOneWayPortalReservation(ObjectInstanceID entrance)
{
	if(oneWayPortalReservations.erase(entrance))
		logAi->debug("Cleared reservation for one-way portal %d", entrance.getNum());
}

std::optional<ObjectInstanceID> AIMemory::getOneWayPortalReservation(ObjectInstanceID entrance) const
{
	const auto reservation = oneWayPortalReservations.find(entrance);
	if(reservation == oneWayPortalReservations.end())
		return std::nullopt;

	return reservation->second;
}

void AIMemory::recordOneWayPortalTraversal(ObjectInstanceID entrance, ObjectInstanceID exit, ObjectInstanceID hero, int day)
{
	clearOneWayPortalReservation(entrance);
	probedOneWayPortals.insert(entrance);
	oneWayPortalLastTraversalDay[entrance] = day;
	observedOneWayPortalExits[entrance].insert(exit);
	oneWayPortalJourneys[hero] = {entrance, exit};
	oneWayPortalUnreturnedEntrances[hero].insert(entrance);

	logAi->info(
		"One-way portal probe: hero %d traveled from entrance %d to actual exit %d",
		hero.getNum(),
		entrance.getNum(),
		exit.getNum());
}

bool AIMemory::wasOneWayPortalProbed(ObjectInstanceID entrance) const
{
	return vstd::contains(probedOneWayPortals, entrance);
}

bool AIMemory::wasOneWayPortalProbedToday(ObjectInstanceID entrance, int day) const
{
	const auto traversal = oneWayPortalLastTraversalDay.find(entrance);
	return traversal != oneWayPortalLastTraversalDay.end() && traversal->second == day;
}

bool AIMemory::hasKnownOneWayPortalReturn(ObjectInstanceID entrance) const
{
	return vstd::contains(oneWayPortalsWithKnownReturn, entrance);
}

std::optional<std::pair<ObjectInstanceID, ObjectInstanceID>> AIMemory::getOneWayPortalJourney(ObjectInstanceID hero) const
{
	const auto journey = oneWayPortalJourneys.find(hero);
	if(journey == oneWayPortalJourneys.end())
		return std::nullopt;

	return journey->second;
}

void AIMemory::markOneWayPortalReturn(ObjectInstanceID hero)
{
	const auto unreturned = oneWayPortalUnreturnedEntrances.find(hero);
	if(unreturned == oneWayPortalUnreturnedEntrances.end())
		return;

	for(const auto entrance : unreturned->second)
	{
		oneWayPortalsWithKnownReturn.insert(entrance);
		logAi->info(
			"Hero %d demonstrated a return to an owned town after probing one-way portal %d",
			hero.getNum(),
			entrance.getNum());
	}

	oneWayPortalJourneys.erase(hero);
	oneWayPortalUnreturnedEntrances.erase(unreturned);
}

void AIMemory::removeOneWayPortalHero(ObjectInstanceID hero)
{
	vstd::erase_if(oneWayPortalReservations, [&](const auto & reservation)
	{
		return reservation.second == hero;
	});
	oneWayPortalJourneys.erase(hero);
	oneWayPortalUnreturnedEntrances.erase(hero);
}

void AIMemory::resetOneWayPortalState()
{
	oneWayPortalReservations.clear();
	probedOneWayPortals.clear();
	oneWayPortalLastTraversalDay.clear();
	observedOneWayPortalExits.clear();
	oneWayPortalJourneys.clear();
	oneWayPortalUnreturnedEntrances.clear();
	oneWayPortalsWithKnownReturn.clear();
}

void AIMemory::removeOneWayPortalObject(ObjectInstanceID object)
{
	oneWayPortalReservations.erase(object);
	probedOneWayPortals.erase(object);
	oneWayPortalLastTraversalDay.erase(object);
	observedOneWayPortalExits.erase(object);
	oneWayPortalsWithKnownReturn.erase(object);

	for(auto & observed : observedOneWayPortalExits)
		observed.second.erase(object);

	vstd::erase_if(oneWayPortalJourneys, [&](const auto & journey)
	{
		return journey.second.first == object || journey.second.second == object;
	});
	vstd::erase_if(oneWayPortalUnreturnedEntrances, [&](auto & journey)
	{
		journey.second.erase(object);
		return journey.second.empty();
	});
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
