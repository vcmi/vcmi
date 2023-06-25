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

#include "../TerrainHandler.h"
#include "../VCMI_Lib.h"
#include "../bonuses/BonusList.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/MiscObjects.h"

VCMI_LIB_NAMESPACE_BEGIN

TurnInfo::BonusCache::BonusCache(const TConstBonusListPtr & bl)
{
	for(const auto & terrain : VLC->terrainTypeHandler->objects)
	{
		noTerrainPenalty.push_back(static_cast<bool>(
				bl->getFirst(Selector::type()(BonusType::NO_TERRAIN_PENALTY).And(Selector::subtype()(terrain->getIndex())))));
	}

	freeShipBoarding = static_cast<bool>(bl->getFirst(Selector::type()(BonusType::FREE_SHIP_BOARDING)));
	flyingMovement = static_cast<bool>(bl->getFirst(Selector::type()(BonusType::FLYING_MOVEMENT)));
	flyingMovementVal = bl->valOfBonuses(Selector::type()(BonusType::FLYING_MOVEMENT));
	waterWalking = static_cast<bool>(bl->getFirst(Selector::type()(BonusType::WATER_WALKING)));
	waterWalkingVal = bl->valOfBonuses(Selector::type()(BonusType::WATER_WALKING));
	pathfindingVal = bl->valOfBonuses(Selector::type()(BonusType::ROUGH_TERRAIN_DISCOUNT));
}

TurnInfo::TurnInfo(const CGHeroInstance * Hero, const int turn):
	hero(Hero),
	maxMovePointsLand(-1),
	maxMovePointsWater(-1),
	turn(turn)
{
	bonuses = hero->getAllBonuses(Selector::days(turn), Selector::all, nullptr, "");
	bonusCache = std::make_unique<BonusCache>(bonuses);
	nativeTerrain = hero->getNativeTerrain();
}

bool TurnInfo::isLayerAvailable(const EPathfindingLayer & layer) const
{
	switch(layer)
	{
	case EPathfindingLayer::AIR:
		if(hero && hero->boat && hero->boat->layer == EPathfindingLayer::AIR)
			break;

		if(!hasBonusOfType(BonusType::FLYING_MOVEMENT))
			return false;

		break;

	case EPathfindingLayer::WATER:
		if(hero && hero->boat && hero->boat->layer == EPathfindingLayer::WATER)
			break;

		if(!hasBonusOfType(BonusType::WATER_WALKING))
			return false;

		break;
	}

	return true;
}

bool TurnInfo::hasBonusOfType(BonusType type, int subtype) const
{
	switch(type)
	{
	case BonusType::FREE_SHIP_BOARDING:
		return bonusCache->freeShipBoarding;
	case BonusType::FLYING_MOVEMENT:
		return bonusCache->flyingMovement;
	case BonusType::WATER_WALKING:
		return bonusCache->waterWalking;
	case BonusType::NO_TERRAIN_PENALTY:
		return bonusCache->noTerrainPenalty[subtype];
	}

	return static_cast<bool>(
			bonuses->getFirst(Selector::type()(type).And(Selector::subtype()(subtype))));
}

int TurnInfo::valOfBonuses(BonusType type, int subtype) const
{
	switch(type)
	{
	case BonusType::FLYING_MOVEMENT:
		return bonusCache->flyingMovementVal;
	case BonusType::WATER_WALKING:
		return bonusCache->waterWalkingVal;
	case BonusType::ROUGH_TERRAIN_DISCOUNT:
		return bonusCache->pathfindingVal;
	}

	return bonuses->valOfBonuses(Selector::type()(type).And(Selector::subtype()(subtype)));
}

int TurnInfo::getMaxMovePoints(const EPathfindingLayer & layer) const
{
	if(maxMovePointsLand == -1)
		maxMovePointsLand = hero->movementPointsLimitCached(true, this);
	if(maxMovePointsWater == -1)
		maxMovePointsWater = hero->movementPointsLimitCached(false, this);

	return layer == EPathfindingLayer::SAIL ? maxMovePointsWater : maxMovePointsLand;
}

void TurnInfo::updateHeroBonuses(BonusType type, const CSelector& sel) const
{
	switch(type)
	{
	case BonusType::FREE_SHIP_BOARDING:
		bonusCache->freeShipBoarding = static_cast<bool>(bonuses->getFirst(Selector::type()(BonusType::FREE_SHIP_BOARDING)));
		break;
	case BonusType::FLYING_MOVEMENT:
		bonusCache->flyingMovement = static_cast<bool>(bonuses->getFirst(Selector::type()(BonusType::FLYING_MOVEMENT)));
		bonusCache->flyingMovementVal = bonuses->valOfBonuses(Selector::type()(BonusType::FLYING_MOVEMENT));
		break;
	case BonusType::WATER_WALKING:
		bonusCache->waterWalking = static_cast<bool>(bonuses->getFirst(Selector::type()(BonusType::WATER_WALKING)));
		bonusCache->waterWalkingVal = bonuses->valOfBonuses(Selector::type()(BonusType::WATER_WALKING));
		break;
	case BonusType::ROUGH_TERRAIN_DISCOUNT:
		bonusCache->pathfindingVal = bonuses->valOfBonuses(Selector::type()(BonusType::ROUGH_TERRAIN_DISCOUNT));
		break;
	default:
		bonuses = hero->getAllBonuses(Selector::days(turn), Selector::all, nullptr, "");
	}
}

VCMI_LIB_NAMESPACE_END
