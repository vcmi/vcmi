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

#include "../../lib/CStack.h"
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
#include "../../lib/spells/CSpell.h"
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

	auto attackerQuery = gameHandler->queries->topQuery(battle->getSide(BattleSide::ATTACKER).color);
	auto * lastBattleQuery = gameHandler->queries->queryAs<CBattleQuery>(attackerQuery);
	if(!lastBattleQuery)
	{
		auto defenderPlayer = battle->getSide(BattleSide::DEFENDER).color;
		if(defenderPlayer.isValidPlayer())
		{
			auto defenderQuery = gameHandler->queries->topQuery(defenderPlayer);
			lastBattleQuery = gameHandler->queries->queryAs<CBattleQuery>(defenderQuery);
		}
	}

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

	startBattle(army1, army2, tile, hero1, hero2, layout, town, true);
}

void BattleProcessor::startBattle(const CArmedInstance *army1, const CArmedInstance *army2, int3 tile,
								const CGHeroInstance *hero1, const CGHeroInstance *hero2, const BattleLayout & layout, const CGTownInstance *town, bool restarted)
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

	auto attackerQuery = gameHandler->queries->topQuery(battle->getSide(BattleSide::ATTACKER).color);
	auto * topBattleQuery = gameHandler->queries->queryAs<CBattleQuery>(attackerQuery);
	if(!topBattleQuery && battle->getSide(BattleSide::DEFENDER).color.isValidPlayer())
	{
		auto defenderQuery = gameHandler->queries->topQuery(battle->getSide(BattleSide::DEFENDER).color);
		topBattleQuery = gameHandler->queries->queryAs<CBattleQuery>(defenderQuery);
	}
	if (topBattleQuery)
	{
		topBattleQuery->battleID = battleID;
	}
	else
	{
		auto newBattleQuery = std::make_shared<CBattleQuery>(gameHandler, battle);
		gameHandler->queries->addQuery(newBattleQuery);
	}

	if (!restarted)
	{
		tryLearnEnemySpellsPreBattle(battle, BattleSide::ATTACKER);
		tryLearnEnemySpellsPreBattle(battle, BattleSide::DEFENDER);
	}

	flowProcessor->onBattleStarted(*battle);
}

void BattleProcessor::tryLearnEnemySpellsPreBattle(const BattleInfo * battle, BattleSide side)
{
	const auto * learner = battle->battleGetFightingHero(side);
	const auto * enemy = battle->battleGetFightingHero(battle->otherSide(side));

	if(!learner || !enemy || !learner->hasSpellbook())
		return;

	const auto eagleEyeLevel = learner->valOfBonuses(BonusType::LEARN_BATTLE_SPELL_LEVEL_LIMIT_PRE_BATTLE);
	if(eagleEyeLevel <= 0)
		return;

	const auto eagleEyeChance = learner->valOfBonuses(BonusType::LEARN_BATTLE_SPELL_CHANCE_PRE_BATTLE);
	if(eagleEyeChance <= 0)
		return;

	ChangeSpells learnedSpells;
	learnedSpells.eagleEyeBonus = true;
	learnedSpells.learn = true;
	learnedSpells.hid = learner->id;

	for(const auto spellID : enemy->getSpellsInSpellbook())
	{
		const auto * spell = spellID.toSpell();
		if(!spell)
			continue;

		if(spell->getLevel() <= eagleEyeLevel && !learner->spellbookContainsSpell(spell->getId()) && gameHandler->getRandomGenerator().nextInt(99) < eagleEyeChance)
			learnedSpells.spells.insert(spell->getId());
	}

	if(!learnedSpells.spells.empty())
		gameHandler->sendAndApply(learnedSpells);
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

	BattleField battlefieldType = gameHandler->gameState().battleGetBattlefieldType(tile, gameHandler->getRandomGenerator());

	// The battle may take place on a terrain dictated by an object rather than the map tile:
	// a town siege uses the town's native terrain, and objects such as an abandoned mine can
	// force e.g. subterranean terrain. In that case the battlefield is the object's fixed one if it
	// defines one, otherwise it is selected from that terrain - keeping terrain, battlefield and
	// obstacles consistent.
	// A town's battle terrain is dictated only through the explicit 'town' parameter; a null town
	// means an outside/field battle that uses the map tile terrain, so the town object sitting on
	// the battle tile must be ignored here.
	const CGObjectInstance * topObject = nullptr;
	if (!town && !t.visitableObjects.empty())
	{
		const auto * tileObject = gameHandler->gameInfo().getObjInstance(t.visitableObjects.front());
		if (tileObject && tileObject->ID != Obj::TOWN)
			topObject = tileObject;
	}

	TerrainId forcedTerrain = town ? town->getBattleTerrain() : (topObject ? topObject->getBattleTerrain() : TerrainId::NONE);

	if (forcedTerrain != TerrainId::NONE)
	{
		terrain = forcedTerrain;
		BattleField forcedBattlefield = topObject ? topObject->getBattlefield() : BattleField::NONE;
		if (forcedBattlefield != BattleField::NONE)
			battlefieldType = forcedBattlefield; // object defines a fixed battlefield explicitly
		else
		{
			const TerrainType * terrainData = LIBRARY->terrainTypeHandler->getById(terrain);
			battlefieldType = BattleField(*RandomGeneratorUtil::nextItem(terrainData->battleFields, gameHandler->getRandomGenerator()));
		}
	}
	else if (gameHandler->gameState().getMap().isCoastalTile(tile)) //coastal tile is always ground
		terrain = ETerrainId::SAND;
	else if (heroes[BattleSide::ATTACKER] && heroes[BattleSide::ATTACKER]->inBoat() && heroes[BattleSide::DEFENDER] && heroes[BattleSide::DEFENDER]->inBoat())
		battlefieldType = BattleField(*LIBRARY->identifiers()->getIdentifier("core", "battlefield.ship_to_ship"));

	//send info about battles
	BattleStart bs;
	bs.info = BattleInfo::setupBattle(&gameHandler->gameInfo(), tile, terrain, battlefieldType, armies, heroes, layout, town);
	bs.battleID = gameHandler->gameState().nextBattleID;

	engageIntoBattle(bs.info->getSide(BattleSide::ATTACKER).color);
	engageIntoBattle(bs.info->getSide(BattleSide::DEFENDER).color);

	auto attackerQuery = gameHandler->queries->topQuery(bs.info->getSide(BattleSide::ATTACKER).color);
	auto * topBattleQuery = gameHandler->queries->queryAs<CBattleQuery>(attackerQuery);
	bool isDefenderHuman = bs.info->getSide(BattleSide::DEFENDER).color.isValidPlayer() && gameHandler->gameInfo().getPlayerState(bs.info->getSide(BattleSide::DEFENDER).color)->isHuman();
	if(!topBattleQuery && isDefenderHuman)
	{
		auto defenderQuery = gameHandler->queries->topQuery(bs.info->getSide(BattleSide::DEFENDER).color);
		topBattleQuery = gameHandler->queries->queryAs<CBattleQuery>(defenderQuery);
	}

	bool isAttackerHuman = gameHandler->gameInfo().getPlayerState(bs.info->getSide(BattleSide::ATTACKER).color)->isHuman();

	bool onlyOnePlayerHuman = isDefenderHuman != isAttackerHuman;
	bs.info->replayAllowed = topBattleQuery == nullptr && onlyOnePlayerHuman;

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

void BattleProcessor::cheatBattleVictory(PlayerColor player)
{
	auto * battle = gameHandler->gameState().getBattle(player);
	if(!battle || resultProcessor->battleIsEnding(*battle))
		return;

	const BattleSide winningSide = battle->playerToSide(player);
	if(winningSide != BattleSide::ATTACKER && winningSide != BattleSide::DEFENDER)
		return;

	BattleUnitsChanged killedUnits;
	killedUnits.battleID = battle->getBattleID();

	for(const CStack * stack : battle->battleGetAllStacks(true))
	{
		if(stack->unitSide() == winningSide || !stack->alive())
			continue;

		auto state = stack->acquireState();
		int64_t damage = state->getAvailableHealth();
		state->damage(damage);

		UnitChanges info(stack->unitId(), UnitChanges::EOperation::UPDATE);
		info.data = state->save();
		info.healthDelta = -damage;
		killedUnits.changedStacks.push_back(info);
	}

	if(!killedUnits.changedStacks.empty())
		gameHandler->sendAndApply(killedUnits);

	setBattleResult(*battle, EBattleResult::NORMAL, winningSide);
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

void BattleProcessor::processBattleEventTriggers(const CBattleInfoCallback & battle, CombatEventType event, const battle::Unit * target, const battle::Unit * secondary)
{
	actionsProcessor->processBattleEventTriggers(battle, event, target, secondary);
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
