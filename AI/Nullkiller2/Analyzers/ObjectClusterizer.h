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

namespace NK2AI
{

static constexpr float MINIMUM_STRATEGICAL_VALUE_NON_TOWN = 0.3f;

struct ClusterObjectInfo
{
	float priority = 0.f;
	float movementCost = 0.f;
	uint64_t danger = 0;
	uint8_t turn = 0;
};

struct ObjectInstanceIDHash
{
	ObjectInstanceID::hash hash;
	bool equal(ObjectInstanceID o1, ObjectInstanceID o2) const
	{
		return o1 == o2;
	}
};
using ClusterObjects = tbb::concurrent_hash_map<ObjectInstanceID, ClusterObjectInfo, ObjectInstanceIDHash>;

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

	std::vector<const CGObjectInstance *> getObjects(const CPlayerSpecificInfoCallback * cpsic) const;
	const CGObjectInstance * calculateCenter(const CPlayerSpecificInfoCallback * cpsic) const;
};

using ClusterMap = tbb::concurrent_hash_map<ObjectInstanceID, std::shared_ptr<ObjectCluster>, ObjectInstanceIDHash>;

class ObjectClusterizer
{
private:
	static Obj IgnoredObjectTypes[];

	ObjectCluster nearObjects;
	ObjectCluster farObjects;
	ClusterMap blockedObjects;
	const Nullkiller * aiNk;
	RewardEvaluator valueEvaluator;
	bool isUpToDate;
	std::vector<ObjectInstanceID> invalidated;

public:
	void clusterize();
	std::vector<const CGObjectInstance *> getNearbyObjects() const;
	std::vector<const CGObjectInstance *> getFarObjects() const;
	std::vector<std::shared_ptr<ObjectCluster>> getLockedClusters() const;
	const CGObjectInstance * getBlocker(const AIPath & path) const;
	std::optional<const CGObjectInstance *> getBlocker(const AIPathNodeInfo & node) const;

	ObjectClusterizer(const Nullkiller * aiNk): aiNk(aiNk), valueEvaluator(aiNk), isUpToDate(false){}

	void validateObjects();
	void onObjectRemoved(ObjectInstanceID id);
	void invalidate(ObjectInstanceID id);

	void reset() {
		isUpToDate = false;
		invalidated.clear();
	}

private:
	bool shouldVisitObject(const CGObjectInstance * obj) const;
	void clusterizeObject(
		const CGObjectInstance * obj,
		PriorityEvaluator * priorityEvaluator,
		std::vector<AIPath> & pathCache,
		std::vector<const CGHeroInstance *> & heroes);
};

}
