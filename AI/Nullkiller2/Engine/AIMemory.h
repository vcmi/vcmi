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

namespace NK2AI
{

class AIMemory
{
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
	// Utility method to reuse code, use visitableIds directly where possible to avoid time-of-check-to-time-of-use (TOCTOU) race condition
	std::vector<const CGObjectInstance *> visitableIdsToObjsVector(const CCallback & cc) const;
	// Utility method to reuse code, use visitableIds directly where possible to avoid time-of-check-to-time-of-use (TOCTOU) race condition
	std::set<const CGObjectInstance *> visitableIdsToObjsSet(const CCallback & cc) const;
};

}
