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
#include "../bonuses/BonusSelector.h"
#include "../bonuses/BonusCache.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;

class TurnInfoBonusList
{
	TConstBonusListPtr bonusList;
	std::mutex bonusListMutex;
	std::atomic<int64_t> bonusListVersion = 0;
public:
	TConstBonusListPtr getBonusList(const CGHeroInstance * target, const CSelector & bonusSelector);
};

struct TurnInfoCache
{
	TurnInfoBonusList waterWalking;
	TurnInfoBonusList flyingMovement;
	TurnInfoBonusList noTerrainPenalty;
	TurnInfoBonusList freeShipBoarding;
	TurnInfoBonusList roughTerrainDiscount;
	TurnInfoBonusList baseTileMovementCost;
	TurnInfoBonusList movementPointsLimitLand;
	TurnInfoBonusList movementPointsLimitWater;

	const CGHeroInstance * target;

	mutable std::atomic<int64_t> heroLowestSpeedVersion = 0;
	mutable std::atomic<int64_t> heroLowestSpeedValue = 0;

	explicit TurnInfoCache(const CGHeroInstance * target):
		target(target)
	{}
};

class DLL_LINKAGE TurnInfo
{
private:
	const CGHeroInstance * target;

	// stores cached values per each terrain
	std::vector<bool> noterrainPenalty;

	int flyingMovementValue;
	int waterWalkingValue;
	int roughTerrainDiscountValue;
	int moveCostBaseValue;
	int movePointsLimitLand;
	int movePointsLimitWater;

	bool waterWalkingTest;
	bool flyingMovementTest;
	bool freeShipBoardingTest;

public:
	int hasWaterWalking() const;
	int hasFlyingMovement() const;
	int hasNoTerrainPenalty(const TerrainId & terrain) const;
	int hasFreeShipBoarding() const;

	int getFlyingMovementValue() const;
	int getWaterWalkingValue() const;
	int getRoughTerrainDiscountValue() const;
	int getMovementCostBase() const;
	int getMovePointsLimitLand() const;
	int getMovePointsLimitWater() const;

	TurnInfo(TurnInfoCache * sharedCache, const CGHeroInstance * target, int Turn);
	bool isLayerAvailable(const EPathfindingLayer & layer) const;
	int getMaxMovePoints(const EPathfindingLayer & layer) const;
};

VCMI_LIB_NAMESPACE_END
