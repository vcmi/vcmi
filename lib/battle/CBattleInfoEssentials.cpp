/*
 * CBattleInfoEssentials.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleInfoEssentials.h"

#include "../CStack.h"
#include "BattleInfo.h"
#include "CObstacleInstance.h"
#include "GameLibrary.h"
#include "IGameSettings.h"

#include "../constants/EntityIdentifiers.h"
#include "../entities/building/TownFortifications.h"
#include "../gameState/InfoAboutArmy.h"
#include "../mapObjects/CGTownInstance.h"

bool CBattleInfoEssentials::duringBattle() const
{
	return getBattle() != nullptr;
}

TerrainId CBattleInfoEssentials::battleTerrainType() const
{
	RETURN_IF_NOT_BATTLE(TerrainId());
	return getBattle()->getTerrainType();
}

BattleField CBattleInfoEssentials::battleGetBattlefieldType() const
{
	RETURN_IF_NOT_BATTLE(BattleField::NONE);
	return getBattle()->getBattlefieldType();
}

int32_t CBattleInfoEssentials::battleGetEnchanterCounter(BattleSide side) const
{
	RETURN_IF_NOT_BATTLE(0);
	return getBattle()->getEnchanterCounter(side);
}

int32_t CBattleInfoEssentials::nextObstacleId() const
{
	int32_t maxId = -1;
	for (const auto & obstacle : getBattle()->getAllObstacles())
	{
		if (obstacle->uniqueID > maxId)
			maxId = obstacle->uniqueID;
	}
	return maxId + 1;
}

std::vector<std::shared_ptr<const CObstacleInstance>> CBattleInfoEssentials::battleGetAllObstacles(std::optional<BattleSide> perspective) const
{
	std::vector<std::shared_ptr<const CObstacleInstance> > ret;
	RETURN_IF_NOT_BATTLE(ret);

	if(!perspective)
	{
		//if no particular perspective request, use default one
		perspective = std::make_optional(battleGetMySide());
	}
	else
	{
		if(!!getPlayerID() && *perspective != battleGetMySide())
			logGlobal->warn("Unauthorized obstacles access attempt, assuming massive spell");
	}

	for(const auto & obstacle : getBattle()->getAllObstacles())
	{
		if(battleIsObstacleVisibleForSide(*(obstacle), *perspective))
			ret.push_back(obstacle);
	}

	return ret;
}

std::shared_ptr<const CObstacleInstance> CBattleInfoEssentials::battleGetObstacleByID(uint32_t ID) const
{
	std::shared_ptr<const CObstacleInstance> ret;

	RETURN_IF_NOT_BATTLE(std::shared_ptr<const CObstacleInstance>());

	for(auto obstacle : getBattle()->getAllObstacles())
	{
		if(obstacle->uniqueID == ID)
			return obstacle;
	}

	logGlobal->error("Invalid obstacle ID %d", ID);
	return std::shared_ptr<const CObstacleInstance>();
}

bool CBattleInfoEssentials::battleIsObstacleVisibleForSide(const CObstacleInstance & coi, BattleSide side) const
{
	RETURN_IF_NOT_BATTLE(false);
	return side == BattleSide::ALL_KNOWING || coi.visibleForSide(side, battleHasNativeStack(side));
}

bool CBattleInfoEssentials::battleHasNativeStack(BattleSide side) const
{
	RETURN_IF_NOT_BATTLE(false);

	for(const auto * s : battleGetAllStacks())
	{
		if(s->unitSide() == side && s->isNativeTerrain(getBattle()->getTerrainType()))
			return true;
	}

	return false;
}

TStacks CBattleInfoEssentials::battleGetAllStacks(bool includeTurrets) const
{
	return battleGetStacksIf([=](const CStack * s)
	{
		return !s->isGhost() && (includeTurrets || !s->isTurret());
	});
}

battle::Units CBattleInfoEssentials::battleGetAllUnits(bool includeTurrets) const
{
	return battleGetUnitsIf([includeTurrets](const battle::Unit * unit)
	{
		return !unit->isGhost() && (includeTurrets || !unit->isTurret());
	});
}

TStacks CBattleInfoEssentials::battleGetStacksIf(const TStackFilter & predicate) const
{
	RETURN_IF_NOT_BATTLE(TStacks());
	return getBattle()->getStacksIf(std::move(predicate));
}

battle::Units CBattleInfoEssentials::battleGetUnitsIf(const battle::UnitFilter & predicate)  const
{
	RETURN_IF_NOT_BATTLE(battle::Units());
	return getBattle()->getUnitsIf(predicate);
}

const battle::Unit * CBattleInfoEssentials::battleGetUnitByID(uint32_t ID) const
{
	RETURN_IF_NOT_BATTLE(nullptr);

	//TODO: consider using map ID -> Unit

	auto ret = battleGetUnitsIf([=](const battle::Unit * unit)
	{
		return unit->unitId() == ID;
	});

	if(ret.empty())
		return nullptr;
	else
		return ret[0];
}

const battle::Unit * CBattleInfoEssentials::battleActiveUnit() const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	auto id = getBattle()->getActiveStackID();
	if(id >= 0)
		return battleGetUnitByID(static_cast<uint32_t>(id));
	else
		return nullptr;
}

uint32_t CBattleInfoEssentials::battleNextUnitId() const
{
	return getBattle()->nextUnitId();
}

const CGTownInstance * CBattleInfoEssentials::battleGetDefendedTown() const
{
	RETURN_IF_NOT_BATTLE(nullptr);

	return getBattle()->getDefendedTown();
}

bool CBattleInfoEssentials::hasFortifications() const
{
	const auto * town = battleGetDefendedTown();
	return town != nullptr && town->fortificationsLevel().wallsHealth > 0;
}

bool CBattleInfoEssentials::hasMoat() const
{
	return battleGetFortifications().hasMoat;
}

BattleHex CBattleInfoEssentials::getTowerShooterHex(EWallPart part) const
{
	switch(part)
	{
		case EWallPart::KEEP:         return BattleHex(BattleHex::CASTLE_CENTRAL_TOWER);
		case EWallPart::BOTTOM_TOWER: return BattleHex(BattleHex::CASTLE_BOTTOM_TOWER);
		case EWallPart::UPPER_TOWER:  return BattleHex(BattleHex::CASTLE_UPPER_TOWER);
		default:                      return BattleHex(BattleHex::INVALID);
	}
}

BattleSide CBattleInfoEssentials::battleGetMySide() const
{
	RETURN_IF_NOT_BATTLE(BattleSide::INVALID);
	if(!getPlayerID() || getPlayerID()->isSpectator())
		return BattleSide::ALL_KNOWING;
	if(*getPlayerID() == getBattle()->getSidePlayer(BattleSide::ATTACKER))
		return BattleSide::LEFT_SIDE;
	if(*getPlayerID() == getBattle()->getSidePlayer(BattleSide::DEFENDER))
		return BattleSide::RIGHT_SIDE;

	logGlobal->error("Cannot find player %s in battle!", getPlayerID()->toString());
	return BattleSide::INVALID;
}

const CStack* CBattleInfoEssentials::battleGetStackByID(int ID, bool onlyAlive) const
{
	RETURN_IF_NOT_BATTLE(nullptr);

	auto stacks = battleGetStacksIf([=](const CStack * s)
	{
		return s->unitId() == ID && (!onlyAlive || s->alive());
	});

	if(stacks.empty())
		return nullptr;
	else
		return stacks[0];
}

bool CBattleInfoEssentials::battleDoWeKnowAbout(BattleSide side) const
{
	RETURN_IF_NOT_BATTLE(false);
	auto p = battleGetMySide();
	return p == BattleSide::ALL_KNOWING || p == side;
}

si8 CBattleInfoEssentials::battleTacticDist() const
{
	RETURN_IF_NOT_BATTLE(0);
	return getBattle()->getTacticDist();
}

BattleSide CBattleInfoEssentials::battleGetTacticsSide() const
{
	RETURN_IF_NOT_BATTLE(BattleSide::NONE);
	return getBattle()->getTacticsSide();
}

int32_t CBattleInfoEssentials::battleGetRound() const
{
	RETURN_IF_NOT_BATTLE(-1);
	return getBattle()->getRound();
}

const CGHeroInstance * CBattleInfoEssentials::battleGetFightingHero(BattleSide side) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	if(side != BattleSide::DEFENDER && side != BattleSide::ATTACKER)
	{
		logGlobal->error("FIXME: %s wrong argument!", __FUNCTION__);
		return nullptr;
	}

	if(!battleDoWeKnowAbout(side))
	{
		logGlobal->error("FIXME: %s access check ", __FUNCTION__);
		return nullptr;
	}

	return getBattle()->getSideHero(side);
}

const CArmedInstance * CBattleInfoEssentials::battleGetArmyObject(BattleSide side) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	if(side != BattleSide::DEFENDER && side != BattleSide::ATTACKER)
	{
		logGlobal->error("FIXME: %s wrong argument!", __FUNCTION__);
		return nullptr;
	}
	if(!battleDoWeKnowAbout(side))
	{
		logGlobal->error("FIXME: %s access check!", __FUNCTION__);
		return nullptr;
	}
	return getBattle()->getSideArmy(side);
}

InfoAboutHero CBattleInfoEssentials::battleGetHeroInfo(BattleSide side) const
{
	const auto * hero = getBattle()->getSideHero(side);
	if(!hero)
	{
		return InfoAboutHero();
	}
	InfoAboutHero::EInfoLevel infoLevel = battleDoWeKnowAbout(side) ? InfoAboutHero::EInfoLevel::DETAILED : InfoAboutHero::EInfoLevel::BASIC;
	return InfoAboutHero(hero, infoLevel);
}

int32_t CBattleInfoEssentials::battleCastSpells(BattleSide side) const
{
	RETURN_IF_NOT_BATTLE(-1);
	return getBattle()->getCastSpells(side);
}

const IBonusBearer * CBattleInfoEssentials::getBonusBearer() const
{
	return getBattle()->getBonusBearer();
}

int CBattleInfoEssentials::battleGetRetreatPermission(BattleSide side, BonusType permission) const
{
	const CGHeroInstance * myHero = battleGetFightingHero(side);
	const bool enemyHasHero = battleHasHero(otherSide(side));
	int result = 0;

	//blockers such as shackles of war have nobody to enforce them if enemy army has no hero
	for(const auto & bonus : *myHero->getBonusesOfType(permission))
		if(enemyHasHero || bonus->val > 0)
			result += bonus->val;

	//we are besieged defender, town buildings decide - village hall locks us in, escape tunnel provides a way out
	if(side == BattleSide::DEFENDER && battleGetDefendedTown() != nullptr)
		result += battleGetDefendedTown()->valOfBonuses(permission);

	//cannot leave after casting spell in X first turns as attacker
	if(getBattle()->getRound() <= LIBRARY->engineSettings()->getInteger(EGameSettings::COMBAT_NO_SPELL_HIT_AND_RUN_ROUNDS)
		&& side == BattleSide::ATTACKER && enemyHasHero && getBattle()->getCastSpells(side) >= 1)
		result -= GameConstants::BATTLE_RETREAT_RESTRICTION;

	return result;
}

bool CBattleInfoEssentials::battleCanFlee(const PlayerColor & player) const
{
	RETURN_IF_NOT_BATTLE(false);
	const BattleSide side = playerToSide(player);
	if(side == BattleSide::NONE)
		return false;

	//current player has no hero
	if(!battleGetFightingHero(side))
		return false;

	return battleGetRetreatPermission(side, BonusType::BATTLE_CAN_FLEE) >= 0;
}

BattleSide CBattleInfoEssentials::playerToSide(const PlayerColor & player) const
{
	RETURN_IF_NOT_BATTLE(BattleSide::NONE);

	if(getBattle()->getSidePlayer(BattleSide::ATTACKER) == player)
		return BattleSide::ATTACKER;

	if(getBattle()->getSidePlayer(BattleSide::DEFENDER) == player)
		return BattleSide::DEFENDER;

	logGlobal->warn("Cannot find side for player %s", player.toString());

	return BattleSide::INVALID;
}

PlayerColor CBattleInfoEssentials::sideToPlayer(BattleSide side) const
{
	RETURN_IF_NOT_BATTLE(PlayerColor::CANNOT_DETERMINE);
	return getBattle()->getSidePlayer(side);
}

BattleSide CBattleInfoEssentials::otherSide(BattleSide side)
{
	if(side == BattleSide::ATTACKER)
		return BattleSide::DEFENDER;
	else
		return BattleSide::ATTACKER;
}

PlayerColor CBattleInfoEssentials::otherPlayer(const PlayerColor & player) const
{
	RETURN_IF_NOT_BATTLE(PlayerColor::CANNOT_DETERMINE);

	auto side = playerToSide(player);
	if(side == BattleSide::NONE)
		return PlayerColor::CANNOT_DETERMINE;

	return getBattle()->getSidePlayer(otherSide(side));
}

bool CBattleInfoEssentials::playerHasAccessToHeroInfo(const PlayerColor & player, const CGHeroInstance * h) const
{
	RETURN_IF_NOT_BATTLE(false);
	const auto side = playerToSide(player);
	if(side != BattleSide::NONE)
	{
		auto opponentSide = otherSide(side);
		if(getBattle()->getSideHero(opponentSide) == h)
			return true;
	}
	return false;
}

TownFortifications CBattleInfoEssentials::battleGetFortifications() const
{
	RETURN_IF_NOT_BATTLE(TownFortifications());
	return getBattle()->getDefendedTown() ? getBattle()->getDefendedTown()->fortificationsLevel() : TownFortifications();
}

bool CBattleInfoEssentials::battleCanSurrender(const PlayerColor & player) const
{
	RETURN_IF_NOT_BATTLE(false);
	const auto side = playerToSide(player);
	if(side == BattleSide::NONE)
		return false;

	//current player has no hero
	if(!battleGetFightingHero(side))
		return false;

	int permission = battleGetRetreatPermission(side, BonusType::BATTLE_CAN_SURRENDER);

	//enemy must have a hero to negotiate the surrender with
	if(!battleHasHero(otherSide(side)))
		permission -= GameConstants::BATTLE_RETREAT_RESTRICTION;

	//hero that is not allowed to retreat can not surrender either
	return permission >= 0 && battleCanFlee(player);
}

bool CBattleInfoEssentials::battleHasHero(BattleSide side) const
{
	RETURN_IF_NOT_BATTLE(false);
	return getBattle()->getSideHero(side) != nullptr;
}

EWallState CBattleInfoEssentials::battleGetWallState(EWallPart partOfWall) const
{
	RETURN_IF_NOT_BATTLE(EWallState::NONE);
	if(battleGetFortifications().wallsHealth == 0)
		return EWallState::NONE;

	return getBattle()->getWallState(partOfWall);
}

EGateState CBattleInfoEssentials::battleGetGateState() const
{
	RETURN_IF_NOT_BATTLE(EGateState::NONE);
	if(battleGetFortifications().wallsHealth == 0)
		return EGateState::NONE;

	return getBattle()->getGateState();
}

bool CBattleInfoEssentials::battleIsGatePassable() const
{
	RETURN_IF_NOT_BATTLE(true);
	if(battleGetFortifications().wallsHealth == 0)
		return true;

	return battleGetGateState() == EGateState::OPENED || battleGetGateState() == EGateState::DESTROYED; 
}

PlayerColor CBattleInfoEssentials::battleGetOwner(const battle::Unit * unit) const
{
	RETURN_IF_NOT_BATTLE(PlayerColor::CANNOT_DETERMINE);

	PlayerColor initialOwner = getBattle()->getSidePlayer(unit->unitSide());

	if(unit->isHypnotized())
		return otherPlayer(initialOwner);
	else
		return initialOwner;
}

const CGHeroInstance * CBattleInfoEssentials::battleGetOwnerHero(const battle::Unit * unit) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	const auto side = playerToSide(battleGetOwner(unit));
	if(side == BattleSide::NONE)
		return nullptr;
	return getBattle()->getSideHero(side);
}

bool CBattleInfoEssentials::battleMatchOwner(const battle::Unit * attacker, const battle::Unit * defender, const bool sameOwner) const
{
	RETURN_IF_NOT_BATTLE(false);
	if(attacker->unitId() == defender->unitId())
		return sameOwner;
	else
		return battleMatchOwner(battleGetOwner(attacker), defender, sameOwner);
}

bool CBattleInfoEssentials::battleMatchOwner(const PlayerColor & attacker, const battle::Unit * defender, const bool sameOwner) const
{
	RETURN_IF_NOT_BATTLE(false);

	PlayerColor initialOwner = getBattle()->getSidePlayer(defender->unitSide());

	return (attacker == initialOwner) == sameOwner;
}
