/*
 * TurnInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../bonuses/Bonus.h"
#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;

struct DLL_LINKAGE TurnInfo
{
	/// This is certainly not the best design ever and certainly can be improved
	/// Unfortunately for pathfinder that do hundreds of thousands calls onus system add too big overhead
	struct BonusCache {
		std::vector<bool> noTerrainPenalty;
		bool freeShipBoarding;
		bool flyingMovement;
		int flyingMovementVal;
		bool waterWalking;
		int waterWalkingVal;
		int pathfindingVal;

		BonusCache(const TConstBonusListPtr & bonusList);
	};
	std::unique_ptr<BonusCache> bonusCache;

	const CGHeroInstance * hero;
	mutable TConstBonusListPtr bonuses;
	mutable int maxMovePointsLand;
	mutable int maxMovePointsWater;
	TerrainId nativeTerrain;
	int turn;

	TurnInfo(const CGHeroInstance * Hero, const int Turn = 0);
	bool isLayerAvailable(const EPathfindingLayer & layer) const;
	bool hasBonusOfType(const BonusType type, const int subtype = -1) const;
	int valOfBonuses(const BonusType type, const int subtype = -1) const;
	void updateHeroBonuses(BonusType type, const CSelector& sel) const;
	int getMaxMovePoints(const EPathfindingLayer & layer) const;
};

VCMI_LIB_NAMESPACE_END
