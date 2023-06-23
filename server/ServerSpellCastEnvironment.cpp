/*
 * ServerSpellCastEnvironment.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "../lib/gameState/CGameState.h"
#include "CGameHandler.h"
#include "ServerSpellCastEnvironment.h"

///ServerSpellCastEnvironment
ServerSpellCastEnvironment::ServerSpellCastEnvironment(CGameHandler * gh)
	: gh(gh)
{
}

bool ServerSpellCastEnvironment::describeChanges() const
{
	return true;
}

void ServerSpellCastEnvironment::complain(const std::string & problem)
{
	gh->complain(problem);
}

vstd::RNG * ServerSpellCastEnvironment::getRNG()
{
	return &gh->getRandomGenerator();
}

void ServerSpellCastEnvironment::apply(CPackForClient * pack)
{
	gh->sendAndApply(pack);
}

void ServerSpellCastEnvironment::apply(BattleLogMessage * pack)
{
	gh->sendAndApply(pack);
}

void ServerSpellCastEnvironment::apply(BattleStackMoved * pack)
{
	gh->sendAndApply(pack);
}

void ServerSpellCastEnvironment::apply(BattleUnitsChanged * pack)
{
	gh->sendAndApply(pack);
}

void ServerSpellCastEnvironment::apply(SetStackEffect * pack)
{
	gh->sendAndApply(pack);
}

void ServerSpellCastEnvironment::apply(StacksInjured * pack)
{
	gh->sendAndApply(pack);
}

void ServerSpellCastEnvironment::apply(BattleObstaclesChanged * pack)
{
	gh->sendAndApply(pack);
}

void ServerSpellCastEnvironment::apply(CatapultAttack * pack)
{
	gh->sendAndApply(pack);
}

const CGameInfoCallback * ServerSpellCastEnvironment::getCb() const
{
	return gh;
}

const CMap * ServerSpellCastEnvironment::getMap() const
{
	return gh->gameState()->map;
}

bool ServerSpellCastEnvironment::moveHero(ObjectInstanceID hid, int3 dst, bool teleporting)
{
	return gh->moveHero(hid, dst, teleporting, false);
}

void ServerSpellCastEnvironment::genericQuery(Query * request, PlayerColor color, std::function<void(const JsonNode&)> callback)
{
	auto query = std::make_shared<CGenericQuery>(&gh->queries, color, callback);
	request->queryID = query->queryID;
	gh->queries.addQuery(query);
	gh->sendAndApply(request);
}
