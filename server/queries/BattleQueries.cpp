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
#include "../../lib/battle/SideInBattle.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/networkPacks/PacksForServer.h"
#include "../../lib/serializer/Cast.h"

void CBattleQuery::notifyObjectAboutRemoval(const CObjectVisitQuery & objectVisit) const
{
	assert(result);

	if(result && !isAiVsHuman)
		objectVisit.visitedObject->battleFinished(objectVisit.visitingHero, *result);
}

CBattleQuery::CBattleQuery(CGameHandler * owner, const IBattleInfo * bi):
	CQuery(owner),
	battleID(bi->getBattleID())
{
	belligerents[0] = bi->getSideArmy(0);
	belligerents[1] = bi->getSideArmy(1);

	isAiVsHuman = bi->getSidePlayer(1).isValidPlayer() && gh->getPlayerState(bi->getSidePlayer(0))->isHuman() != gh->getPlayerState(bi->getSidePlayer(1))->isHuman();

	addPlayer(bi->getSidePlayer(0));
	addPlayer(bi->getSidePlayer(1));
}

CBattleQuery::CBattleQuery(CGameHandler * owner):
	CQuery(owner)
{
	belligerents[0] = belligerents[1] = nullptr;
}

bool CBattleQuery::blocksPack(const CPack * pack) const
{
	if(dynamic_ptr_cast<MakeAction>(pack) != nullptr)
		return false;

	if(dynamic_ptr_cast<GamePause>(pack) != nullptr)
		return false;

	return true;
}

void CBattleQuery::onRemoval(PlayerColor color)
{
	assert(result);

	if(result)
		gh->battles->battleAfterLevelUp(battleID, *result);
}

void CBattleQuery::onExposure(QueryPtr topQuery)
{
	// this method may be called in two cases:
	// 1) when requesting battle replay (but before replay starts -> no valid result)
	// 2) when aswering on levelup queries after accepting battle result -> valid result
	if(result)
		owner->popQuery(*this);
}

CBattleDialogQuery::CBattleDialogQuery(CGameHandler * owner, const IBattleInfo * bi, std::optional<BattleResult> Br):
	CDialogQuery(owner),
	bi(bi),
	result(Br)
{
	addPlayer(bi->getSidePlayer(0));
	addPlayer(bi->getSidePlayer(1));
}

void CBattleDialogQuery::onRemoval(PlayerColor color)
{
	if (!gh->getPlayerState(color)->isHuman())
		return;

	assert(answer);
	if(*answer == 1)
	{
		gh->battles->restartBattlePrimary(
			bi->getBattleID(),
			bi->getSideArmy(0),
			bi->getSideArmy(1),
			bi->getLocation(),
			bi->getSideHero(0),
			bi->getSideHero(1),
			bi->isCreatureBank(),
			bi->getDefendedTown()
		);
	}
	else
	{
		auto hero = bi->getSideHero(BattleSide::ATTACKER);
		auto visitingObj = bi->getDefendedTown() ? bi->getDefendedTown() : gh->getVisitingObject(hero);
		bool isAiVsHuman = bi->getSidePlayer(1).isValidPlayer() && gh->getPlayerState(bi->getSidePlayer(0))->isHuman() != gh->getPlayerState(bi->getSidePlayer(1))->isHuman();
		
		gh->battles->endBattleConfirm(bi->getBattleID());

		if(visitingObj && result && isAiVsHuman)
			visitingObj->battleFinished(hero, *result);
	}
}
