/*
 * BattleQueries.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleQueries.h"
#include "MapQueries.h"
#include "QueriesProcessor.h"

#include "../CGameHandler.h"
#include "../battles/BattleProcessor.h"

#include "../../lib/battle/IBattleState.h"
#include "../../lib/battle/BattleLayout.h"
#include "../../lib/battle/SideInBattle.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/networkPacks/PacksForServer.h"

void CBattleQuery::notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const
{
	assert(result);

	if(result)
		visitedObject->battleFinished(*gh, visitingHero, *result);
}

CBattleQuery::CBattleQuery(CGameHandler * owner, const IBattleInfo * bi):
	CQuery(owner),
	battleID(bi->getBattleID())
{
	belligerents[BattleSide::ATTACKER] = bi->getSideArmy(BattleSide::ATTACKER);
	belligerents[BattleSide::DEFENDER] = bi->getSideArmy(BattleSide::DEFENDER);

	addPlayer(bi->getSidePlayer(BattleSide::ATTACKER));
	addPlayer(bi->getSidePlayer(BattleSide::DEFENDER));
}

CBattleQuery::CBattleQuery(CGameHandler * owner):
	CQuery(owner)
{
	belligerents[BattleSide::ATTACKER] = nullptr;
	belligerents[BattleSide::DEFENDER] = nullptr;
}

bool CBattleQuery::blocksPack(const CPackForServer * pack) const
{
	if(dynamic_cast<const MakeAction*>(pack) != nullptr)
		return false;

	if(dynamic_cast<const GamePause*>(pack) != nullptr)
		return false;

	return true;
}

void CBattleQuery::onRemoval(PlayerColor color)
{
	assert(result);

	if(result)
		gh->battles->battleFinalize(battleID, *result);
}

void CBattleQuery::onExposure(QueryPtr topQuery)
{
	// this method may be called in two cases:
	// 1) when requesting battle replay (but before replay starts -> no valid result)
	// 2) when aswering on levelup queries after accepting battle result -> valid result
	if(result)
		owner->popQuery(*this);
}

CBattleDialogQuery::CBattleDialogQuery(CGameHandler * owner, const IBattleInfo * bi, const std::optional<BattleResult> & Br):
	CDialogQuery(owner),
	bi(bi),
	result(Br)
{
	addPlayer(bi->getSidePlayer(BattleSide::ATTACKER));
	addPlayer(bi->getSidePlayer(BattleSide::DEFENDER));
}

void CBattleDialogQuery::onRemoval(PlayerColor color)
{
	// answer to this query was already processed when handling 1st player
	// this removal call for 2nd player which can be safely ignored
	if (resultProcessed)
		return;

	assert(answer);
	if(*answer == 1)
	{
		gh->battles->restartBattle(
			bi->getBattleID(),
			bi->getSideArmy(BattleSide::ATTACKER),
			bi->getSideArmy(BattleSide::DEFENDER),
			bi->getLocation(),
			bi->getSideHero(BattleSide::ATTACKER),
			bi->getSideHero(BattleSide::DEFENDER),
			bi->getLayout(),
			bi->getDefendedTown()
		);
	}
	else
	{
		gh->battles->endBattleConfirm(bi->getBattleID());
	}
	resultProcessed = true;
}
