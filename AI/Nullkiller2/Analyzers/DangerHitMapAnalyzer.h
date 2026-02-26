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

#include "../AIUtility.h"
#include "../Pathfinding/AIPathfinder.h"

namespace NK2AI
{

struct AIPath;

struct HitMapInfo
{
	static const HitMapInfo NoThreat;

	uint64_t danger = 0;
	uint8_t turn = PathfinderSettings::MaxTurnDistanceLimit;
	float threat = 0.f;
	HeroPtr heroPtr = HeroPtr(nullptr, nullptr);

	double value() const
	{
		return danger / std::sqrt(turn / 3.0f + 1);
	}
};

struct HitMapNode
{
	// threat = path.getHeroStrength() * (1 - path.movementCost() / 2.0); danger = path.getHeroStrength()
	HitMapInfo maximumDanger = HitMapInfo::NoThreat;
	// threat = path.getHeroStrength() * (1 - path.movementCost() / 2.0); danger = path.getHeroStrength()
	HitMapInfo fastestDanger = HitMapInfo::NoThreat;
	const CGTownInstance * closestTown = nullptr;

	void reset()
	{
		maximumDanger = HitMapInfo::NoThreat;
		fastestDanger = HitMapInfo::NoThreat;
		closestTown = nullptr;
	}
};

struct EnemyHeroAccessibleObject
{
	const CGHeroInstance * hero;
	const CGObjectInstance * obj;

	EnemyHeroAccessibleObject(const CGHeroInstance * hero, const CGObjectInstance * obj)
		:hero(hero), obj(obj)
	{
	}
};

class DangerHitMapAnalyzer
{
private:
	boost::multi_array<HitMapNode, 3> hitMap;
	tbb::concurrent_vector<EnemyHeroAccessibleObject> enemyHeroAccessibleObjects;
	bool hitMapUpToDate = false;
	bool tileOwnersUpToDate = false;
	const Nullkiller * aiNk;
	std::map<ObjectInstanceID, std::vector<HitMapInfo>> townThreats;

public:
	DangerHitMapAnalyzer(const Nullkiller * aiNk) :aiNk(aiNk) {}

	void updateHitMap();
	void calculateTileOwners();
	uint64_t enemyCanKillOurHeroesAlongThePath(const AIPath & path) const;
	const HitMapNode & getObjectThreat(const CGObjectInstance * obj) const;
	const HitMapNode & getTileThreat(const int3 & tile) const;
	std::set<const CGObjectInstance *> getOneTurnAccessibleObjects(const CGHeroInstance * enemy) const;
	void resetHitmap() {hitMapUpToDate = false;}
	bool isHitMapUpToDate() const { return hitMapUpToDate; }
	void resetTileOwners() { tileOwnersUpToDate = false; }
	bool isTileOwnersUpToDate() const { return tileOwnersUpToDate; }
	PlayerColor getTileOwner(const int3 & tile) const;
	const CGTownInstance * getClosestTown(const int3 & tile) const;
	const std::vector<HitMapInfo> & getTownThreats(const CGTownInstance * town) const;
};

}
