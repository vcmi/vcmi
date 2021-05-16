/*
* DangerHitMapAnalyzer.h, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#pragma once

#include "../Pathfinding/AINodeStorage.h"

struct ObjectInfo
{
	float priority;
	float movementCost;
	uint64_t danger;
	uint8_t turn;
};

struct ObjectCluster
{
public:
	std::map<const CGObjectInstance *, ObjectInfo> objects;
	const CGObjectInstance * blocker;

	void reset()
	{
		objects.clear();
	}

	void addObject(const CGObjectInstance * object, const AIPath & path, float priority);
	
	ObjectCluster(const CGObjectInstance * blocker)
		:objects(), blocker(blocker)
	{
	}

	ObjectCluster() : ObjectCluster(nullptr)
	{
	}

	std::vector<const CGObjectInstance *> getObjects() const;
	const CGObjectInstance * calculateCenter() const;
};

class ObjectClusterizer
{
private:
	ObjectCluster nearObjects;
	ObjectCluster farObjects;
	std::map<const CGObjectInstance *, std::shared_ptr<ObjectCluster>> blockedObjects;

public:
	void clusterize();
	std::vector<const CGObjectInstance *> getNearbyObjects() const;
	std::vector<const CGObjectInstance *> getFarObjects() const;
	std::vector<std::shared_ptr<ObjectCluster>> getLockedClusters() const;
	const CGObjectInstance * getBlocker(const AIPath & path) const;

	ObjectClusterizer()
		:nearObjects(), farObjects(), blockedObjects()
	{
	}
};
