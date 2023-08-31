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

#include "../CGameHandler.h"
#include "../battles/BattleProcessor.h"

#include "../../lib/battle/IBattleState.h"

void CBattleQuery::notifyObjectAboutRemoval(const CObjectVisitQuery & objectVisit) const
{
	if(result)
		objectVisit.visitedObject->battleFinished(objectVisit.visitingHero, *result);
}

CBattleQuery::CBattleQuery(CGameHandler * owner, const IBattleInfo * bi):
	CGhQuery(owner),
	battleID(bi->getBattleID())
{
	belligerents[0] = bi->getSideArmy(0);
	belligerents[1] = bi->getSideArmy(1);

	addPlayer(bi->getSidePlayer(0));
	addPlayer(bi->getSidePlayer(1));
}

CBattleQuery::CBattleQuery(CGameHandler * owner):
	CGhQuery(owner)
{
	belligerents[0] = belligerents[1] = nullptr;
}

bool CBattleQuery::blocksPack(const CPack * pack) const
{
	const char * name = typeid(*pack).name();
	return strcmp(name, typeid(MakeAction).name()) != 0;
}

void CBattleQuery::onRemoval(PlayerColor color)
{
	if(result)
		gh->battles->battleAfterLevelUp(battleID, *result);
}

CBattleDialogQuery::CBattleDialogQuery(CGameHandler * owner, const IBattleInfo * bi):
	CDialogQuery(owner),
	bi(bi)
{
	addPlayer(bi->getSidePlayer(0));
	addPlayer(bi->getSidePlayer(1));
}

void CBattleDialogQuery::onRemoval(PlayerColor color)
{
	assert(answer);
	if(*answer == 1)
	{
		gh->startBattlePrimary(
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
		gh->battles->endBattleConfirm(bi->getBattleID());
	}
}
