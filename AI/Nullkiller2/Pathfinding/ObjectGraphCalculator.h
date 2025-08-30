/*
* ObjectGraphCalculator.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/

#pragma once

#include "ObjectGraph.h"
#include "../AIUtility.h"

namespace NK2AI
{

struct ConnectionCostInfo
{
	float totalCost = 0;
	float avg = 0;
	int connectionsCount = 0;
};

class ObjectGraphCalculator
{
private:
	ObjectGraph * target;
	const Nullkiller * aiNk;
	std::mutex syncLock;

	std::map<const CGHeroInstance *, HeroRole> actors;
	std::map<const CGHeroInstance *, const CGObjectInstance *> actorObjectMap;

	std::vector<std::unique_ptr<CGBoat>> temporaryBoats;
	std::vector<std::unique_ptr<CGHeroInstance>> temporaryActorHeroes;

public:
	ObjectGraphCalculator(ObjectGraph * target, const Nullkiller * aiNk);
	void setGraphObjects();
	void calculateConnections();
	float getNeighborConnectionsCost(const int3 & pos, std::vector<AIPath> & pathCache);
	void addMinimalDistanceJunctions();

private:
	void updatePaths();
	void calculateConnections(const int3 & pos, std::vector<AIPath> & pathCache);
	bool isExtraConnection(float direct, float side1, float side2) const;
	void removeExtraConnections();
	void addObjectActor(const CGObjectInstance * obj);
	void addJunctionActor(const int3 & visitablePos, bool isVirtualBoat = false);
	ConnectionCostInfo getConnectionsCost(std::vector<AIPath> & paths) const;
};

}
