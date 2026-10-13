/*
 * CPlayerBattleCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CPlayerBattleCallback.h"
#include "../CStack.h"
#include "../gameState/InfoAboutArmy.h"
#include "../callback/CallbackDefines.h"


CPlayerBattleCallback::CPlayerBattleCallback(const IBattleInfo * battle, PlayerColor player):
	battle(battle),
	player(player)
{

}

const IBattleInfo * CPlayerBattleCallback::getBattle() const
{
	return battle;
}

std::optional<PlayerColor> CPlayerBattleCallback::getPlayerID() const
{
	return player;
}

bool CPlayerBattleCallback::battleCanFlee() const
{
	THROW_IF_NOT_BATTLE();
	THROW_IF_CALLED_WITH_PLAYER();
	return CBattleInfoEssentials::battleCanFlee(getPlayerID().value());
}

TStacks CPlayerBattleCallback::battleGetStacks(EStackOwnership whose, bool onlyAlive) const
{
	if(whose != MINE_AND_ENEMY)
		THROW_IF_CALLED_WITH_PLAYER();

	return battleGetStacksIf([&](const CStack * s){
		const bool ownerMatches = (whose == MINE_AND_ENEMY)
								|| (whose == ONLY_MINE && s->unitOwner() == getPlayerID())
								|| (whose == ONLY_ENEMY && s->unitOwner() != getPlayerID());

		return ownerMatches && s->isValidTarget(!onlyAlive);
	});
}

int CPlayerBattleCallback::battleGetSurrenderCost() const
{
	THROW_IF_NOT_BATTLE();
	THROW_IF_CALLED_WITH_PLAYER();
	return CBattleInfoCallback::battleGetSurrenderCost(getPlayerID().value());
}

const CGHeroInstance * CPlayerBattleCallback::battleGetMyHero() const
{
	return CBattleInfoEssentials::battleGetFightingHero(battleGetMySide());
}

InfoAboutHero CPlayerBattleCallback::battleGetEnemyHero() const
{
	return battleGetHeroInfo(otherSide(battleGetMySide()));
}

