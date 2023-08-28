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

#include "../../lib/TerrainHandler.h"
#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/battle/BattleInfo.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/modding/IdentifierStorage.h"

BattleProcessor::BattleProcessor(CGameHandler * gameHandler)
	: gameHandler(gameHandler)
	, flowProcessor(std::make_unique<BattleFlowProcessor>(this))
	, actionsProcessor(std::make_unique<BattleActionProcessor>(this))
	, resultProcessor(std::make_unique<BattleResultProcessor>(this))
{
	setGameHandler(gameHandler);
}

BattleProcessor::BattleProcessor():
	BattleProcessor(nullptr)
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
	gameHandler->sendAndApply(&pb);
}

void BattleProcessor::startBattlePrimary(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile,
								const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool creatureBank,
								const CGTownInstance *town) //use hero=nullptr for no hero
{
	assert(gameHandler->gameState()->getBattle(army1->getOwner()) == nullptr);
	assert(gameHandler->gameState()->getBattle(army2->getOwner()) == nullptr);

	engageIntoBattle(army1->tempOwner);
	engageIntoBattle(army2->tempOwner);

	static const CArmedInstance *armies[2];
	armies[0] = army1;
	armies[1] = army2;
	static const CGHeroInstance*heroes[2];
	heroes[0] = hero1;
	heroes[1] = hero2;

	auto battleID = setupBattle(tile, armies, heroes, creatureBank, town); //initializes stacks, places creatures on battlefield, blocks and informs player interfaces

	const auto * battle = gameHandler->gameState()->getBattle(battleID);
	assert(battle);

	auto lastBattleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(battle->sides[0].color));

	//existing battle query for retying auto-combat
	if(lastBattleQuery)
	{
		for(int i : {0, 1})
		{
			if(heroes[i])
			{
				SetMana restoreInitialMana;
				restoreInitialMana.val = lastBattleQuery->initialHeroMana[i];
				restoreInitialMana.hid = heroes[i]->id;
				gameHandler->sendAndApply(&restoreInitialMana);
			}
		}

		lastBattleQuery->bi = battle;
		lastBattleQuery->result = std::nullopt;
		lastBattleQuery->belligerents[0] = battle->sides[0].armyObject;
		lastBattleQuery->belligerents[1] = battle->sides[1].armyObject;
	}

	auto nextBattleQuery = std::make_shared<CBattleQuery>(gameHandler, battle);
	for(int i : {0, 1})
	{
		if(heroes[i])
		{
			nextBattleQuery->initialHeroMana[i] = heroes[i]->mana;
		}
	}
	gameHandler->queries->addQuery(nextBattleQuery);

	flowProcessor->onBattleStarted(*battle);
}

void BattleProcessor::startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile, bool creatureBank)
{
	startBattlePrimary(army1, army2, tile,
		army1->ID == Obj::HERO ? static_cast<const CGHeroInstance*>(army1) : nullptr,
		army2->ID == Obj::HERO ? static_cast<const CGHeroInstance*>(army2) : nullptr,
		creatureBank);
}

void BattleProcessor::startBattleI(const CArmedInstance *army1, const CArmedInstance *army2, bool creatureBank)
{
	startBattleI(army1, army2, army2->visitablePos(), creatureBank);
}

BattleID BattleProcessor::setupBattle(int3 tile, const CArmedInstance *armies[2], const CGHeroInstance *heroes[2], bool creatureBank, const CGTownInstance *town)
{
	const auto & t = *gameHandler->getTile(tile);
	TerrainId terrain = t.terType->getId();
	if (gameHandler->gameState()->map->isCoastalTile(tile)) //coastal tile is always ground
		terrain = ETerrainId::SAND;

	BattleField terType = gameHandler->gameState()->battleGetBattlefieldType(tile, gameHandler->getRandomGenerator());
	if (heroes[0] && heroes[0]->boat && heroes[1] && heroes[1]->boat)
		terType = BattleField(*VLC->identifiers()->getIdentifier("core", "battlefield.ship_to_ship"));

	//send info about battles
	BattleStart bs;
	bs.info = BattleInfo::setupBattle(tile, terrain, terType, armies, heroes, creatureBank, town);
	bs.battleID = gameHandler->gameState()->nextBattleID;

	engageIntoBattle(bs.info->sides[0].color);
	engageIntoBattle(bs.info->sides[1].color);

	auto lastBattleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(bs.info->sides[0].color));
	bs.info->replayAllowed = lastBattleQuery == nullptr && !bs.info->sides[1].color.isValidPlayer();

	gameHandler->sendAndApply(&bs);

	return bs.battleID;
}

bool BattleProcessor::checkBattleStateChanges(const CBattleInfoCallback & battle)
{
	//check if drawbridge state need to be changes
	if (battle.battleGetSiegeLevel() > 0)
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
		gameHandler->sendAndApply(&db);
}

bool BattleProcessor::makePlayerBattleAction(const BattleID & battleID, PlayerColor player, const BattleAction &ba)
{
	const auto * battle = gameHandler->gameState()->getBattle(battleID);

	if (!battle)
		return false;

	bool result = actionsProcessor->makePlayerBattleAction(*battle, player, ba);
	if (gameHandler->gameState()->getBattle(battleID) != nullptr && !resultProcessor->battleIsEnding(*battle))
		flowProcessor->onActionMade(*battle, ba);
	return result;
}

void BattleProcessor::setBattleResult(const CBattleInfoCallback & battle, EBattleResult resultType, int victoriusSide)
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
	auto battle = gameHandler->gameState()->getBattle(battleID);
	assert(battle);

	if (!battle)
		return;

	resultProcessor->endBattleConfirm(*battle);
}

void BattleProcessor::battleAfterLevelUp(const BattleID & battleID, const BattleResult &result)
{
	auto battle = gameHandler->gameState()->getBattle(battleID);
	assert(battle);

	if (!battle)
		return;

	resultProcessor->battleAfterLevelUp(*battle, result);
}

void BattleProcessor::setGameHandler(CGameHandler * newGameHandler)
{
	gameHandler = newGameHandler;

	actionsProcessor->setGameHandler(newGameHandler);
	flowProcessor->setGameHandler(newGameHandler);
	resultProcessor->setGameHandler(newGameHandler);
}
