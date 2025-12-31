/*
 * BattleProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleProcessor.h"

#include "BattleActionProcessor.h"
#include "BattleFlowProcessor.h"
#include "BattleResultProcessor.h"

#include "../CGameHandler.h"
#include "../queries/QueriesProcessor.h"
#include "../queries/BattleQueries.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/battle/CObstacleInstance.h"
#include "../../lib/battle/BattleInfo.h"
#include "../../lib/battle/BattleLayout.h"
#include "../../lib/entities/building/TownFortifications.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapObjects/CGHeroInstance.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/networkPacks/PacksForClient.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/CPlayerState.h"
#include <vstd/RNG.h>

BattleProcessor::BattleProcessor(CGameHandler * gameHandler)
	: gameHandler(gameHandler)
	, actionsProcessor(std::make_unique<BattleActionProcessor>(this, gameHandler))
	, flowProcessor(std::make_unique<BattleFlowProcessor>(this, gameHandler))
	, resultProcessor(std::make_unique<BattleResultProcessor>(gameHandler))
{
}

BattleProcessor::~BattleProcessor() = default;

void BattleProcessor::engageIntoBattle(PlayerColor player)
{
	//notify interfaces
	PlayerBlocked pb;
	pb.player = player;
	pb.reason = PlayerBlocked::UPCOMING_BATTLE;
	pb.startOrEnd = PlayerBlocked::BLOCKADE_STARTED;
	gameHandler->sendAndApply(pb);
}

void BattleProcessor::restartBattle(const BattleID & battleID, const CArmedInstance *army1, const CArmedInstance *army2, int3 tile,
								const CGHeroInstance *hero1, const CGHeroInstance *hero2, const BattleLayout & layout, const CGTownInstance *town)
{
	auto battle = gameHandler->gameState().getBattle(battleID);

	auto lastBattleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(battle->getSide(BattleSide::ATTACKER).color));
	if(!lastBattleQuery)
		lastBattleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(battle->getSide(BattleSide::DEFENDER).color));

	assert(lastBattleQuery);

	//existing battle query for retying auto-combat
	if(lastBattleQuery)
	{
		BattleSideArray<const CGHeroInstance*> heroes{hero1, hero2};

		for(auto i : {BattleSide::ATTACKER, BattleSide::DEFENDER})
		{
			if(heroes[i])
			{
				SetMana restoreInitialMana;
				restoreInitialMana.val = battle->getSide(i).initialMana;
				restoreInitialMana.hid = heroes[i]->id;
				restoreInitialMana.mode = ChangeValueMode::ABSOLUTE;
				gameHandler->sendAndApply(restoreInitialMana);
			}
		}

		lastBattleQuery->result = std::nullopt;

		assert(lastBattleQuery->belligerents[BattleSide::ATTACKER] == battle->getSideArmy(BattleSide::ATTACKER));
		assert(lastBattleQuery->belligerents[BattleSide::DEFENDER] == battle->getSideArmy(BattleSide::DEFENDER));
	}

	BattleCancelled bc;
	bc.battleID = battleID;
	gameHandler->sendAndApply(bc);

	startBattle(army1, army2, tile, hero1, hero2, layout, town);
}

void BattleProcessor::startBattle(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile,
								const CGHeroInstance *hero1, const CGHeroInstance *hero2, const BattleLayout & layout, const CGTownInstance *town)
{
	assert(gameHandler->gameState().getBattle(army1->getOwner()) == nullptr);
	assert(gameHandler->gameState().getBattle(army2->getOwner()) == nullptr);

	BattleSideArray<const CArmedInstance *> armies{army1, army2};
	BattleSideArray<const CGHeroInstance*>heroes{hero1, hero2};

	auto battleID = setupBattle(tile, armies, heroes, layout, town); //initializes stacks, places creatures on battlefield, blocks and informs player interfaces

	const auto * battle = gameHandler->gameState().getBattle(battleID);
	assert(battle);
	
	//add battle bonuses based from player state only when attacks neutral creatures
	const auto * attackerInfo = gameHandler->gameInfo().getPlayerState(army1->getOwner(), false);
	if(attackerInfo && !army2->getOwner().isValidPlayer())
	{
		for(const auto & bonus : attackerInfo->battleBonuses)
		{
			GiveBonus giveBonus(GiveBonus::ETarget::OBJECT);
			giveBonus.id = hero1->id;
			giveBonus.bonus = bonus;
			gameHandler->sendAndApply(giveBonus);
		}
	}

	auto lastBattleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(battle->getSide(BattleSide::ATTACKER).color));
	if(!lastBattleQuery)
		lastBattleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(battle->getSide(BattleSide::DEFENDER).color));

	if (lastBattleQuery)
	{
		lastBattleQuery->battleID = battleID;
	}
	else
	{
		auto newBattleQuery = std::make_shared<CBattleQuery>(gameHandler, battle);
		gameHandler->queries->addQuery(newBattleQuery);
	}

	flowProcessor->onBattleStarted(*battle);
}

void BattleProcessor::startBattle(const CArmedInstance *army1, const CArmedInstance *army2)
{
	startBattle(army1, army2, army2->visitablePos(),
		army1->ID == Obj::HERO ? dynamic_cast<const CGHeroInstance*>(army1) : nullptr,
		army2->ID == Obj::HERO ? dynamic_cast<const CGHeroInstance*>(army2) : nullptr,
		BattleLayout::createDefaultLayout(gameHandler->gameInfo(), army1, army2),
		nullptr);
}

BattleID BattleProcessor::setupBattle(int3 tile, BattleSideArray<const CArmedInstance *> armies, BattleSideArray<const CGHeroInstance *> heroes, const BattleLayout & layout, const CGTownInstance *town)
{
	const auto & t = *gameHandler->gameInfo().getTile(tile);
	TerrainId terrain = t.getTerrainID();
	if (town)
		terrain = town->getNativeTerrain();
	else if (gameHandler->gameState().getMap().isCoastalTile(tile)) //coastal tile is always ground
		terrain = ETerrainId::SAND;

	BattleField battlefieldType = gameHandler->gameState().battleGetBattlefieldType(tile, gameHandler->getRandomGenerator());

	if (town)
	{
		const TerrainType* terrainData = LIBRARY->terrainTypeHandler->getById(terrain);
		battlefieldType = BattleField(*RandomGeneratorUtil::nextItem(terrainData->battleFields, gameHandler->getRandomGenerator()));
	}
	else if (heroes[BattleSide::ATTACKER] && heroes[BattleSide::ATTACKER]->inBoat() && heroes[BattleSide::DEFENDER] && heroes[BattleSide::DEFENDER]->inBoat())
		battlefieldType = BattleField(*LIBRARY->identifiers()->getIdentifier("core", "battlefield.ship_to_ship"));

	//send info about battles
	BattleStart bs;
	bs.info = BattleInfo::setupBattle(&gameHandler->gameInfo(), tile, terrain, battlefieldType, armies, heroes, layout, town);
	bs.battleID = gameHandler->gameState().nextBattleID;

	engageIntoBattle(bs.info->getSide(BattleSide::ATTACKER).color);
	engageIntoBattle(bs.info->getSide(BattleSide::DEFENDER).color);

	auto lastBattleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(bs.info->getSide(BattleSide::ATTACKER).color));
	if(!lastBattleQuery)
		lastBattleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(bs.info->getSide(BattleSide::DEFENDER).color));
	bool isDefenderHuman = bs.info->getSide(BattleSide::DEFENDER).color.isValidPlayer() && gameHandler->gameInfo().getPlayerState(bs.info->getSide(BattleSide::DEFENDER).color)->isHuman();
	bool isAttackerHuman = gameHandler->gameInfo().getPlayerState(bs.info->getSide(BattleSide::ATTACKER).color)->isHuman();

	bool onlyOnePlayerHuman = isDefenderHuman != isAttackerHuman;
	bs.info->replayAllowed = lastBattleQuery == nullptr && onlyOnePlayerHuman;

	gameHandler->sendAndApply(bs);

	return bs.battleID;
}

bool BattleProcessor::checkBattleStateChanges(const CBattleInfoCallback & battle)
{
	//check if drawbridge state need to be changes
	if (battle.battleGetFortifications().wallsHealth > 0)
		updateGateState(battle);

	if (resultProcessor->battleIsEnding(battle))
		return true;

	//check if battle ended
	if (auto result = battle.battleIsFinished())
	{
		setBattleResult(battle, EBattleResult::NORMAL, *result);
		return true;
	}

	return false;
}

void BattleProcessor::updateGateState(const CBattleInfoCallback & battle)
{
	// GATE_BRIDGE - leftmost tile, located over moat
	// GATE_OUTER - central tile, mostly covered by gate image
	// GATE_INNER - rightmost tile, inside the walls

	// GATE_OUTER or GATE_INNER:
	// - if defender moves unit on these tiles, bridge will open
	// - if there is a creature (dead or alive) on these tiles, bridge will always remain open
	// - blocked to attacker if bridge is closed

	// GATE_BRIDGE
	// - if there is a unit or corpse here, bridge can't open (and can't close in fortress)
	// - if Force Field is cast here, bridge can't open (but can close, in any town)
	// - deals moat damage to attacker if bridge is closed (fortress only)

	bool hasForceFieldOnBridge = !battle.battleGetAllObstaclesOnPos(BattleHex(BattleHex::GATE_BRIDGE), true).empty();
	bool hasStackAtGateInner   = battle.battleGetUnitByPos(BattleHex(BattleHex::GATE_INNER), false) != nullptr;
	bool hasStackAtGateOuter   = battle.battleGetUnitByPos(BattleHex(BattleHex::GATE_OUTER), false) != nullptr;
	bool hasStackAtGateBridge  = battle.battleGetUnitByPos(BattleHex(BattleHex::GATE_BRIDGE), false) != nullptr;
	bool hasWideMoat           = vstd::contains_if(battle.battleGetAllObstaclesOnPos(BattleHex(BattleHex::GATE_BRIDGE), false), [](const std::shared_ptr<const CObstacleInstance> & obst)
	{
		return obst->obstacleType == CObstacleInstance::MOAT;
	});

	BattleUpdateGateState db;
	db.state = battle.battleGetGateState();
	db.battleID = battle.getBattle()->getBattleID();

	if (battle.battleGetWallState(EWallPart::GATE) == EWallState::DESTROYED)
	{
		db.state = EGateState::DESTROYED;
	}
	else if (db.state == EGateState::OPENED)
	{
		bool hasStackOnLongBridge = hasStackAtGateBridge && hasWideMoat;
		bool gateCanClose = !hasStackAtGateInner && !hasStackAtGateOuter && !hasStackOnLongBridge;

		if (gateCanClose)
			db.state = EGateState::CLOSED;
		else
			db.state = EGateState::OPENED;
	}
	else // CLOSED or BLOCKED
	{
		bool gateBlocked = hasForceFieldOnBridge || hasStackAtGateBridge;

		if (gateBlocked)
			db.state = EGateState::BLOCKED;
		else
			db.state = EGateState::CLOSED;
	}

	if (db.state != battle.battleGetGateState())
		gameHandler->sendAndApply(db);
}

bool BattleProcessor::makePlayerBattleAction(const BattleID & battleID, PlayerColor player, const BattleAction &ba)
{
	const auto * battle = gameHandler->gameState().getBattle(battleID);

	if (!battle)
		return false;

	bool result = actionsProcessor->makePlayerBattleAction(*battle, player, ba);
	if (gameHandler->gameState().getBattle(battleID) != nullptr && !resultProcessor->battleIsEnding(*battle))
		flowProcessor->onActionMade(*battle, ba);
	return result;
}

void BattleProcessor::setBattleResult(const CBattleInfoCallback & battle, EBattleResult resultType, BattleSide victoriusSide)
{
	resultProcessor->setBattleResult(battle, resultType, victoriusSide);
	resultProcessor->endBattle(battle);
}

bool BattleProcessor::makeAutomaticBattleAction(const CBattleInfoCallback & battle, const BattleAction &ba)
{
	return actionsProcessor->makeAutomaticBattleAction(battle, ba);
}

void BattleProcessor::endBattleConfirm(const BattleID & battleID)
{
	auto battle = gameHandler->gameState().getBattle(battleID);
	assert(battle);

	if (!battle)
		return;

	resultProcessor->endBattleConfirm(*battle);
}

void BattleProcessor::battleFinalize(const BattleID & battleID, const BattleResult &result)
{
	resultProcessor->battleFinalize(battleID, result);
}
