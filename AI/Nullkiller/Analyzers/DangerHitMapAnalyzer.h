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

namespace NKAI
{

struct AIPath;

struct HitMapInfo
{
	static HitMapInfo NoTreat;

	uint64_t danger;
	uint8_t turn;
	HeroPtr hero;

	HitMapInfo()
	{
		reset();
	}

	void reset()
	{
		danger = 0;
		turn = 255;
		hero = HeroPtr();
	}

	double value() const;
};

struct HitMapNode
{
	HitMapInfo maximumDanger;
	HitMapInfo fastestDanger;

	const CGTownInstance * closestTown = nullptr;

	HitMapNode() = default;

	void reset()
	{
		maximumDanger.reset();
		fastestDanger.reset();
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
	const Nullkiller * ai;
	std::map<ObjectInstanceID, std::vector<HitMapInfo>> townTreats;

public:
	DangerHitMapAnalyzer(const Nullkiller * ai) :ai(ai) {}

	void updateHitMap();
	void calculateTileOwners();
	uint64_t enemyCanKillOurHeroesAlongThePath(const AIPath & path) const;
	const HitMapNode & getObjectTreat(const CGObjectInstance * obj) const;
	const HitMapNode & getTileTreat(const int3 & tile) const;
	std::set<const CGObjectInstance *> getOneTurnAccessibleObjects(const CGHeroInstance * enemy) const;
	void reset();
	void resetTileOwners() { tileOwnersUpToDate = false; }
	PlayerColor getTileOwner(const int3 & tile) const;
	const CGTownInstance * getClosestTown(const int3 & tile) const;
	const std::vector<HitMapInfo> & getTownTreats(const CGTownInstance * town) const;
};

}
