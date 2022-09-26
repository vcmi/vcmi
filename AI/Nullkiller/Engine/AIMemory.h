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

namespace NKAI
{

class AIMemory
{
public:
	std::set<const CGObjectInstance *> visitableObjs;
	std::set<const CGObjectInstance *> alreadyVisited;
	std::map<TeleportChannelID, std::shared_ptr<TeleportChannel>> knownTeleportChannels;
	std::map<const CGObjectInstance *, const CGObjectInstance *> knownSubterraneanGates;

	void removeFromMemory(const CGObjectInstance * obj);
	void removeFromMemory(ObjectIdRef obj);
	void addSubterraneanGate(const CGObjectInstance * entrance, const CGObjectInstance * exit);
	void addVisitableObject(const CGObjectInstance * obj);
	void markObjectVisited(const CGObjectInstance * obj);
	void markObjectUnvisited(const CGObjectInstance * obj);
	bool wasVisited(const CGObjectInstance * obj) const;
	void removeInvisibleObjects(CCallback * cb);
};

}
