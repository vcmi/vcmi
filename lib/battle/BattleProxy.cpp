/*
 * BattleProxy.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleProxy.h"
#include "Unit.h"

VCMI_LIB_NAMESPACE_BEGIN

///BattleProxy

BattleProxy::BattleProxy(Subject subject_): 
	subject(std::move(subject_))
{
	setBattle(this);
	player = subject->getPlayerID();
}

BattleProxy::~BattleProxy() = default;

int32_t BattleProxy::getActiveStackID() const
{
	const auto * ret = subject->battleActiveUnit();
	if(ret)
		return ret->unitId();
	else
		return -1;
}

TStacks BattleProxy::getStacksIf(TStackFilter predicate) const
{
	return subject->battleGetStacksIf(predicate);
}

battle::Units BattleProxy::getUnitsIf(battle::UnitFilter predicate) const
{
	return subject->battleGetUnitsIf(predicate);
}

BattleField BattleProxy::getBattlefieldType() const
{
	return subject->battleGetBattlefieldType();
}

TerrainId BattleProxy::getTerrainType() const
{
	return subject->battleTerrainType();
}

IBattleInfo::ObstacleCList BattleProxy::getAllObstacles() const
{
	return subject->battleGetAllObstacles();
}

PlayerColor BattleProxy::getSidePlayer(ui8 side) const
{
	return subject->sideToPlayer(side);
}

const CArmedInstance * BattleProxy::getSideArmy(ui8 side) const
{
	return subject->battleGetArmyObject(side);
}

const CGHeroInstance * BattleProxy::getSideHero(ui8 side) const
{
	return subject->battleGetFightingHero(side);
}

ui8 BattleProxy::getTacticDist() const
{
	return subject->battleTacticDist();
}

ui8 BattleProxy::getTacticsSide() const
{
	return subject->battleGetTacticsSide();
}

const CGTownInstance * BattleProxy::getDefendedTown() const
{
	return subject->battleGetDefendedTown();
}

EWallState BattleProxy::getWallState(EWallPart partOfWall) const
{
	return subject->battleGetWallState(partOfWall);
}

EGateState BattleProxy::getGateState() const
{
	return subject->battleGetGateState();
}

uint32_t BattleProxy::getCastSpells(ui8 side) const
{
	return subject->battleCastSpells(side);
}

int32_t BattleProxy::getEnchanterCounter(ui8 side) const
{
	return subject->battleGetEnchanterCounter(side);
}

const IBonusBearer * BattleProxy::getBonusBearer() const
{
	return subject->getBonusBearer();
}


VCMI_LIB_NAMESPACE_END
