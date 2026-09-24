/*
 * BattleActivities.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleActivities.h"
#include "MapActivities.h"
#include "ActivityProcessor.h"
#include "VisitActivities.h"

#include "../CGameHandler.h"
#include "../battles/BattleProcessor.h"

#include "../../lib/battle/IBattleState.h"
#include "../../lib/battle/BattleLayout.h"
#include "../../lib/battle/SideInBattle.h"
#include "../../lib/CPlayerState.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/networkPacks/PacksForServer.h"

bool BattleActivity::hasPendingBattleOrVisitActivities() const
{
	return std::any_of(players.begin(), players.end(), [this](const PlayerColor & player)
	{
		auto top = owner->topActivity(player);
		return top.get() == this || std::dynamic_pointer_cast<MapObjectVisitActivity>(top);
	});
}

std::vector<ObjectInstanceID> BattleActivity::takeDeferredLevelUps()
{
	deferredLevelUpsApplied = true;
	auto deferredLevelUps = std::move(heroesWithDeferredLevelUp);
	heroesWithDeferredLevelUp.clear();
	return deferredLevelUps;
}

void BattleActivity::completeDeferredLevelUps() const
{
	if(deferredLevelUpsApplied)
		return;

	deferredLevelUpsApplied = true;
	for(const auto & heroID : heroesWithDeferredLevelUp)
		if(const auto * hero = gh->gameState().getHero(heroID))
			gh->expGiven(hero);
}

void BattleActivity::notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, int32_t continuationTag) const
{
	assert(result);

	if(result)
		visitedObject->battleFinished(*gh, visitingHero, *result);
}

BattleActivity::BattleActivity(CGameHandler * owner, const IBattleInfo * bi):
	Activity(owner, TYPE),
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

BattleActivity::BattleActivity(CGameHandler * owner):
	Activity(owner, TYPE)
{
	belligerents[BattleSide::ATTACKER] = nullptr;
	belligerents[BattleSide::DEFENDER] = nullptr;
}

bool BattleActivity::blocksPack(const CPackForServer * pack) const
{
	if(dynamic_cast<const MakeAction*>(pack) != nullptr)
		return false;

	if(dynamic_cast<const GamePause*>(pack) != nullptr)
		return false;

	if(const auto * trade = dynamic_cast<const TradeOnMarketplace *>(pack); trade && trade->mode == EMarketMode::RESOURCE_RESOURCE)
		return false;

	return true;
}

void BattleActivity::onRemoval(PlayerColor color)
{
	assert(result);

	if(result)
		gh->battles->battleFinalize(battleID, *result);

	// Guarded map object visits are notified after the battle activity is removed, so
	// level-ups are postponed until the object applies its battle result. In multiplayer
	// battles they also wait until this activity is removed for every player.
	if(!hasPendingBattleOrVisitActivities())
		completeDeferredLevelUps();
}

void BattleActivity::onExposure(ActivityPtr topActivity)
{
	// this method may be called in two cases:
	// 1) when requesting battle replay (but before replay starts -> no valid result)
	// 2) when aswering on levelup activities after accepting battle result -> valid result
	if(result)
		owner->popActivity(*this);
}

BattleResultActivity::BattleResultActivity(CGameHandler * owner, const IBattleInfo * bi, const std::optional<BattleResult> & Br):
	DialogActivity(owner, TYPE),
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

void BattleResultActivity::onRemoval(PlayerColor color)
{
	// answer to this activity was already processed when handling 1st player
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
