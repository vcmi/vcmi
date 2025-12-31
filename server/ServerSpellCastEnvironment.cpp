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
#include "ServerSpellCastEnvironment.h"

#include "CGameHandler.h"
#include "queries/QueriesProcessor.h"
#include "queries/CQuery.h"

#include "../lib/gameState/CGameState.h"
#include "../lib/networkPacks/PacksForClientBattle.h"
#include "../lib/networkPacks/SetStackEffect.h"

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

void ServerSpellCastEnvironment::apply(CPackForClient & pack)
{
	gh->sendAndApply(pack);
}

void ServerSpellCastEnvironment::apply(BattleLogMessage & pack)
{
	gh->sendAndApply(pack);
}

void ServerSpellCastEnvironment::apply(BattleStackMoved & pack)
{
	gh->sendAndApply(pack);
}

void ServerSpellCastEnvironment::apply(BattleUnitsChanged & pack)
{
	gh->sendAndApply(pack);
}

void ServerSpellCastEnvironment::apply(SetStackEffect & pack)
{
	gh->sendAndApply(pack);
}

void ServerSpellCastEnvironment::apply(StacksInjured & pack)
{
	gh->sendAndApply(pack);
}

void ServerSpellCastEnvironment::apply(BattleObstaclesChanged & pack)
{
	gh->sendAndApply(pack);
}

void ServerSpellCastEnvironment::apply(CatapultAttack & pack)
{
	gh->sendAndApply(pack);
}

const IGameInfoCallback * ServerSpellCastEnvironment::getCb() const
{
	return &gh->gameInfo();
}

const CMap * ServerSpellCastEnvironment::getMap() const
{
	return &gh->gameState().getMap();
}

bool ServerSpellCastEnvironment::moveHero(ObjectInstanceID hid, int3 dst, EMovementMode mode)
{
	return gh->moveHero(hid, dst, mode, false);
}

void ServerSpellCastEnvironment::createBoat(const int3 & visitablePosition, BoatId type, PlayerColor initiator)
{
	return gh->createBoat(visitablePosition, type, initiator);
}

void ServerSpellCastEnvironment::genericQuery(Query * request, PlayerColor color, std::function<void(std::optional<int32_t>)> callback)
{
	auto query = std::make_shared<CGenericQuery>(gh, color, callback);
	request->queryID = query->queryID;
	gh->queries->addQuery(query);
	gh->sendAndApply(*request);
}
