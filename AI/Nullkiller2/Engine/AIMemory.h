/*
* AIMemory.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../AIUtility.h"
#include "../../../lib/mapObjects/MapObjects.h"

class JsonNode;

namespace NK2AI
{

class AIMemory
{
private:
	std::map<ObjectInstanceID, ObjectInstanceID> oneWayPortalReservations;
	std::set<ObjectInstanceID> probedOneWayPortals;
	std::map<ObjectInstanceID, int> oneWayPortalLastTraversalDay;
	std::map<ObjectInstanceID, std::set<ObjectInstanceID>> observedOneWayPortalExits;
	std::map<ObjectInstanceID, std::pair<uint64_t, uint64_t>> oneWayPortalGuardFailures;
	std::map<ObjectInstanceID, std::pair<ObjectInstanceID, ObjectInstanceID>> oneWayPortalJourneys;
	std::map<ObjectInstanceID, std::set<ObjectInstanceID>> oneWayPortalUnreturnedEntrances;
	std::set<ObjectInstanceID> oneWayPortalsWithKnownReturn;

public:
	std::set<ObjectInstanceID> visitableObjs;
	std::set<ObjectInstanceID> alreadyVisited;
	std::map<TeleportChannelID, std::shared_ptr<TeleportChannel>> knownTeleportChannels;
	std::map<const CGObjectInstance *, const CGObjectInstance *> knownSubterraneanGates;

	void removeFromMemory(const CGObjectInstance * obj);
	void removeFromMemory(ObjectIdRef obj);
	void addSubterraneanGate(const CGObjectInstance * entrance, const CGObjectInstance * exit);
	void addVisitableObject(const CGObjectInstance * obj);
	void markObjectVisited(const CGObjectInstance * obj);
	void markObjectUnvisited(const CGObjectInstance * obj);
	bool wasVisited(const CGObjectInstance * obj) const;
	void removeInvisibleOrDeletedObjects(const CCallback & cc);
	bool reserveOneWayPortal(ObjectInstanceID entrance, ObjectInstanceID hero);
	void clearOneWayPortalReservation(ObjectInstanceID entrance);
	std::optional<ObjectInstanceID> getOneWayPortalReservation(ObjectInstanceID entrance) const;
	void recordOneWayPortalTraversal(ObjectInstanceID entrance, ObjectInstanceID exit, ObjectInstanceID hero, int day);
	void recoverOneWayPortalTraversal(ObjectInstanceID entrance, ObjectInstanceID exit, ObjectInstanceID hero, int day);
	bool wasOneWayPortalProbed(ObjectInstanceID entrance) const;
	bool wasOneWayPortalProbedToday(ObjectInstanceID entrance, int day) const;
	bool hasKnownOneWayPortalReturn(ObjectInstanceID entrance) const;
	bool hasActiveOneWayPortalJourney(ObjectInstanceID entrance) const;
	std::vector<ObjectInstanceID> getUnreturnedOneWayPortalHeroes(ObjectInstanceID entrance) const;
	void recordOneWayPortalGuardFailure(
		ObjectInstanceID entrance,
		uint64_t guardDanger,
		uint64_t failedHeroStrength);
	std::optional<std::pair<uint64_t, uint64_t>> getOneWayPortalGuardFailure(ObjectInstanceID entrance) const;
	std::optional<std::pair<ObjectInstanceID, ObjectInstanceID>> getOneWayPortalJourney(ObjectInstanceID hero) const;
	bool completeOneWayPortalJourney(ObjectInstanceID hero);
	void markOneWayPortalReturn(ObjectInstanceID hero);
	void removeOneWayPortalHero(ObjectInstanceID hero);
	void resetOneWayPortalState();
	bool hasOneWayPortalState() const;
	void loadOneWayPortalState(const JsonNode & source);
	void saveOneWayPortalState(JsonNode & destination) const;
	// Utility method to reuse code, use visitableIds directly where possible to avoid time-of-check-to-time-of-use (TOCTOU) race condition
	std::vector<const CGObjectInstance *> visitableIdsToObjsVector(const CCallback & cc) const;
	// Utility method to reuse code, use visitableIds directly where possible to avoid time-of-check-to-time-of-use (TOCTOU) race condition
	std::set<const CGObjectInstance *> visitableIdsToObjsSet(const CCallback & cc) const;

private:
	void storeOneWayPortalTraversal(ObjectInstanceID entrance, ObjectInstanceID exit, ObjectInstanceID hero, int day);
	void removeOneWayPortalObject(ObjectInstanceID object);
};

}
