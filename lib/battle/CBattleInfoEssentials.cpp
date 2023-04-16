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
#include "../NetPacks.h"
#include "../mapObjects/CGTownInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

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

int32_t CBattleInfoEssentials::battleGetEnchanterCounter(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(0);
	return getBattle()->getEnchanterCounter(side);
}

std::vector<std::shared_ptr<const CObstacleInstance>> CBattleInfoEssentials::battleGetAllObstacles(boost::optional<BattlePerspective::BattlePerspective> perspective) const
{
	std::vector<std::shared_ptr<const CObstacleInstance> > ret;
	RETURN_IF_NOT_BATTLE(ret);

	if(!perspective)
	{
		//if no particular perspective request, use default one
		perspective = boost::make_optional(battleGetMySide());
	}
	else
	{
		if(!!player && *perspective != battleGetMySide())
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

bool CBattleInfoEssentials::battleIsObstacleVisibleForSide(const CObstacleInstance & coi, BattlePerspective::BattlePerspective side) const
{
	RETURN_IF_NOT_BATTLE(false);
	return side == BattlePerspective::ALL_KNOWING || coi.visibleForSide(side, battleHasNativeStack(side));
}

bool CBattleInfoEssentials::battleHasNativeStack(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(false);

	for(const auto * s : battleGetAllStacks())
	{
		if(s->side == side && s->isNativeTerrain(getBattle()->getTerrainType()))
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

TStacks CBattleInfoEssentials::battleGetStacksIf(TStackFilter predicate) const
{
	RETURN_IF_NOT_BATTLE(TStacks());
	return getBattle()->getStacksIf(std::move(predicate));
}

battle::Units CBattleInfoEssentials::battleGetUnitsIf(battle::UnitFilter predicate)  const
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

BattlePerspective::BattlePerspective CBattleInfoEssentials::battleGetMySide() const
{
	RETURN_IF_NOT_BATTLE(BattlePerspective::INVALID);
	if(!player || player.get().isSpectator())
		return BattlePerspective::ALL_KNOWING;
	if(*player == getBattle()->getSidePlayer(BattleSide::ATTACKER))
		return BattlePerspective::LEFT_SIDE;
	if(*player == getBattle()->getSidePlayer(BattleSide::DEFENDER))
		return BattlePerspective::RIGHT_SIDE;

	logGlobal->error("Cannot find player %s in battle!", player->getStr());
	return BattlePerspective::INVALID;
}

const CStack* CBattleInfoEssentials::battleGetStackByID(int ID, bool onlyAlive) const
{
	RETURN_IF_NOT_BATTLE(nullptr);

	auto stacks = battleGetStacksIf([=](const CStack * s)
	{
		return s->ID == ID && (!onlyAlive || s->alive());
	});

	if(stacks.empty())
		return nullptr;
	else
		return stacks[0];
}

bool CBattleInfoEssentials::battleDoWeKnowAbout(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(false);
	auto p = battleGetMySide();
	return p == BattlePerspective::ALL_KNOWING || p == side;
}

si8 CBattleInfoEssentials::battleTacticDist() const
{
	RETURN_IF_NOT_BATTLE(0);
	return getBattle()->getTacticDist();
}

si8 CBattleInfoEssentials::battleGetTacticsSide() const
{
	RETURN_IF_NOT_BATTLE(-1);
	return getBattle()->getTacticsSide();
}

const CGHeroInstance * CBattleInfoEssentials::battleGetFightingHero(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	if(side > 1)
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

const CArmedInstance * CBattleInfoEssentials::battleGetArmyObject(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	if(side > 1)
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

InfoAboutHero CBattleInfoEssentials::battleGetHeroInfo(ui8 side) const
{
	const auto * hero = getBattle()->getSideHero(side);
	if(!hero)
	{
		return InfoAboutHero();
	}
	InfoAboutHero::EInfoLevel infoLevel = battleDoWeKnowAbout(side) ? InfoAboutHero::EInfoLevel::DETAILED : InfoAboutHero::EInfoLevel::BASIC;
	return InfoAboutHero(hero, infoLevel);
}

uint32_t CBattleInfoEssentials::battleCastSpells(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(-1);
	return getBattle()->getCastSpells(side);
}

const IBonusBearer * CBattleInfoEssentials::getBonusBearer() const
{
	return getBattle()->getBonusBearer();
}

bool CBattleInfoEssentials::battleCanFlee(const PlayerColor & player) const
{
	RETURN_IF_NOT_BATTLE(false);
	const auto side = playerToSide(player);
	if(!side)
		return false;

	const CGHeroInstance *myHero = battleGetFightingHero(side.get());

	//current player have no hero
	if(!myHero)
		return false;

	//eg. one of heroes is wearing shakles of war
	if(myHero->hasBonusOfType(Bonus::BATTLE_NO_FLEEING))
		return false;

	//we are besieged defender
	if(side.get() == BattleSide::DEFENDER && battleGetSiegeLevel())
	{
		const auto * town = battleGetDefendedTown();
		if(!town->hasBuilt(BuildingSubID::ESCAPE_TUNNEL))
			return false;
	}

	return true;
}

BattleSideOpt CBattleInfoEssentials::playerToSide(const PlayerColor & player) const
{
	RETURN_IF_NOT_BATTLE(boost::none);

	if(getBattle()->getSidePlayer(BattleSide::ATTACKER) == player)
		return BattleSideOpt(BattleSide::ATTACKER);

	if(getBattle()->getSidePlayer(BattleSide::DEFENDER) == player)
		return BattleSideOpt(BattleSide::DEFENDER);

	logGlobal->warn("Cannot find side for player %s", player.getStr());

	return boost::none;
}

PlayerColor CBattleInfoEssentials::sideToPlayer(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(PlayerColor::CANNOT_DETERMINE);
    return getBattle()->getSidePlayer(side);
}

ui8 CBattleInfoEssentials::otherSide(ui8 side) const
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
    if(!side)
		return PlayerColor::CANNOT_DETERMINE;

	return getBattle()->getSidePlayer(otherSide(side.get()));
}

bool CBattleInfoEssentials::playerHasAccessToHeroInfo(const PlayerColor & player, const CGHeroInstance * h) const
{
	RETURN_IF_NOT_BATTLE(false);
	const auto side = playerToSide(player);
	if(side)
	{
		auto opponentSide = otherSide(side.get());
		if(getBattle()->getSideHero(opponentSide) == h)
			return true;
	}
	return false;
}

ui8 CBattleInfoEssentials::battleGetSiegeLevel() const
{
	RETURN_IF_NOT_BATTLE(CGTownInstance::NONE);
	return getBattle()->getDefendedTown() ? getBattle()->getDefendedTown()->fortLevel() : CGTownInstance::NONE;
}

bool CBattleInfoEssentials::battleCanSurrender(const PlayerColor & player) const
{
	RETURN_IF_NOT_BATTLE(false);
	const auto side = playerToSide(player);
	if(!side)
		return false;
	bool iAmSiegeDefender = (side.get() == BattleSide::DEFENDER && battleGetSiegeLevel());
	//conditions like for fleeing (except escape tunnel presence) + enemy must have a hero
	return battleCanFlee(player) && !iAmSiegeDefender && battleHasHero(otherSide(side.get()));
}

bool CBattleInfoEssentials::battleHasHero(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(false);
	return getBattle()->getSideHero(side) != nullptr;
}

EWallState CBattleInfoEssentials::battleGetWallState(EWallPart partOfWall) const
{
	RETURN_IF_NOT_BATTLE(EWallState::NONE);
	if(battleGetSiegeLevel() == CGTownInstance::NONE)
		return EWallState::NONE;

	return getBattle()->getWallState(partOfWall);
}

EGateState CBattleInfoEssentials::battleGetGateState() const
{
	RETURN_IF_NOT_BATTLE(EGateState::NONE);
	if(battleGetSiegeLevel() == CGTownInstance::NONE)
		return EGateState::NONE;

	return getBattle()->getGateState();
}

PlayerColor CBattleInfoEssentials::battleGetOwner(const battle::Unit * unit) const
{
	RETURN_IF_NOT_BATTLE(PlayerColor::CANNOT_DETERMINE);

	PlayerColor initialOwner = getBattle()->getSidePlayer(unit->unitSide());

	static CSelector selector = Selector::type()(Bonus::HYPNOTIZED);
	static std::string cachingString = "type_103s-1";

	if(unit->hasBonus(selector, cachingString))
		return otherPlayer(initialOwner);
	else
		return initialOwner;
}

const CGHeroInstance * CBattleInfoEssentials::battleGetOwnerHero(const battle::Unit * unit) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	const auto side = playerToSide(battleGetOwner(unit));
	if(!side)
		return nullptr;
	return getBattle()->getSideHero(side.get());
}

bool CBattleInfoEssentials::battleMatchOwner(const battle::Unit * attacker, const battle::Unit * defender, const boost::logic::tribool positivness) const
{
	RETURN_IF_NOT_BATTLE(false);
	if(boost::logic::indeterminate(positivness))
		return true;
	else if(attacker->unitId() == defender->unitId())
		return (bool)positivness;
	else
		return battleMatchOwner(battleGetOwner(attacker), defender, positivness);
}

bool CBattleInfoEssentials::battleMatchOwner(const PlayerColor & attacker, const battle::Unit * defender, const boost::logic::tribool positivness) const
{
	RETURN_IF_NOT_BATTLE(false);

	PlayerColor initialOwner = getBattle()->getSidePlayer(defender->unitSide());

	return boost::logic::indeterminate(positivness) || (attacker == initialOwner) == (bool)positivness;
}

VCMI_LIB_NAMESPACE_END
