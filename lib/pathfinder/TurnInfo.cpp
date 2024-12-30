/*
 * TurnInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "TurnInfo.h"

#include "../IGameCallback.h"
#include "../IGameSettings.h"
#include "../TerrainHandler.h"
#include "../VCMI_Lib.h"
#include "../bonuses/BonusList.h"
#include "../json/JsonNode.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/MiscObjects.h"

VCMI_LIB_NAMESPACE_BEGIN

TConstBonusListPtr TurnInfoBonusList::getBonusList(const CGHeroInstance * target, const CSelector & bonusSelector)
{
	std::lock_guard guard(bonusListMutex);

	if (target->getTreeVersion() == bonusListVersion)
		return bonusList;

	bonusList = target->getBonuses(bonusSelector);
	bonusListVersion = target->getTreeVersion();

	return bonusList;
}

int TurnInfo::hasWaterWalking() const
{
	return waterWalkingTest;
}

int TurnInfo::hasFlyingMovement() const
{
	return flyingMovementTest;
}

int TurnInfo::hasNoTerrainPenalty(const TerrainId &terrain) const
{
	return noterrainPenalty[terrain.num];
}

int TurnInfo::hasFreeShipBoarding() const
{
	return freeShipBoardingTest;
}

int TurnInfo::getFlyingMovementValue() const
{
	return flyingMovementValue;
}

int TurnInfo::getWaterWalkingValue() const
{
	return waterWalkingValue;
}

int TurnInfo::getRoughTerrainDiscountValue() const
{
	return roughTerrainDiscountValue;
}

int TurnInfo::getMovePointsLimitLand() const
{
	return movePointsLimitLand;
}

int TurnInfo::getMovePointsLimitWater() const
{
	return movePointsLimitWater;
}

TurnInfo::TurnInfo(TurnInfoCache * sharedCache, const CGHeroInstance * target, int Turn)
	: noterrainPenalty(VLC->terrainTypeHandler->size())
	, target(target)
{
	CSelector daySelector = Selector::days(Turn);

	int lowestSpeed;
	if (target->getTreeVersion() == sharedCache->heroLowestSpeedVersion)
	{
		lowestSpeed = sharedCache->heroLowestSpeedValue;
	}
	else
	{
		lowestSpeed = target->getLowestCreatureSpeed();
		sharedCache->heroLowestSpeedValue = lowestSpeed;
		sharedCache->heroLowestSpeedVersion = target->getTreeVersion();
	}

	{
		static const CSelector selector = Selector::type()(BonusType::WATER_WALKING);
		const auto & bonuses = sharedCache->waterWalking.getBonusList(target, selector);
		waterWalkingTest = bonuses->getFirst(selector) != nullptr;
		waterWalkingValue = bonuses->valOfBonuses(selector);
	}

	{
		static const CSelector selector = Selector::type()(BonusType::FLYING_MOVEMENT);
		const auto & bonuses = sharedCache->flyingMovement.getBonusList(target, selector);
		flyingMovementTest = bonuses->getFirst(selector) != nullptr;
		flyingMovementValue = bonuses->valOfBonuses(selector);
	}

	{
		static const CSelector selector = Selector::type()(BonusType::FREE_SHIP_BOARDING);
		const auto & bonuses = sharedCache->freeShipBoarding.getBonusList(target, selector);
		freeShipBoardingTest = bonuses->getFirst(selector) != nullptr;
	}

	{
		static const CSelector selector = Selector::type()(BonusType::ROUGH_TERRAIN_DISCOUNT);
		const auto & bonuses = sharedCache->roughTerrainDiscount.getBonusList(target, selector);
		roughTerrainDiscountValue = bonuses->getFirst(selector) != nullptr;
	}

	{
		static const CSelector selector = Selector::typeSubtype(BonusType::MOVEMENT, BonusCustomSubtype::heroMovementSea);
		const auto & vectorSea = target->cb->getSettings().getValue(EGameSettings::HEROES_MOVEMENT_POINTS_SEA).Vector();
		const auto & bonuses = sharedCache->movementPointsLimitWater.getBonusList(target, selector);
		int baseMovementPointsSea;
		if (lowestSpeed < vectorSea.size())
			baseMovementPointsSea = vectorSea[lowestSpeed].Integer();
		else
			baseMovementPointsSea = vectorSea.back().Integer();

		movePointsLimitWater = bonuses->valOfBonuses(selector, baseMovementPointsSea);
	}

	{
		static const CSelector selector = Selector::typeSubtype(BonusType::MOVEMENT, BonusCustomSubtype::heroMovementSea);
		const auto & vectorLand = target->cb->getSettings().getValue(EGameSettings::HEROES_MOVEMENT_POINTS_LAND).Vector();
		const auto & bonuses = sharedCache->movementPointsLimitLand.getBonusList(target, selector);
		int baseMovementPointsLand;
		if (lowestSpeed < vectorLand.size())
			baseMovementPointsLand = vectorLand[lowestSpeed].Integer();
		else
			baseMovementPointsLand = vectorLand.back().Integer();

		movePointsLimitLand = bonuses->valOfBonuses(selector, baseMovementPointsLand);
	}

	{
		static const CSelector selector = Selector::type()(BonusType::NO_TERRAIN_PENALTY);
		const auto & bonuses = sharedCache->noTerrainPenalty.getBonusList(target, selector);
		for (const auto & bonus : *bonuses)
		{
			TerrainId affectedTerrain = bonus->subtype.as<TerrainId>();
			noterrainPenalty.at(affectedTerrain.num) = true;
		}
	}
}

bool TurnInfo::isLayerAvailable(const EPathfindingLayer & layer) const
{
	switch(layer.toEnum())
	{
	case EPathfindingLayer::AIR:
		if(target && target->boat && target->boat->layer == EPathfindingLayer::AIR)
			break;

		if(!hasFlyingMovement())
			return false;

		break;

	case EPathfindingLayer::WATER:
		if(target && target->boat && target->boat->layer == EPathfindingLayer::WATER)
			break;

		if(!hasWaterWalking())
			return false;

		break;
	}

	return true;
}

int TurnInfo::getMaxMovePoints(const EPathfindingLayer & layer) const
{
	return layer == EPathfindingLayer::SAIL ? getMovePointsLimitWater() : getMovePointsLimitLand();
}

VCMI_LIB_NAMESPACE_END
