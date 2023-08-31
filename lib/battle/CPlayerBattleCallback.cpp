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
#include "../CGameInfoCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

CPlayerBattleCallback::CPlayerBattleCallback(const IBattleInfo * battle, PlayerColor player):
	battle(battle),
	player(player)
{

}

#if SCRIPTING_ENABLED
scripting::Pool * CPlayerBattleCallback::getContextPool() const
{
	return nullptr; //TODO cl->getGlobalContextPool();
}
#endif

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
	RETURN_IF_NOT_BATTLE(false);
	ASSERT_IF_CALLED_WITH_PLAYER
			return CBattleInfoEssentials::battleCanFlee(*getPlayerID());
}

TStacks CPlayerBattleCallback::battleGetStacks(EStackOwnership whose, bool onlyAlive) const
{
	if(whose != MINE_AND_ENEMY)
	{
		ASSERT_IF_CALLED_WITH_PLAYER
	}

	return battleGetStacksIf([=](const CStack * s){
		const bool ownerMatches = (whose == MINE_AND_ENEMY)
								|| (whose == ONLY_MINE && s->unitOwner() == getPlayerID())
								|| (whose == ONLY_ENEMY && s->unitOwner() != getPlayerID());

		return ownerMatches && s->isValidTarget(!onlyAlive);
	});
}

int CPlayerBattleCallback::battleGetSurrenderCost() const
{
	RETURN_IF_NOT_BATTLE(-3)
			ASSERT_IF_CALLED_WITH_PLAYER
			return CBattleInfoCallback::battleGetSurrenderCost(*getPlayerID());
}

const CGHeroInstance * CPlayerBattleCallback::battleGetMyHero() const
{
	return CBattleInfoEssentials::battleGetFightingHero(battleGetMySide());
}

InfoAboutHero CPlayerBattleCallback::battleGetEnemyHero() const
{
	return battleGetHeroInfo(!battleGetMySide());
}


VCMI_LIB_NAMESPACE_END
