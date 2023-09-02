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

#include "../../lib/battle/BattleInfo.h"

void CBattleQuery::notifyObjectAboutRemoval(const CObjectVisitQuery & objectVisit) const
{
	if(result)
		objectVisit.visitedObject->battleFinished(objectVisit.visitingHero, *result);
}

CBattleQuery::CBattleQuery(CGameHandler * owner, const BattleInfo * Bi):
	CGhQuery(owner)
{
	belligerents[0] = Bi->sides[0].armyObject;
	belligerents[1] = Bi->sides[1].armyObject;

	bi = Bi;

	for(auto & side : bi->sides)
		addPlayer(side.color);
}

CBattleQuery::CBattleQuery(CGameHandler * owner):
	CGhQuery(owner), bi(nullptr)
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
		gh->battles->battleAfterLevelUp(*result);
}

CBattleDialogQuery::CBattleDialogQuery(CGameHandler * owner, const BattleInfo * Bi):
	CDialogQuery(owner)
{
	bi = Bi;

	for(auto & side : bi->sides)
		addPlayer(side.color);
}

CBattleDialogQuery::CBattleDialogQuery(CGameHandler * owner, const BattleInfo * Bi, const SideInBattle & sideToAdd):
		CDialogQuery(owner)
{
	bi = Bi;

	addPlayer(sideToAdd.color);
}

void CBattleDialogQuery::onRemoval(PlayerColor color)
{
	assert(answer);
	if(*answer == 1)
	{
		gh->startBattlePrimary(bi->sides[0].armyObject, bi->sides[1].armyObject, bi->tile, bi->sides[0].hero, bi->sides[1].hero, bi->creatureBank, bi->town);
	}
	else
	{
		gh->battles->endBattleConfirm(bi);
	}
}
