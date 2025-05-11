/*
 * CBattleCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CBattleCallback.h"

#include "CGameInterface.h"

#include "../battle/CPlayerBattleCallback.h"
#include "../battle/IBattleState.h"
#include "../networkPacks/PacksForServer.h"

VCMI_LIB_NAMESPACE_BEGIN

CBattleCallback::CBattleCallback(std::optional<PlayerColor> player, CClient * C):
	cl(C),
	player(player)
{
}

void CBattleCallback::battleMakeUnitAction(const BattleID & battleID, const BattleAction & action)
{
	MakeAction ma;
	ma.ba = action;
	ma.battleID = battleID;
	sendRequest(ma);
}

void CBattleCallback::battleMakeTacticAction(const BattleID & battleID, const BattleAction & action )
{
	MakeAction ma;
	ma.ba = action;
	ma.battleID = battleID;
	sendRequest(ma);
}

std::optional<BattleAction> CBattleCallback::makeSurrenderRetreatDecision(const BattleID & battleID, const BattleStateInfoForRetreat & battleState)
{
	return cl->playerint[getPlayerID().value()]->makeSurrenderRetreatDecision(battleID, battleState);
}

std::shared_ptr<CPlayerBattleCallback> CBattleCallback::getBattle(const BattleID & battleID)
{
	if (activeBattles.count(battleID))
		return activeBattles.at(battleID);

	throw std::runtime_error("Failed to find battle " + std::to_string(battleID.getNum()) + " of player " + player->toString() + ". Number of ongoing battles: " + std::to_string(activeBattles.size()));
}

std::optional<PlayerColor> CBattleCallback::getPlayerID() const
{
	return player;
}

void CBattleCallback::onBattleStarted(const IBattleInfo * info)
{
	if (activeBattles.count(info->getBattleID()) > 0)
		throw std::runtime_error("Player " + player->toString() + " is already engaged in battle " + std::to_string(info->getBattleID().getNum()));

	logGlobal->debug("Battle %d started for player %s", info->getBattleID(), player->toString());
	activeBattles[info->getBattleID()] = std::make_shared<CPlayerBattleCallback>(info, *getPlayerID());
}

void CBattleCallback::onBattleEnded(const BattleID & battleID)
{
	if (activeBattles.count(battleID) == 0)
		throw std::runtime_error("Player " + player->toString() + " is not engaged in battle " + std::to_string(battleID.getNum()));

	logGlobal->debug("Battle %d ended for player %s", battleID, player->toString());
	activeBattles.erase(battleID);
}

VCMI_LIB_NAMESPACE_END
