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

#include "../CGameHandler.h"
#include "../battles/BattleProcessor.h"

#include "../../lib/battle/IBattleState.h"
#include "../../lib/battle/BattleLayout.h"
#include "../../lib/battle/SideInBattle.h"
#include "../../lib/mapObjects/CGObjectInstance.h"
#include "../../lib/networkPacks/PacksForServer.h"

void BattleActivity::notifyObjectAboutRemoval(const IObjectInterface * visitedObject, const CGHeroInstance * visitingHero, const JsonNode & visitState) const
{
	if(!result)
		throw std::runtime_error("Battle activity removed without a result");

	visitedObject->battleFinished(*gh, visitingHero, *result);
}

BattleActivity::BattleActivity(CGameHandler * owner, const IBattleInfo * bi):
	Activity(owner, TYPE),
	battleID(bi->getBattleID())
{
	belligerents[BattleSide::ATTACKER] = bi->getSideArmy(BattleSide::ATTACKER);
	belligerents[BattleSide::DEFENDER] = bi->getSideArmy(BattleSide::DEFENDER);

	// Only the defender can be neutral
	addPlayer(bi->getSidePlayer(BattleSide::ATTACKER));

	auto defender = bi->getSidePlayer(BattleSide::DEFENDER);
	if(defender.isValidPlayer())
		addPlayer(defender);
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

BattleResultActivity::BattleResultActivity(CGameHandler * owner, const IBattleInfo * bi, const std::optional<BattleResult> & Br):
	DialogActivity(owner, TYPE),
	bi(bi),
	result(Br)
{
	// Only the defender can be neutral
	addPlayer(bi->getSidePlayer(BattleSide::ATTACKER));

	auto defender = bi->getSidePlayer(BattleSide::DEFENDER);
	if(defender.isValidPlayer())
		addPlayer(defender);
}

void BattleResultActivity::onRemoval()
{
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
}
