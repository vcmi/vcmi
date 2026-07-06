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
#include "VisitQueries.h"

#include "../CGameHandler.h"
#include "../battles/BattleProcessor.h"

#include "../../lib/battle/IBattleState.h"
#include "../../lib/battle/BattleLayout.h"
#include "../../lib/battle/SideInBattle.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/networkPacks/PacksForServer.h"

bool CBattleQuery::hasPendingBattleOrVisitQueries() const
{
	return std::any_of(players.begin(), players.end(), [this](const PlayerColor & player)
	{
		auto top = owner->topQuery(player);
		return top.get() == this || std::dynamic_pointer_cast<MapObjectVisitQuery>(top);
	});
}

std::vector<ObjectInstanceID> CBattleQuery::takeDeferredLevelUps()
{
	deferredLevelUpsApplied = true;
	auto deferredLevelUps = std::move(heroesWithDeferredLevelUp);
	heroesWithDeferredLevelUp.clear();
	return deferredLevelUps;
}

void CBattleQuery::completeDeferredLevelUps() const
{
	if(deferredLevelUpsApplied)
		return;

	deferredLevelUpsApplied = true;
	for(const auto & heroID : heroesWithDeferredLevelUp)
		if(const auto * hero = gh->gameState().getHero(heroID))
			gh->expGiven(hero);
}

void CBattleQuery::notifyObjectAboutRemoval(const CGObjectInstance * visitedObject, const CGHeroInstance * visitingHero) const
{
	assert(result);

	if(result)
		visitedObject->battleFinished(*gh, visitingHero, *result);
}

CBattleQuery::CBattleQuery(CGameHandler * owner, const IBattleInfo * bi):
	CQuery(owner, TYPE),
	battleID(bi->getBattleID())
{
	belligerents[BattleSide::ATTACKER] = bi->getSideArmy(BattleSide::ATTACKER);
	belligerents[BattleSide::DEFENDER] = bi->getSideArmy(BattleSide::DEFENDER);

	auto attacker = bi->getSidePlayer(BattleSide::ATTACKER);
	if(attacker.isValidPlayer())
		addPlayer(attacker);

	auto defender = bi->getSidePlayer(BattleSide::DEFENDER);
	if(defender.isValidPlayer())
		addPlayer(defender);
}

CBattleQuery::CBattleQuery(CGameHandler * owner):
	CQuery(owner, TYPE)
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

	if(const auto * trade = dynamic_cast<const TradeOnMarketplace *>(pack); trade && trade->mode == EMarketMode::RESOURCE_RESOURCE)
		return false;

	return true;
}

void CBattleQuery::onRemoval(PlayerColor color)
{
	assert(result);

	if(result)
		gh->battles->battleFinalize(battleID, *result);

	// Guarded map object visits are notified after the battle query is removed.
	// In that case, defer level-up prompts until the object applies its battle result.
	// In multi-player battles, also wait until this battle query is removed for all players.
	if(!hasPendingBattleOrVisitQueries())
		completeDeferredLevelUps();
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
	CDialogQuery(owner, TYPE),
	bi(bi),
	result(Br)
{
	auto attacker = bi->getSidePlayer(BattleSide::ATTACKER);
	if(attacker.isValidPlayer())
		addPlayer(attacker);

	auto defender = bi->getSidePlayer(BattleSide::DEFENDER);
	if(defender.isValidPlayer())
		addPlayer(defender);
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
