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

ETerrainType CBattleInfoEssentials::battleTerrainType() const
{
	RETURN_IF_NOT_BATTLE(ETerrainType::WRONG);
	return getBattle()->terrainType;
}

BFieldType CBattleInfoEssentials::battleGetBattlefieldType() const
{
	RETURN_IF_NOT_BATTLE(BFieldType::NONE);
	return getBattle()->battlefieldType;
}

std::vector<std::shared_ptr<const CObstacleInstance> > CBattleInfoEssentials::battleGetAllObstacles(boost::optional<BattlePerspective::BattlePerspective> perspective /*= boost::none*/) const
{
	std::vector<std::shared_ptr<const CObstacleInstance> > ret;
	RETURN_IF_NOT_BATTLE(ret);

	if(!perspective)
	{
		//if no particular perspective request, use default one
		perspective = battleGetMySide();
	}
	else
	{
		if(!!player && *perspective != battleGetMySide())
		{
			logGlobal->error("Unauthorized access attempt!");
			assert(0); //I want to notice if that happens
			//perspective = battleGetMySide();
		}
	}

	for(auto oi : getBattle()->obstacles)
	{
		if(getBattle()->battleIsObstacleVisibleForSide(*oi, *perspective))
			ret.push_back(oi);
	}

	return ret;
}

bool CBattleInfoEssentials::battleIsObstacleVisibleForSide(const CObstacleInstance & coi, BattlePerspective::BattlePerspective side) const
{
	RETURN_IF_NOT_BATTLE(false);
	return side == BattlePerspective::ALL_KNOWING || coi.visibleForSide(side, battleHasNativeStack(side));
}

bool CBattleInfoEssentials::battleHasNativeStack(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(false);

	for(const CStack * s : battleGetAllStacks())
	{
		if(s->attackerOwned == !side && s->getCreature()->isItNativeTerrain(getBattle()->terrainType))
			return true;
	}

	return false;
}

TStacks CBattleInfoEssentials::battleGetAllStacks(bool includeTurrets /*= false*/) const
{
	return battleGetStacksIf([=](const CStack * s)
	{
		return !s->isGhost() && (includeTurrets || !s->isTurret());
	});
}

TStacks CBattleInfoEssentials::battleGetStacksIf(TStackFilter predicate) const
{
	TStacks ret;
	RETURN_IF_NOT_BATTLE(ret);

	vstd::copy_if(getBattle()->stacks, std::back_inserter(ret), predicate);

	return ret;
}

TStacks CBattleInfoEssentials::battleAliveStacks() const
{
	return battleGetStacksIf([](const CStack * s){
		return s->isValidTarget(false);
	});
}

TStacks CBattleInfoEssentials::battleAliveStacks(ui8 side) const
{
	return battleGetStacksIf([=](const CStack * s){
		return s->isValidTarget(false) && s->attackerOwned == !side;
	});
}

int CBattleInfoEssentials::battleGetMoatDmg() const
{
	RETURN_IF_NOT_BATTLE(0);
	auto town = getBattle()->town;
	if(!town)
		return 0;
	return town->town->moatDamage;
}

const CGTownInstance * CBattleInfoEssentials::battleGetDefendedTown() const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	if(!getBattle() || getBattle()->town == nullptr)
		return nullptr;
	return getBattle()->town;
}

BattlePerspective::BattlePerspective CBattleInfoEssentials::battleGetMySide() const
{
	RETURN_IF_NOT_BATTLE(BattlePerspective::INVALID);
	if(!player || player.get().isSpectator())
		return BattlePerspective::ALL_KNOWING;
	if(*player == getBattle()->sides[0].color)
		return BattlePerspective::LEFT_SIDE;
	if(*player == getBattle()->sides[1].color)
		return BattlePerspective::RIGHT_SIDE;

	logGlobal->errorStream() << "Cannot find player " << *player << " in battle!";
	return BattlePerspective::INVALID;
}

const CStack * CBattleInfoEssentials::battleActiveStack() const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	return battleGetStackByID(getBattle()->activeStack);
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
	return getBattle()->tacticDistance;
}

si8 CBattleInfoEssentials::battleGetTacticsSide() const
{
	RETURN_IF_NOT_BATTLE(-1);
	return getBattle()->tacticsSide;
}

const CGHeroInstance * CBattleInfoEssentials::battleGetFightingHero(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	if(side > 1)
	{
		logGlobal->errorStream() << "FIXME: " <<  __FUNCTION__ << " wrong argument!";
		return nullptr;
	}

	if(!battleDoWeKnowAbout(side))
	{
		logGlobal->errorStream() << "FIXME: " <<  __FUNCTION__ << " access check ";
		return nullptr;
	}

	return getBattle()->sides[side].hero;
}

const CArmedInstance * CBattleInfoEssentials::battleGetArmyObject(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	if(side > 1)
	{
		logGlobal->errorStream() << "FIXME: " <<  __FUNCTION__ << " wrong argument!";
		return nullptr;
	}
	if(!battleDoWeKnowAbout(side))
	{
		logGlobal->errorStream() << "FIXME: " <<  __FUNCTION__ << " access check ";
		return nullptr;
	}
	return getBattle()->sides[side].armyObject;
}

InfoAboutHero CBattleInfoEssentials::battleGetHeroInfo(ui8 side) const
{
	auto hero = getBattle()->sides[side].hero;
	if(!hero)
	{
		logGlobal->warnStream() << __FUNCTION__ << ": side " << (int)side << " does not have hero!";
		return InfoAboutHero();
	}
	InfoAboutHero::EInfoLevel infoLevel = battleDoWeKnowAbout(side) ? InfoAboutHero::EInfoLevel::DETAILED : InfoAboutHero::EInfoLevel::BASIC;
	return InfoAboutHero(hero, infoLevel);
}

int CBattleInfoEssentials::battleCastSpells(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(-1);
	return getBattle()->sides[side].castSpellsCount;
}

const IBonusBearer * CBattleInfoEssentials::getBattleNode() const
{
	return getBattle();
}

bool CBattleInfoEssentials::battleCanFlee(PlayerColor player) const
{
	RETURN_IF_NOT_BATTLE(false);
	const si8 mySide = playerToSide(player);
	const CGHeroInstance *myHero = battleGetFightingHero(mySide);

	//current player have no hero
	if(!myHero)
		return false;

	//eg. one of heroes is wearing shakles of war
	if(myHero->hasBonusOfType(Bonus::BATTLE_NO_FLEEING))
		return false;

	//we are besieged defender
	if(mySide == BattleSide::DEFENDER && battleGetSiegeLevel())
	{
		auto town = battleGetDefendedTown();
		if(!town->hasBuilt(BuildingID::ESCAPE_TUNNEL, ETownType::STRONGHOLD))
			return false;
	}

	return true;
}

si8 CBattleInfoEssentials::playerToSide(PlayerColor player) const
{
	RETURN_IF_NOT_BATTLE(-1);
	int ret = vstd::find_pos_if(getBattle()->sides, [=](const SideInBattle &side){ return side.color == player; });
	if(ret < 0)
		logGlobal->warnStream() << "Cannot find side for player " << player;

	return ret;
}

bool CBattleInfoEssentials::playerHasAccessToHeroInfo(PlayerColor player, const CGHeroInstance * h) const
{
	RETURN_IF_NOT_BATTLE(false);
	const si8 playerSide = playerToSide(player);
	if (playerSide >= 0)
	{
		if (getBattle()->sides[!playerSide].hero == h)
			return true;
	}
	return false;
}

ui8 CBattleInfoEssentials::battleGetSiegeLevel() const
{
	RETURN_IF_NOT_BATTLE(0);
	return getBattle()->town ? getBattle()->town->fortLevel() : CGTownInstance::NONE;
}

bool CBattleInfoEssentials::battleCanSurrender(PlayerColor player) const
{
	RETURN_IF_NOT_BATTLE(false);
	ui8 mySide = playerToSide(player);
	bool iAmSiegeDefender = (mySide == BattleSide::DEFENDER && battleGetSiegeLevel());
	//conditions like for fleeing (except escape tunnel presence) + enemy must have a hero
	return battleCanFlee(player) && !iAmSiegeDefender && battleHasHero(!mySide);
}

bool CBattleInfoEssentials::battleHasHero(ui8 side) const
{
	RETURN_IF_NOT_BATTLE(false);
	assert(side < 2);
	return getBattle()->sides[side].hero;
}

si8 CBattleInfoEssentials::battleGetWallState(int partOfWall) const
{
	RETURN_IF_NOT_BATTLE(0);
	if(getBattle()->town == nullptr || getBattle()->town->fortLevel() == CGTownInstance::NONE)
		return EWallState::NONE;

	assert(partOfWall >= 0 && partOfWall < EWallPart::PARTS_COUNT);
	return getBattle()->si.wallState[partOfWall];
}

EGateState CBattleInfoEssentials::battleGetGateState() const
{
	RETURN_IF_NOT_BATTLE(EGateState::NONE);
	if(getBattle()->town == nullptr || getBattle()->town->fortLevel() == CGTownInstance::NONE)
		return EGateState::NONE;

	return getBattle()->si.gateState;
}

PlayerColor CBattleInfoEssentials::battleGetOwner(const CStack * stack) const
{
	RETURN_IF_NOT_BATTLE(PlayerColor::CANNOT_DETERMINE);
	if(stack->hasBonusOfType(Bonus::HYPNOTIZED))
		return getBattle()->theOtherPlayer(stack->owner);
	else
		return stack->owner;
}

const CGHeroInstance * CBattleInfoEssentials::battleGetOwnerHero(const CStack * stack) const
{
	RETURN_IF_NOT_BATTLE(nullptr);
	return getBattle()->sides.at(playerToSide(battleGetOwner(stack))).hero;
}

bool CBattleInfoEssentials::battleMatchOwner(const CStack * attacker, const CStack * defender, const boost::logic::tribool positivness /* = false*/) const
{
	RETURN_IF_NOT_BATTLE(false);
	if(boost::logic::indeterminate(positivness))
		return true;
	else if(defender->owner != battleGetOwner(defender))
		return true; //mind controlled unit is attackable for both sides
	else
		return (battleGetOwner(attacker) == battleGetOwner(defender)) == positivness;
}
