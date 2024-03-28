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
#include "../Engine/PriorityEvaluator.h"

namespace NKAI
{

struct ClusterObjectInfo
{
	float priority;
	float movementCost;
	uint64_t danger;
	uint8_t turn;
};

using ClusterObjects = tbb::concurrent_hash_map<const CGObjectInstance *, ClusterObjectInfo>;

struct ObjectCluster
{
public:
	ClusterObjects objects;
	const CGObjectInstance * blocker;

	void reset()
	{
		objects.clear();
	}

	void addObject(const CGObjectInstance * object, const AIPath & path, float priority);

	ObjectCluster(const CGObjectInstance * blocker): blocker(blocker) {}

	ObjectCluster() : ObjectCluster(nullptr)
	{
	}

	std::vector<const CGObjectInstance *> getObjects() const;
	const CGObjectInstance * calculateCenter() const;
};

using ClusterMap = tbb::concurrent_hash_map<const CGObjectInstance *, std::shared_ptr<ObjectCluster>>;

class ObjectClusterizer
{
private:
	static Obj IgnoredObjectTypes[];

	ObjectCluster nearObjects;
	ObjectCluster farObjects;
	ClusterMap blockedObjects;
	const Nullkiller * ai;
	RewardEvaluator valueEvaluator;

public:
	void clusterize();
	std::vector<const CGObjectInstance *> getNearbyObjects() const;
	std::vector<const CGObjectInstance *> getFarObjects() const;
	std::vector<std::shared_ptr<ObjectCluster>> getLockedClusters() const;
	const CGObjectInstance * getBlocker(const AIPath & path) const;
	std::optional<const CGObjectInstance *> getBlocker(const AIPathNodeInfo & node) const;

	ObjectClusterizer(const Nullkiller * ai): ai(ai), valueEvaluator(ai) {}

private:
	bool shouldVisitObject(const CGObjectInstance * obj) const;
	void clusterizeObject(
		const CGObjectInstance * obj,
		PriorityEvaluator * priorityEvaluator,
		std::vector<AIPath> & pathCache,
		std::vector<const CGHeroInstance *> & heroes);
};

}
