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

#include "../CGameHandler.h"
#include "../CVCMIServer.h"
#include "../processors/HeroPoolProcessor.h"
#include "../queries/QueriesProcessor.h"
#include "../queries/BattleQueries.h"

#include "../../lib/ArtifactUtils.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/CStack.h"
#include "../../lib/CondSh.h"
#include "../../lib/GameSettings.h"
#include "../../lib/ScopeGuard.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/UnlockGuard.h"
#include "../../lib/battle/BattleInfo.h"
#include "../../lib/battle/CUnitState.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/modding/IdentifierStorage.h"
#include "../../lib/serializer/Cast.h"
#include "../../lib/spells/AbilityCaster.h"
#include "../../lib/spells/BonusCaster.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/spells/ObstacleCasterProxy.h"
#include "../../lib/spells/Problem.h"

#define COMPLAIN_RET_IF(cond, txt) do {if (cond){gameHandler->complain(txt); return;}} while(0)
#define COMPLAIN_RET_FALSE_IF(cond, txt) do {if (cond){gameHandler->complain(txt); return false;}} while(0)
#define COMPLAIN_RET(txt) {gameHandler->complain(txt); return false;}
#define COMPLAIN_RETF(txt, FORMAT) {gameHandler->complain(boost::str(boost::format(txt) % FORMAT)); return false;}

CondSh<bool> battleMadeAction(false);
CondSh<BattleResult *> battleResult(nullptr);
boost::recursive_mutex battleActionMutex;
static EndAction end_action;

static void giveExp(BattleResult &r)
{
	if (r.winner > 1)
	{
		// draw
		return;
	}
	r.exp[0] = 0;
	r.exp[1] = 0;
	for (auto i = r.casualties[!r.winner].begin(); i!=r.casualties[!r.winner].end(); i++)
	{
		r.exp[r.winner] += VLC->creh->objects.at(i->first)->valOfBonuses(BonusType::STACK_HEALTH) * i->second;
	}
}

static void summonGuardiansHelper(std::vector<BattleHex> & output, const BattleHex & targetPosition, ui8 side, bool targetIsTwoHex) //return hexes for summoning two hex monsters in output, target = unit to guard
{
	int x = targetPosition.getX();
	int y = targetPosition.getY();

	const bool targetIsAttacker = side == BattleSide::ATTACKER;

	if (targetIsAttacker) //handle front guardians, TODO: should we handle situation when units start battle near opposite side of the battlefield? Cannot happen in normal H3...
		BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::RIGHT, false), output);
	else
		BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::LEFT, false), output);

	//guardian spawn locations for four default position cases for attacker and defender, non-default starting location for att and def is handled in first two if's
	if (targetIsAttacker && ((y % 2 == 0) || (x > 1)))
	{
		if (targetIsTwoHex && (y % 2 == 1) && (x == 2)) //handle exceptional case
		{
			BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::TOP_RIGHT, false), output);
			BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::BOTTOM_RIGHT, false), output);
		}
		else
		{	//add back-side guardians for two-hex target, side guardians for one-hex
			BattleHex::checkAndPush(targetPosition.cloneInDirection(targetIsTwoHex ? BattleHex::EDir::TOP_LEFT : BattleHex::EDir::TOP_RIGHT, false), output);
			BattleHex::checkAndPush(targetPosition.cloneInDirection(targetIsTwoHex ? BattleHex::EDir::BOTTOM_LEFT : BattleHex::EDir::BOTTOM_RIGHT, false), output);

			if (!targetIsTwoHex && x > 2) //back guard for one-hex
				BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false), output);
			else if (targetIsTwoHex)//front-side guardians for two-hex target
			{
				BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::TOP_RIGHT, false), output);
				BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::BOTTOM_RIGHT, false), output);
				if (x > 3) //back guard for two-hex
					BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::LEFT, false), output);
			}
		}

	}

	else if (!targetIsAttacker && ((y % 2 == 1) || (x < GameConstants::BFIELD_WIDTH - 2)))
	{
		if (targetIsTwoHex && (y % 2 == 0) && (x == GameConstants::BFIELD_WIDTH - 3)) //handle exceptional case... equivalent for above for defender side
		{
			BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::TOP_LEFT, false), output);
			BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false), output);
		}
		else
		{
			BattleHex::checkAndPush(targetPosition.cloneInDirection(targetIsTwoHex ? BattleHex::EDir::TOP_RIGHT : BattleHex::EDir::TOP_LEFT, false), output);
			BattleHex::checkAndPush(targetPosition.cloneInDirection(targetIsTwoHex ? BattleHex::EDir::BOTTOM_RIGHT : BattleHex::EDir::BOTTOM_LEFT, false), output);

			if (!targetIsTwoHex && x < GameConstants::BFIELD_WIDTH - 3)
				BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false), output);
			else if (targetIsTwoHex)
			{
				BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::TOP_LEFT, false), output);
				BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false), output);
				if (x < GameConstants::BFIELD_WIDTH - 4)
					BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::RIGHT, false), output);
			}
		}
	}

	else if (!targetIsAttacker && y % 2 == 0)
	{
		BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::TOP_LEFT, false), output);
		BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false), output);
	}

	else if (targetIsAttacker && y % 2 == 1)
	{
		BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::TOP_RIGHT, false), output);
		BattleHex::checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::BOTTOM_RIGHT, false), output);
	}
}


CasualtiesAfterBattle::CasualtiesAfterBattle(const SideInBattle & battleSide, const BattleInfo * bat):
	army(battleSide.armyObject)
{
	heroWithDeadCommander = ObjectInstanceID();

	PlayerColor color = battleSide.color;

	for(CStack * st : bat->stacks)
	{
		if(st->summoned) //don't take into account temporary summoned stacks
			continue;
		if(st->unitOwner() != color) //remove only our stacks
			continue;

		logGlobal->debug("Calculating casualties for %s", st->nodeName());

		st->health.takeResurrected();

		if(st->unitSlot() == SlotID::ARROW_TOWERS_SLOT)
		{
			logGlobal->debug("Ignored arrow towers stack.");
		}
		else if(st->unitSlot() == SlotID::WAR_MACHINES_SLOT)
		{
			auto warMachine = st->unitType()->warMachine;

			if(warMachine == ArtifactID::NONE)
			{
				logGlobal->error("Invalid creature in war machine virtual slot. Stack: %s", st->nodeName());
			}
			//catapult artifact remain even if "creature" killed in siege
			else if(warMachine != ArtifactID::CATAPULT && st->getCount() <= 0)
			{
				logGlobal->debug("War machine has been destroyed");
				auto hero = dynamic_ptr_cast<CGHeroInstance> (army);
				if (hero)
					removedWarMachines.push_back (ArtifactLocation(hero, hero->getArtPos(warMachine, true)));
				else
					logGlobal->error("War machine in army without hero");
			}
		}
		else if(st->unitSlot() == SlotID::SUMMONED_SLOT_PLACEHOLDER)
		{
			if(st->alive() && st->getCount() > 0)
			{
				logGlobal->debug("Permanently summoned %d units.", st->getCount());
				const CreatureID summonedType = st->creatureId();
				summoned[summonedType] += st->getCount();
			}
		}
		else if(st->unitSlot() == SlotID::COMMANDER_SLOT_PLACEHOLDER)
		{
			if (nullptr == st->base)
			{
				logGlobal->error("Stack with no base in commander slot. Stack: %s", st->nodeName());
			}
			else
			{
				auto c = dynamic_cast <const CCommanderInstance *>(st->base);
				if(c)
				{
					auto h = dynamic_cast <const CGHeroInstance *>(army);
					if(h && h->commander == c && (st->getCount() == 0 || !st->alive()))
					{
						logGlobal->debug("Commander is dead.");
						heroWithDeadCommander = army->id; //TODO: unify commander handling
					}
				}
				else
					logGlobal->error("Stack with invalid instance in commander slot. Stack: %s", st->nodeName());
			}
		}
		else if(st->base && !army->slotEmpty(st->unitSlot()))
		{
			logGlobal->debug("Count: %d; base count: %d", st->getCount(), army->getStackCount(st->unitSlot()));
			if(st->getCount() == 0 || !st->alive())
			{
				logGlobal->debug("Stack has been destroyed.");
				StackLocation sl(army, st->unitSlot());
				newStackCounts.push_back(TStackAndItsNewCount(sl, 0));
			}
			else if(st->getCount() < army->getStackCount(st->unitSlot()))
			{
				logGlobal->debug("Stack lost %d units.", army->getStackCount(st->unitSlot()) - st->getCount());
				StackLocation sl(army, st->unitSlot());
				newStackCounts.push_back(TStackAndItsNewCount(sl, st->getCount()));
			}
			else if(st->getCount() > army->getStackCount(st->unitSlot()))
			{
				logGlobal->debug("Stack gained %d units.", st->getCount() - army->getStackCount(st->unitSlot()));
				StackLocation sl(army, st->unitSlot());
				newStackCounts.push_back(TStackAndItsNewCount(sl, st->getCount()));
			}
		}
		else
		{
			logGlobal->warn("Unable to process stack: %s", st->nodeName());
		}
	}
}

void CasualtiesAfterBattle::updateArmy(CGameHandler *gh)
{
	for (TStackAndItsNewCount &ncount : newStackCounts)
	{
		if (ncount.second > 0)
			gh->changeStackCount(ncount.first, ncount.second, true);
		else
			gh->eraseStack(ncount.first, true);
	}
	for (auto summoned_iter : summoned)
	{
		SlotID slot = army->getSlotFor(summoned_iter.first);
		if (slot.validSlot())
		{
			StackLocation location(army, slot);
			gh->addToSlot(location, summoned_iter.first.toCreature(), summoned_iter.second);
		}
		else
		{
			//even if it will be possible to summon anything permanently it should be checked for free slot
			//necromancy is handled separately
			gh->complain("No free slot to put summoned creature");
		}
	}
	for (auto al : removedWarMachines)
	{
		gh->removeArtifact(al);
	}
	if (heroWithDeadCommander != ObjectInstanceID())
	{
		SetCommanderProperty scp;
		scp.heroid = heroWithDeadCommander;
		scp.which = SetCommanderProperty::ALIVE;
		scp.amount = 0;
		gh->sendAndApply(&scp);
	}
}

BattleProcessor::BattleProcessor(CGameHandler * gameHandler):
	visitObjectAfterVictory(false),
	gameHandler(gameHandler)
{
}

BattleProcessor::BattleProcessor():
	visitObjectAfterVictory(false),
	gameHandler(nullptr)
{
}

BattleProcessor::~BattleProcessor()
{
	if (battleThread)
	{
		//Setting battleMadeAction is needed because battleThread waits for the action to continue the main loop
		battleMadeAction.setn(true);
		battleThread->join();
	}
}

FinishingBattleHelper::FinishingBattleHelper(std::shared_ptr<const CBattleQuery> Query, int remainingBattleQueriesCount)
{
	assert(Query->result);
	assert(Query->bi);
	auto &result = *Query->result;
	auto &info = *Query->bi;

	winnerHero = result.winner != 0 ? info.sides[1].hero : info.sides[0].hero;
	loserHero = result.winner != 0 ? info.sides[0].hero : info.sides[1].hero;
	victor = info.sides[result.winner].color;
	loser = info.sides[!result.winner].color;
	winnerSide = result.winner;
	this->remainingBattleQueriesCount = remainingBattleQueriesCount;
}

FinishingBattleHelper::FinishingBattleHelper()
{
	winnerHero = loserHero = nullptr;
	winnerSide = 0;
	remainingBattleQueriesCount = 0;
}

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
	if(gameHandler->gameState()->curB)
		gameHandler->gameState()->curB.dellNull();

	engageIntoBattle(army1->tempOwner);
	engageIntoBattle(army2->tempOwner);

	static const CArmedInstance *armies[2];
	armies[0] = army1;
	armies[1] = army2;
	static const CGHeroInstance*heroes[2];
	heroes[0] = hero1;
	heroes[1] = hero2;


	setupBattle(tile, armies, heroes, creatureBank, town); //initializes stacks, places creatures on battlefield, blocks and informs player interfaces

	auto lastBattleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(gameHandler->gameState()->curB->sides[0].color));

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

		lastBattleQuery->bi = gameHandler->gameState()->curB;
		lastBattleQuery->result = std::nullopt;
		lastBattleQuery->belligerents[0] = gameHandler->gameState()->curB->sides[0].armyObject;
		lastBattleQuery->belligerents[1] = gameHandler->gameState()->curB->sides[1].armyObject;
	}

	auto nextBattleQuery = std::make_shared<CBattleQuery>(gameHandler, gameHandler->gameState()->curB);
	for(int i : {0, 1})
	{
		if(heroes[i])
		{
			nextBattleQuery->initialHeroMana[i] = heroes[i]->mana;
		}
	}
	gameHandler->queries->addQuery(nextBattleQuery);

	this->battleThread = std::make_unique<boost::thread>(boost::thread(&BattleProcessor::runBattle, this));
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

void BattleProcessor::runBattle()
{
	boost::unique_lock lock(battleActionMutex);

	gameHandler->setBattle(gameHandler->gameState()->curB);
	assert(gameHandler->gameState()->curB);
	//TODO: pre-tactic stuff, call scripts etc.

	//Moat should be initialized here, because only here we can use spellcasting
	if (gameHandler->gameState()->curB->town && gameHandler->gameState()->curB->town->fortLevel() >= CGTownInstance::CITADEL)
	{
		const auto * h = gameHandler->gameState()->curB->battleGetFightingHero(BattleSide::DEFENDER);
		const auto * actualCaster = h ? static_cast<const spells::Caster*>(h) : nullptr;
		auto moatCaster = spells::SilentCaster(gameHandler->gameState()->curB->getSidePlayer(BattleSide::DEFENDER), actualCaster);
		auto cast = spells::BattleCast(gameHandler->gameState()->curB, &moatCaster, spells::Mode::PASSIVE, gameHandler->gameState()->curB->town->town->moatAbility.toSpell());
		auto target = spells::Target();
		cast.cast(gameHandler->spellEnv, target);
	}

	//tactic round
	{
		while ((gameHandler->gameLobby()->state != EServerState::SHUTDOWN) && gameHandler->gameState()->curB->tacticDistance && !battleResult.get())
		{
			auto unlockGuard = vstd::makeUnlockGuard(battleActionMutex);
			boost::this_thread::sleep(boost::posix_time::milliseconds(50));
		}
	}

	//initial stacks appearance triggers, e.g. built-in bonus spells
	auto initialStacks = gameHandler->gameState()->curB->stacks; //use temporary variable to outclude summoned stacks added to gameHandler->gameState()->curB->stacks from processing

	for (CStack * stack : initialStacks)
	{
		if (stack->hasBonusOfType(BonusType::SUMMON_GUARDIANS))
		{
			std::shared_ptr<const Bonus> summonInfo = stack->getBonus(Selector::type()(BonusType::SUMMON_GUARDIANS));
			auto accessibility = gameHandler->getAccesibility();
			CreatureID creatureData = CreatureID(summonInfo->subtype);
			std::vector<BattleHex> targetHexes;
			const bool targetIsBig = stack->unitType()->isDoubleWide(); //target = creature to guard
			const bool guardianIsBig = creatureData.toCreature()->isDoubleWide();

			/*Chosen idea for two hex units was to cover all possible surrounding hexes of target unit with as small number of stacks as possible.
			For one-hex targets there are four guardians - front, back and one per side (up + down).
			Two-hex targets are wider and the difference is there are two guardians per side to cover 3 hexes + extra hex in the front
			Additionally, there are special cases for starting positions etc., where guardians would be outside of battlefield if spawned normally*/
			if (!guardianIsBig)
				targetHexes = stack->getSurroundingHexes();
			else
				summonGuardiansHelper(targetHexes, stack->getPosition(), stack->unitSide(), targetIsBig);

			for(auto hex : targetHexes)
			{
				if(accessibility.accessible(hex, guardianIsBig, stack->unitSide())) //without this multiple creatures can occupy one hex
				{
					battle::UnitInfo info;
					info.id = gameHandler->gameState()->curB->battleNextUnitId();
					info.count =  std::max(1, (int)(stack->getCount() * 0.01 * summonInfo->val));
					info.type = creatureData;
					info.side = stack->unitSide();
					info.position = hex;
					info.summoned = true;

					BattleUnitsChanged pack;
					pack.changedStacks.emplace_back(info.id, UnitChanges::EOperation::ADD);
					info.save(pack.changedStacks.back().data);
					gameHandler->sendAndApply(&pack);
				}
			}
		}

		stackEnchantedTrigger(stack);
	}

	//spells opening battle
	for (int i = 0; i < 2; ++i)
	{
		auto h = gameHandler->gameState()->curB->battleGetFightingHero(i);
		if (h)
		{
			TConstBonusListPtr bl = h->getBonuses(Selector::type()(BonusType::OPENING_BATTLE_SPELL));

			for (auto b : *bl)
			{
				spells::BonusCaster caster(h, b);

				const CSpell * spell = SpellID(b->subtype).toSpell();

				spells::BattleCast parameters(gameHandler->gameState()->curB, &caster, spells::Mode::PASSIVE, spell);
				parameters.setSpellLevel(3);
				parameters.setEffectDuration(b->val);
				parameters.massive = true;
				parameters.castIfPossible(gameHandler->spellEnv, spells::Target());
			}
		}
	}
	// it is possible that due to opening spells one side was eliminated -> check for end of battle
	checkBattleStateChanges();

	bool firstRound = true;//FIXME: why first round is -1?

	//main loop
	while ((gameHandler->gameLobby()->state != EServerState::SHUTDOWN) && !battleResult.get()) //till the end of the battle ;]
	{
		BattleNextRound bnr;
		bnr.round = gameHandler->gameState()->curB->round + 1;
		logGlobal->debug("Round %d", bnr.round);
		gameHandler->sendAndApply(&bnr);

		auto obstacles = gameHandler->gameState()->curB->obstacles; //we copy container, because we're going to modify it
		for (auto &obstPtr : obstacles)
		{
			if (const SpellCreatedObstacle *sco = dynamic_cast<const SpellCreatedObstacle *>(obstPtr.get()))
				if (sco->turnsRemaining == 0)
					removeObstacle(*obstPtr);
		}

		const BattleInfo & curB = *gameHandler->gameState()->curB;

		for(auto stack : curB.stacks)
		{
			if(stack->alive() && !firstRound)
				stackEnchantedTrigger(stack);
		}

		//stack loop

		auto getNextStack = [this]() -> const CStack *
		{
			if(battleResult.get())
				return nullptr;

			std::vector<battle::Units> q;
			gameHandler->gameState()->curB->battleGetTurnOrder(q, 1, 0, -1); //todo: get rid of "turn -1"

			if(!q.empty())
			{
				if(!q.front().empty())
				{
					auto next = q.front().front();
					const auto stack = dynamic_cast<const CStack *>(next);

					// regeneration takes place before everything else but only during first turn attempt in each round
					// also works under blind and similar effects
					if(stack && stack->alive() && !stack->waiting)
					{
						BattleTriggerEffect bte;
						bte.stackID = stack->unitId();
						bte.effect = vstd::to_underlying(BonusType::HP_REGENERATION);

						const int32_t lostHealth = stack->getMaxHealth() - stack->getFirstHPleft();
						if(stack->hasBonusOfType(BonusType::HP_REGENERATION))
							bte.val = std::min(lostHealth, stack->valOfBonuses(BonusType::HP_REGENERATION));

						if(bte.val) // anything to heal
							gameHandler->sendAndApply(&bte);
					}

					if(next->willMove())
						return stack;
				}
			}

			return nullptr;
		};

		const CStack * next = nullptr;
		while((gameHandler->gameLobby()->state != EServerState::SHUTDOWN) && (next = getNextStack()))
		{
			BattleUnitsChanged removeGhosts;
			for(auto stack : curB.stacks)
			{
				if(stack->ghostPending)
					removeGhosts.changedStacks.emplace_back(stack->unitId(), UnitChanges::EOperation::REMOVE);
			}

			if(!removeGhosts.changedStacks.empty())
				gameHandler->sendAndApply(&removeGhosts);

			// check for bad morale => freeze
			int nextStackMorale = next->moraleVal();
			if(!next->hadMorale && !next->waited() && nextStackMorale < 0)
			{
				auto diceSize = VLC->settings()->getVector(EGameSettings::COMBAT_BAD_MORALE_DICE);
				size_t diceIndex = std::min<size_t>(diceSize.size()-1, -nextStackMorale);

				if(diceSize.size() > 0 && gameHandler->getRandomGenerator().nextInt(1, diceSize[diceIndex]) == 1)
				{
					//unit loses its turn - empty freeze action
					BattleAction ba;
					ba.actionType = EActionType::BAD_MORALE;
					ba.side = next->unitSide();
					ba.stackNumber = next->unitId();

					makeAutomaticAction(next, ba);
					continue;
				}
			}

			if (next->hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE)) //while in berserk
			{
				logGlobal->trace("Handle Berserk effect");
				std::pair<const battle::Unit *, BattleHex> attackInfo = curB.getNearestStack(next);
				if (attackInfo.first != nullptr)
				{
					BattleAction attack;
					attack.actionType = EActionType::WALK_AND_ATTACK;
					attack.side = next->unitSide();
					attack.stackNumber = next->unitId();
					attack.aimToHex(attackInfo.second);
					attack.aimToUnit(attackInfo.first);

					makeAutomaticAction(next, attack);
					logGlobal->trace("Attacked nearest target %s", attackInfo.first->getDescription());
				}
				else
				{
					makeStackDoNothing(next);
					logGlobal->trace("No target found");
				}
				continue;
			}

			const CGHeroInstance * curOwner = gameHandler->battleGetOwnerHero(next);
			const int stackCreatureId = next->unitType()->getId();

			if ((stackCreatureId == CreatureID::ARROW_TOWERS || stackCreatureId == CreatureID::BALLISTA)
				&& (!curOwner || gameHandler->getRandomGenerator().nextInt(99) >= curOwner->valOfBonuses(BonusType::MANUAL_CONTROL, stackCreatureId)))
			{
				BattleAction attack;
				attack.actionType = EActionType::SHOOT;
				attack.side = next->unitSide();
				attack.stackNumber = next->unitId();

				//TODO: select target by priority

				const battle::Unit * target = nullptr;

				for(auto & elem : gameHandler->gameState()->curB->stacks)
				{
					if(elem->unitType()->getId() != CreatureID::CATAPULT
						&& elem->unitOwner() != next->unitOwner()
						&& elem->isValidTarget()
						&& gameHandler->gameState()->curB->battleCanShoot(next, elem->getPosition()))
					{
						target = elem;
						break;
					}
				}

				if(target == nullptr)
				{
					makeStackDoNothing(next);
				}
				else
				{
					attack.aimToUnit(target);
					makeAutomaticAction(next, attack);
				}
				continue;
			}

			if (next->unitType()->getId() == CreatureID::CATAPULT)
			{
				const auto & attackableBattleHexes = curB.getAttackableBattleHexes();

				if (attackableBattleHexes.empty())
				{
					makeStackDoNothing(next);
					continue;
				}

				if (!curOwner || gameHandler->getRandomGenerator().nextInt(99) >= curOwner->valOfBonuses(BonusType::MANUAL_CONTROL, CreatureID::CATAPULT))
				{
					BattleAction attack;
					attack.actionType = EActionType::CATAPULT;
					attack.side = next->unitSide();
					attack.stackNumber = next->unitId();

					makeAutomaticAction(next, attack);
					continue;
				}
			}

			if (next->unitType()->getId() == CreatureID::FIRST_AID_TENT)
			{
				TStacks possibleStacks = gameHandler->battleGetStacksIf([=](const CStack * s)
				{
					return s->unitOwner() == next->unitOwner() && s->canBeHealed();
				});

				if (!possibleStacks.size())
				{
					makeStackDoNothing(next);
					continue;
				}

				if (!curOwner || gameHandler->getRandomGenerator().nextInt(99) >= curOwner->valOfBonuses(BonusType::MANUAL_CONTROL, CreatureID::FIRST_AID_TENT))
				{
					RandomGeneratorUtil::randomShuffle(possibleStacks, gameHandler->getRandomGenerator());
					const CStack * toBeHealed = possibleStacks.front();

					BattleAction heal;
					heal.actionType = EActionType::STACK_HEAL;
					heal.aimToUnit(toBeHealed);
					heal.side = next->unitSide();
					heal.stackNumber = next->unitId();

					makeAutomaticAction(next, heal);
					continue;
				}
			}

			int numberOfAsks = 1;
			bool breakOuter = false;
			do
			{//ask interface and wait for answer
				if (!battleResult.get())
				{
					stackTurnTrigger(next); //various effects

					if(next->fear)
					{
						makeStackDoNothing(next); //end immediately if stack was affected by fear
					}
					else
					{
						logGlobal->trace("Activating %s", next->nodeName());
						auto nextId = next->unitId();
						BattleSetActiveStack sas;
						sas.stack = nextId;
						gameHandler->sendAndApply(&sas);

						auto actionWasMade = [&]() -> bool
						{
							if (battleMadeAction.data)//active stack has made its action
								return true;
							if (battleResult.get())// battle is finished
								return true;
							if (next == nullptr)//active stack was been removed
								return true;
							return !next->alive();//active stack is dead
						};

						boost::unique_lock<boost::mutex> lock(battleMadeAction.mx);
						battleMadeAction.data = false;
						while ((gameHandler->gameLobby()->state != EServerState::SHUTDOWN) && !actionWasMade())
						{
							{
								boost::unique_lock actionLock(battleActionMutex);
								battleMadeAction.cond.wait(lock);
							}
							if (gameHandler->battleGetStackByID(nextId, false) != next)
								next = nullptr; //it may be removed, while we wait
						}
					}
				}

				if (battleResult.get()) //don't touch it, battle could be finished while waiting got action
				{
					breakOuter = true;
					break;
				}
				//we're after action, all results applied
				checkBattleStateChanges(); //check if this action ended the battle

				if(next != nullptr)
				{
					//check for good morale
					nextStackMorale = next->moraleVal();
					if( !battleResult.get()
						&& !next->hadMorale
						&& !next->defending
						&& !next->waited()
						&& !next->fear
						&& next->alive()
						&& nextStackMorale > 0)
					{
						auto diceSize = VLC->settings()->getVector(EGameSettings::COMBAT_GOOD_MORALE_DICE);
						size_t diceIndex = std::min<size_t>(diceSize.size()-1, nextStackMorale);

						if(diceSize.size() > 0 && gameHandler->getRandomGenerator().nextInt(1, diceSize[diceIndex]) == 1)
						{
							BattleTriggerEffect bte;
							bte.stackID = next->unitId();
							bte.effect = vstd::to_underlying(BonusType::MORALE);
							bte.val = 1;
							bte.additionalInfo = 0;
							gameHandler->sendAndApply(&bte); //play animation

							++numberOfAsks; //move this stack once more
						}
					}
				}
				--numberOfAsks;
			} while (numberOfAsks > 0);

			if (breakOuter)
			{
				break;
			}

		}
		firstRound = false;
	}

	if (gameHandler->gameLobby()->state != EServerState::SHUTDOWN)
		endBattle(gameHandler->gameState()->curB->tile, gameHandler->gameState()->curB->battleGetFightingHero(0), gameHandler->gameState()->curB->battleGetFightingHero(1));
}

bool BattleProcessor::makeAutomaticAction(const CStack *stack, BattleAction &ba)
{
	boost::unique_lock lock(battleActionMutex);

	BattleSetActiveStack bsa;
	bsa.stack = stack->unitId();
	bsa.askPlayerInterface = false;
	gameHandler->sendAndApply(&bsa);

	bool ret = makeBattleAction(ba);
	checkBattleStateChanges();
	return ret;
}


void BattleProcessor::attackCasting(bool ranged, BonusType attackMode, const battle::Unit * attacker, const battle::Unit * defender)
{
	if(attacker->hasBonusOfType(attackMode))
	{
		std::set<SpellID> spellsToCast;
		TConstBonusListPtr spells = attacker->getBonuses(Selector::type()(attackMode));
		for(const auto & sf : *spells)
		{
			spellsToCast.insert(SpellID(sf->subtype));
		}
		for(SpellID spellID : spellsToCast)
		{
			bool castMe = false;
			if(!defender->alive())
			{
				logGlobal->debug("attackCasting: all attacked creatures have been killed");
				return;
			}
			int32_t spellLevel = 0;
			TConstBonusListPtr spellsByType = attacker->getBonuses(Selector::typeSubtype(attackMode, spellID));
			for(const auto & sf : *spellsByType)
			{
				int meleeRanged;
				if(sf->additionalInfo.size() < 2)
				{
					// legacy format
					vstd::amax(spellLevel, sf->additionalInfo[0] % 1000);
					meleeRanged = sf->additionalInfo[0] / 1000;
				}
				else
				{
					vstd::amax(spellLevel, sf->additionalInfo[0]);
					meleeRanged = sf->additionalInfo[1];
				}
				if (meleeRanged == 0 || (meleeRanged == 1 && ranged) || (meleeRanged == 2 && !ranged))
					castMe = true;
			}
			int chance = attacker->valOfBonuses((Selector::typeSubtype(attackMode, spellID)));
			vstd::amin(chance, 100);

			const CSpell * spell = SpellID(spellID).toSpell();
			spells::AbilityCaster caster(attacker, spellLevel);

			spells::Target target;
			target.emplace_back(defender);

			spells::BattleCast parameters(gameHandler->gameState()->curB, &caster, spells::Mode::PASSIVE, spell);

			auto m = spell->battleMechanics(&parameters);

			spells::detail::ProblemImpl ignored;

			if(!m->canBeCastAt(target, ignored))
				continue;

			//check if spell should be cast (probability handling)
			if(gameHandler->getRandomGenerator().nextInt(99) >= chance)
				continue;

			//casting
			if(castMe)
			{
				parameters.cast(gameHandler->spellEnv, target);
			}
		}
	}
}

void BattleProcessor::handleAttackBeforeCasting(bool ranged, const CStack * attacker, const CStack * defender)
{
	attackCasting(ranged, BonusType::SPELL_BEFORE_ATTACK, attacker, defender); //no death stare / acid breath needed?
}

void BattleProcessor::handleAfterAttackCasting(bool ranged, const CStack * attacker, const CStack * defender)
{
	if(!attacker->alive() || !defender->alive()) // can be already dead
		return;

	attackCasting(ranged, BonusType::SPELL_AFTER_ATTACK, attacker, defender);

	if(!defender->alive())
	{
		//don't try death stare or acid breath on dead stack (crash!)
		return;
	}

	if(attacker->hasBonusOfType(BonusType::DEATH_STARE))
	{
		// mechanics of Death Stare as in H3:
		// each gorgon have 10% chance to kill (counted separately in H3) -> binomial distribution
		//original formula x = min(x, (gorgons_count + 9)/10);

		double chanceToKill = attacker->valOfBonuses(BonusType::DEATH_STARE, 0) / 100.0f;
		vstd::amin(chanceToKill, 1); //cap at 100%

		std::binomial_distribution<> distribution(attacker->getCount(), chanceToKill);

		int staredCreatures = distribution(gameHandler->getRandomGenerator().getStdGenerator());

		double cap = 1 / std::max(chanceToKill, (double)(0.01));//don't divide by 0
		int maxToKill = static_cast<int>((attacker->getCount() + cap - 1) / cap); //not much more than chance * count
		vstd::amin(staredCreatures, maxToKill);

		staredCreatures += (attacker->level() * attacker->valOfBonuses(BonusType::DEATH_STARE, 1)) / defender->level();
		if(staredCreatures)
		{
			//TODO: death stare was not originally available for multiple-hex attacks, but...
			const CSpell * spell = SpellID(SpellID::DEATH_STARE).toSpell();

			spells::AbilityCaster caster(attacker, 0);

			spells::BattleCast parameters(gameHandler->gameState()->curB, &caster, spells::Mode::PASSIVE, spell);
			spells::Target target;
			target.emplace_back(defender);
			parameters.setEffectValue(staredCreatures);
			parameters.cast(gameHandler->spellEnv, target);
		}
	}

	if(!defender->alive())
		return;

	int64_t acidDamage = 0;
	TConstBonusListPtr acidBreath = attacker->getBonuses(Selector::type()(BonusType::ACID_BREATH));
	for(const auto & b : *acidBreath)
	{
		if(b->additionalInfo[0] > gameHandler->getRandomGenerator().nextInt(99))
			acidDamage += b->val;
	}

	if(acidDamage > 0)
	{
		const CSpell * spell = SpellID(SpellID::ACID_BREATH_DAMAGE).toSpell();

		spells::AbilityCaster caster(attacker, 0);

		spells::BattleCast parameters(gameHandler->gameState()->curB, &caster, spells::Mode::PASSIVE, spell);
		spells::Target target;
		target.emplace_back(defender);

		parameters.setEffectValue(acidDamage * attacker->getCount());
		parameters.cast(gameHandler->spellEnv, target);
	}


	if(!defender->alive())
		return;

	if(attacker->hasBonusOfType(BonusType::TRANSMUTATION) && defender->isLiving()) //transmutation mechanics, similar to WoG werewolf ability
	{
		double chanceToTrigger = attacker->valOfBonuses(BonusType::TRANSMUTATION) / 100.0f;
		vstd::amin(chanceToTrigger, 1); //cap at 100%

		if(gameHandler->getRandomGenerator().getDoubleRange(0, 1)() > chanceToTrigger)
			return;

		int bonusAdditionalInfo = attacker->getBonus(Selector::type()(BonusType::TRANSMUTATION))->additionalInfo[0];

		if(defender->unitType()->getId() == bonusAdditionalInfo ||
			(bonusAdditionalInfo == CAddInfo::NONE && defender->unitType()->getId() == attacker->unitType()->getId()))
			return;

		battle::UnitInfo resurrectInfo;
		resurrectInfo.id = gameHandler->gameState()->curB->battleNextUnitId();
		resurrectInfo.summoned = false;
		resurrectInfo.position = defender->getPosition();
		resurrectInfo.side = defender->unitSide();

		if(bonusAdditionalInfo != CAddInfo::NONE)
			resurrectInfo.type = CreatureID(bonusAdditionalInfo);
		else
			resurrectInfo.type = attacker->creatureId();

		if(attacker->hasBonusOfType((BonusType::TRANSMUTATION), 0))
			resurrectInfo.count = std::max((defender->getCount() * defender->getMaxHealth()) / resurrectInfo.type.toCreature()->getMaxHealth(), 1u);
		else if (attacker->hasBonusOfType((BonusType::TRANSMUTATION), 1))
			resurrectInfo.count = defender->getCount();
		else
			return; //wrong subtype

		BattleUnitsChanged addUnits;
		addUnits.changedStacks.emplace_back(resurrectInfo.id, UnitChanges::EOperation::ADD);
		resurrectInfo.save(addUnits.changedStacks.back().data);

		BattleUnitsChanged removeUnits;
		removeUnits.changedStacks.emplace_back(defender->unitId(), UnitChanges::EOperation::REMOVE);
		gameHandler->sendAndApply(&removeUnits);
		gameHandler->sendAndApply(&addUnits);
	}

	if(attacker->hasBonusOfType(BonusType::DESTRUCTION, 0) || attacker->hasBonusOfType(BonusType::DESTRUCTION, 1))
	{
		double chanceToTrigger = 0;
		int amountToDie = 0;

		if(attacker->hasBonusOfType(BonusType::DESTRUCTION, 0)) //killing by percentage
		{
			chanceToTrigger = attacker->valOfBonuses(BonusType::DESTRUCTION, 0) / 100.0f;
			int percentageToDie = attacker->getBonus(Selector::type()(BonusType::DESTRUCTION).And(Selector::subtype()(0)))->additionalInfo[0];
			amountToDie = static_cast<int>(defender->getCount() * percentageToDie * 0.01f);
		}
		else if(attacker->hasBonusOfType(BonusType::DESTRUCTION, 1)) //killing by count
		{
			chanceToTrigger = attacker->valOfBonuses(BonusType::DESTRUCTION, 1) / 100.0f;
			amountToDie = attacker->getBonus(Selector::type()(BonusType::DESTRUCTION).And(Selector::subtype()(1)))->additionalInfo[0];
		}

		vstd::amin(chanceToTrigger, 1); //cap trigger chance at 100%

		if(gameHandler->getRandomGenerator().getDoubleRange(0, 1)() > chanceToTrigger)
			return;

		BattleStackAttacked bsa;
		bsa.attackerID = -1;
		bsa.stackAttacked = defender->unitId();
		bsa.damageAmount = amountToDie * defender->getMaxHealth();
		bsa.flags = BattleStackAttacked::SPELL_EFFECT;
		bsa.spellID = SpellID::SLAYER;
		defender->prepareAttacked(bsa, gameHandler->getRandomGenerator());

		StacksInjured si;
		si.stacks.push_back(bsa);

		gameHandler->sendAndApply(&si);
		sendGenericKilledLog(defender, bsa.killedAmount, false);
	}
}

void BattleProcessor::makeAttack(const CStack * attacker, const CStack * defender, int distance, BattleHex targetHex, bool first, bool ranged, bool counter)
{
	if(first && !counter)
		handleAttackBeforeCasting(ranged, attacker, defender);

	FireShieldInfo fireShield;
	BattleAttack bat;
	BattleLogMessage blm;
	bat.stackAttacking = attacker->unitId();
	bat.tile = targetHex;

	std::shared_ptr<battle::CUnitState> attackerState = attacker->acquireState();

	if(ranged)
		bat.flags |= BattleAttack::SHOT;
	if(counter)
		bat.flags |= BattleAttack::COUNTER;

	const int attackerLuck = attacker->luckVal();

	if(attackerLuck > 0)
	{
		auto diceSize = VLC->settings()->getVector(EGameSettings::COMBAT_GOOD_LUCK_DICE);
		size_t diceIndex = std::min<size_t>(diceSize.size() - 1, attackerLuck);

		if(diceSize.size() > 0 && gameHandler->getRandomGenerator().nextInt(1, diceSize[diceIndex]) == 1)
			bat.flags |= BattleAttack::LUCKY;
	}

	if(attackerLuck < 0)
	{
		auto diceSize = VLC->settings()->getVector(EGameSettings::COMBAT_BAD_LUCK_DICE);
		size_t diceIndex = std::min<size_t>(diceSize.size() - 1, -attackerLuck);

		if(diceSize.size() > 0 && gameHandler->getRandomGenerator().nextInt(1, diceSize[diceIndex]) == 1)
			bat.flags |= BattleAttack::UNLUCKY;
	}

	if (gameHandler->getRandomGenerator().nextInt(99) < attacker->valOfBonuses(BonusType::DOUBLE_DAMAGE_CHANCE))
	{
		bat.flags |= BattleAttack::DEATH_BLOW;
	}

	const auto * owner = gameHandler->gameState()->curB->getHero(attacker->unitOwner());
	if(owner)
	{
		int chance = owner->valOfBonuses(BonusType::BONUS_DAMAGE_CHANCE, attacker->creatureIndex());
		if (chance > gameHandler->getRandomGenerator().nextInt(99))
			bat.flags |= BattleAttack::BALLISTA_DOUBLE_DMG;
	}

	int64_t drainedLife = 0;

	// only primary target
	if(defender->alive())
		drainedLife += applyBattleEffects(bat, attackerState, fireShield, defender, distance, false);

	//multiple-hex normal attack
	std::set<const CStack*> attackedCreatures = gameHandler->gameState()->curB->getAttackedCreatures(attacker, targetHex, bat.shot()); //creatures other than primary target
	for(const CStack * stack : attackedCreatures)
	{
		if(stack != defender && stack->alive()) //do not hit same stack twice
			drainedLife += applyBattleEffects(bat, attackerState, fireShield, stack, distance, true);
	}

	std::shared_ptr<const Bonus> bonus = attacker->getBonusLocalFirst(Selector::type()(BonusType::SPELL_LIKE_ATTACK));
	if(bonus && ranged) //TODO: make it work in melee?
	{
		//this is need for displaying hit animation
		bat.flags |= BattleAttack::SPELL_LIKE;
		bat.spellID = SpellID(bonus->subtype);

		//TODO: should spell override creature`s projectile?

		auto spell = bat.spellID.toSpell();

		battle::Target target;
		target.emplace_back(defender, targetHex);

		spells::BattleCast event(gameHandler->gameState()->curB, attacker, spells::Mode::SPELL_LIKE_ATTACK, spell);
		event.setSpellLevel(bonus->val);

		auto attackedCreatures = spell->battleMechanics(&event)->getAffectedStacks(target);

		//TODO: get exact attacked hex for defender

		for(const CStack * stack : attackedCreatures)
		{
			if(stack != defender && stack->alive()) //do not hit same stack twice
			{
				drainedLife += applyBattleEffects(bat, attackerState, fireShield, stack, distance, true);
			}
		}

		//now add effect info for all attacked stacks
		for (BattleStackAttacked & bsa : bat.bsa)
		{
			if (bsa.attackerID == attacker->unitId()) //this is our attack and not f.e. fire shield
			{
				//this is need for displaying affect animation
				bsa.flags |= BattleStackAttacked::SPELL_EFFECT;
				bsa.spellID = SpellID(bonus->subtype);
			}
		}
	}

	attackerState->afterAttack(ranged, counter);

	{
		UnitChanges info(attackerState->unitId(), UnitChanges::EOperation::RESET_STATE);
		attackerState->save(info.data);
		bat.attackerChanges.changedStacks.push_back(info);
	}

	if (drainedLife > 0)
		bat.flags |= BattleAttack::LIFE_DRAIN;

	gameHandler->sendAndApply(&bat);

	{
		const bool multipleTargets = bat.bsa.size() > 1;

		int64_t totalDamage = 0;
		int32_t totalKills = 0;

		for(const BattleStackAttacked & bsa : bat.bsa)
		{
			totalDamage += bsa.damageAmount;
			totalKills += bsa.killedAmount;
		}

		{
			MetaString text;
			attacker->addText(text, EMetaText::GENERAL_TXT, 376);
			attacker->addNameReplacement(text);
			text.replaceNumber(totalDamage);
			blm.lines.push_back(text);
		}

		addGenericKilledLog(blm, defender, totalKills, multipleTargets);
	}

	// drain life effect (as well as log entry) must be applied after the attack
	if(drainedLife > 0)
	{
		MetaString text;
		attackerState->addText(text, EMetaText::GENERAL_TXT, 361);
		attackerState->addNameReplacement(text, false);
		text.replaceNumber(drainedLife);
		defender->addNameReplacement(text, true);
		blm.lines.push_back(std::move(text));
	}

	if(!fireShield.empty())
	{
		//todo: this should be "virtual" spell instead, we only need fire spell school bonus here
		const CSpell * fireShieldSpell = SpellID(SpellID::FIRE_SHIELD).toSpell();
		int64_t totalDamage = 0;

		for(const auto & item : fireShield)
		{
			const CStack * actor = item.first;
			int64_t rawDamage = item.second;

			const CGHeroInstance * actorOwner = gameHandler->gameState()->curB->getHero(actor->unitOwner());

			if(actorOwner)
			{
				rawDamage = fireShieldSpell->adjustRawDamage(actorOwner, attacker, rawDamage);
			}
			else
			{
				rawDamage = fireShieldSpell->adjustRawDamage(actor, attacker, rawDamage);
			}

			totalDamage+=rawDamage;
			//FIXME: add custom effect on actor
		}

		if (totalDamage > 0)
		{
			BattleStackAttacked bsa;

			bsa.flags |= BattleStackAttacked::FIRE_SHIELD;
			bsa.stackAttacked = attacker->unitId(); //invert
			bsa.attackerID = defender->unitId();
			bsa.damageAmount = totalDamage;
			attacker->prepareAttacked(bsa, gameHandler->getRandomGenerator());

			StacksInjured pack;
			pack.stacks.push_back(bsa);
			gameHandler->sendAndApply(&pack);

			// TODO: this is already implemented in Damage::describeEffect()
			{
				MetaString text;
				text.appendLocalString(EMetaText::GENERAL_TXT, 376);
				text.replaceLocalString(EMetaText::SPELL_NAME, SpellID::FIRE_SHIELD);
				text.replaceNumber(totalDamage);
				blm.lines.push_back(std::move(text));
			}
			addGenericKilledLog(blm, attacker, bsa.killedAmount, false);
		}
	}

	gameHandler->sendAndApply(&blm);

	handleAfterAttackCasting(ranged, attacker, defender);
}

int64_t BattleProcessor::applyBattleEffects(BattleAttack & bat, std::shared_ptr<battle::CUnitState> attackerState, FireShieldInfo & fireShield, const CStack * def, int distance, bool secondary)
{
	BattleStackAttacked bsa;
	if(secondary)
		bsa.flags |= BattleStackAttacked::SECONDARY; //all other targets do not suffer from spells & spell-like abilities

	bsa.attackerID = attackerState->unitId();
	bsa.stackAttacked = def->unitId();
	{
		BattleAttackInfo bai(attackerState.get(), def, distance, bat.shot());

		bai.deathBlow = bat.deathBlow();
		bai.doubleDamage = bat.ballistaDoubleDmg();
		bai.luckyStrike  = bat.lucky();
		bai.unluckyStrike  = bat.unlucky();

		auto range = gameHandler->gameState()->curB->calculateDmgRange(bai);
		bsa.damageAmount = gameHandler->gameState()->curB->getActualDamage(range.damage, attackerState->getCount(), gameHandler->getRandomGenerator());
		CStack::prepareAttacked(bsa, gameHandler->getRandomGenerator(), bai.defender->acquireState()); //calculate casualties
	}

	int64_t drainedLife = 0;

	//life drain handling
	if(attackerState->hasBonusOfType(BonusType::LIFE_DRAIN) && def->isLiving())
	{
		int64_t toHeal = bsa.damageAmount * attackerState->valOfBonuses(BonusType::LIFE_DRAIN) / 100;
		attackerState->heal(toHeal, EHealLevel::RESURRECT, EHealPower::PERMANENT);
		drainedLife += toHeal;
	}

	//soul steal handling
	if(attackerState->hasBonusOfType(BonusType::SOUL_STEAL) && def->isLiving())
	{
		//we can have two bonuses - one with subtype 0 and another with subtype 1
		//try to use permanent first, use only one of two
		for(si32 subtype = 1; subtype >= 0; subtype--)
		{
			if(attackerState->hasBonusOfType(BonusType::SOUL_STEAL, subtype))
			{
				int64_t toHeal = bsa.killedAmount * attackerState->valOfBonuses(BonusType::SOUL_STEAL, subtype) * attackerState->getMaxHealth();
				attackerState->heal(toHeal, EHealLevel::OVERHEAL, ((subtype == 0) ? EHealPower::ONE_BATTLE : EHealPower::PERMANENT));
				drainedLife += toHeal;
				break;
			}
		}
	}
	bat.bsa.push_back(bsa); //add this stack to the list of victims after drain life has been calculated

	//fire shield handling
	if(!bat.shot() &&
		!def->isClone() &&
		def->hasBonusOfType(BonusType::FIRE_SHIELD) &&
		!attackerState->hasBonusOfType(BonusType::FIRE_IMMUNITY) &&
		CStack::isMeleeAttackPossible(attackerState.get(), def) // attacked needs to be adjacent to defender for fire shield to trigger (e.g. Dragon Breath attack)
			)
	{
		//TODO: use damage with bonus but without penalties
		auto fireShieldDamage = (std::min<int64_t>(def->getAvailableHealth(), bsa.damageAmount) * def->valOfBonuses(BonusType::FIRE_SHIELD)) / 100;
		fireShield.push_back(std::make_pair(def, fireShieldDamage));
	}

	return drainedLife;
}

void BattleProcessor::sendGenericKilledLog(const CStack * defender, int32_t killed, bool multiple)
{
	if(killed > 0)
	{
		BattleLogMessage blm;
		addGenericKilledLog(blm, defender, killed, multiple);
		gameHandler->sendAndApply(&blm);
	}
}

void BattleProcessor::addGenericKilledLog(BattleLogMessage & blm, const CStack * defender, int32_t killed, bool multiple)
{
	if(killed > 0)
	{
		const int32_t txtIndex = (killed > 1) ? 379 : 378;
		std::string formatString = VLC->generaltexth->allTexts[txtIndex];

		// these default h3 texts have unnecessary new lines, so get rid of them before displaying (and trim just in case, trimming newlines does not works for some reason)
		formatString.erase(std::remove(formatString.begin(), formatString.end(), '\n'), formatString.end());
		formatString.erase(std::remove(formatString.begin(), formatString.end(), '\r'), formatString.end());
		boost::algorithm::trim(formatString);

		boost::format txt(formatString);
		if(killed > 1)
		{
			txt % killed % (multiple ? VLC->generaltexth->allTexts[43] : defender->unitType()->getNamePluralTranslated()); // creatures perish
		}
		else //killed == 1
		{
			txt % (multiple ? VLC->generaltexth->allTexts[42] : defender->unitType()->getNameSingularTranslated()); // creature perishes
		}
		MetaString line;
		line.appendRawString(txt.str());
		blm.lines.push_back(std::move(line));
	}
}

void BattleProcessor::endBattle(int3 tile, const CGHeroInstance * heroAttacker, const CGHeroInstance * heroDefender)
{
	LOG_TRACE(logGlobal);

	//Fill BattleResult structure with exp info
	giveExp(*battleResult.data);

	if (battleResult.get()->result == BattleResult::NORMAL) // give 500 exp for defeating hero, unless he escaped
	{
		if(heroAttacker)
			battleResult.data->exp[1] += 500;
		if(heroDefender)
			battleResult.data->exp[0] += 500;
	}

	if(heroAttacker)
		battleResult.data->exp[0] = heroAttacker->calculateXp(battleResult.data->exp[0]);//scholar skill
	if(heroDefender)
		battleResult.data->exp[1] = heroDefender->calculateXp(battleResult.data->exp[1]);

	auto battleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(gameHandler->gameState()->curB->sides[0].color));
	if (!battleQuery)
	{
		logGlobal->error("Cannot find battle query!");
		gameHandler->complain("Player " + boost::lexical_cast<std::string>(gameHandler->gameState()->curB->sides[0].color) + " has no battle query at the top!");
		return;
	}

	battleQuery->result = std::make_optional(*battleResult.data);

	//Check how many battle gameHandler->queries were created (number of players blocked by battle)
	const int queriedPlayers = battleQuery ? (int)boost::count(gameHandler->queries->allQueries(), battleQuery) : 0;
	finishingBattle = std::make_unique<FinishingBattleHelper>(battleQuery, queriedPlayers);

	// in battles against neutrals, 1st player can ask to replay battle manually
	if (!gameHandler->gameState()->curB->sides[1].color.isValidPlayer())
	{
		auto battleDialogQuery = std::make_shared<CBattleDialogQuery>(gameHandler, gameHandler->gameState()->curB);
		battleResult.data->queryID = battleDialogQuery->queryID;
		gameHandler->queries->addQuery(battleDialogQuery);
	}
	else
		battleResult.data->queryID = -1;

	//set same battle result for all gameHandler->queries
	for(auto q : gameHandler->queries->allQueries())
	{
		auto otherBattleQuery = std::dynamic_pointer_cast<CBattleQuery>(q);
		if(otherBattleQuery)
			otherBattleQuery->result = battleQuery->result;
	}

	gameHandler->sendAndApply(battleResult.data); //after this point casualties objects are destroyed

	if (battleResult.data->queryID == -1)
		endBattleConfirm(gameHandler->gameState()->curB);
}

void BattleProcessor::endBattleConfirm(const BattleInfo * battleInfo)
{
	auto battleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(battleInfo->sides.at(0).color));
	if(!battleQuery)
	{
		logGlobal->trace("No battle query, battle end was confirmed by another player");
		return;
	}

	const BattleResult::EResult result = battleResult.get()->result;

	CasualtiesAfterBattle cab1(battleInfo->sides.at(0), battleInfo), cab2(battleInfo->sides.at(1), battleInfo); //calculate casualties before deleting battle
	ChangeSpells cs; //for Eagle Eye

	if(!finishingBattle->isDraw() && finishingBattle->winnerHero)
	{
		if (int eagleEyeLevel = finishingBattle->winnerHero->valOfBonuses(BonusType::LEARN_BATTLE_SPELL_LEVEL_LIMIT, -1))
		{
			double eagleEyeChance = finishingBattle->winnerHero->valOfBonuses(BonusType::LEARN_BATTLE_SPELL_CHANCE, 0);
			for(auto & spellId : battleInfo->sides.at(!battleResult.data->winner).usedSpellsHistory)
			{
				auto spell = spellId.toSpell(VLC->spells());
				if(spell && spell->getLevel() <= eagleEyeLevel && !finishingBattle->winnerHero->spellbookContainsSpell(spell->getId()) && gameHandler->getRandomGenerator().nextInt(99) < eagleEyeChance)
					cs.spells.insert(spell->getId());
			}
		}
	}
	std::vector<const CArtifactInstance *> arts; //display them in window

	if(result == BattleResult::NORMAL && !finishingBattle->isDraw() && finishingBattle->winnerHero)
	{
		auto sendMoveArtifact = [&](const CArtifactInstance *art, MoveArtifact *ma)
		{
			const auto slot = ArtifactUtils::getArtAnyPosition(finishingBattle->winnerHero, art->getTypeId());
			if(slot != ArtifactPosition::PRE_FIRST)
			{
				arts.push_back(art);
				ma->dst = ArtifactLocation(finishingBattle->winnerHero, slot);
				if(ArtifactUtils::isSlotBackpack(slot))
					ma->askAssemble = false;
				gameHandler->sendAndApply(ma);
			}
		};

		if (finishingBattle->loserHero)
		{
			//TODO: wrap it into a function, somehow (std::variant -_-)
			auto artifactsWorn = finishingBattle->loserHero->artifactsWorn;
			for (auto artSlot : artifactsWorn)
			{
				MoveArtifact ma;
				ma.src = ArtifactLocation(finishingBattle->loserHero, artSlot.first);
				const CArtifactInstance * art =  ma.src.getArt();
				if (art && !art->artType->isBig() &&
					art->artType->getId() != ArtifactID::SPELLBOOK)
						// don't move war machines or locked arts (spellbook)
				{
					sendMoveArtifact(art, &ma);
				}
			}
			for(int slotNumber = finishingBattle->loserHero->artifactsInBackpack.size() - 1; slotNumber >= 0; slotNumber--)
			{
				//we assume that no big artifacts can be found
				MoveArtifact ma;
				ma.src = ArtifactLocation(finishingBattle->loserHero,
					ArtifactPosition(GameConstants::BACKPACK_START + slotNumber)); //backpack automatically shifts arts to beginning
				const CArtifactInstance * art =  ma.src.getArt();
				if (art->artType->getId() != ArtifactID::GRAIL) //grail may not be won
				{
					sendMoveArtifact(art, &ma);
				}
			}
			if (finishingBattle->loserHero->commander) //TODO: what if commanders belong to no hero?
			{
				artifactsWorn = finishingBattle->loserHero->commander->artifactsWorn;
				for (auto artSlot : artifactsWorn)
				{
					MoveArtifact ma;
					ma.src = ArtifactLocation(finishingBattle->loserHero->commander.get(), artSlot.first);
					const CArtifactInstance * art =  ma.src.getArt();
					if (art && !art->artType->isBig())
					{
						sendMoveArtifact(art, &ma);
					}
				}
			}
		}
		for (auto armySlot : battleInfo->sides.at(!battleResult.data->winner).armyObject->stacks)
		{
			auto artifactsWorn = armySlot.second->artifactsWorn;
			for (auto artSlot : artifactsWorn)
			{
				MoveArtifact ma;
				ma.src = ArtifactLocation(armySlot.second, artSlot.first);
				const CArtifactInstance * art =  ma.src.getArt();
				if (art && !art->artType->isBig())
				{
					sendMoveArtifact(art, &ma);
				}
			}
		}
	}

	if (arts.size()) //display loot
	{
		InfoWindow iw;
		iw.player = finishingBattle->winnerHero->tempOwner;

		iw.text.appendLocalString (EMetaText::GENERAL_TXT, 30); //You have captured enemy artifact

		for (auto art : arts) //TODO; separate function to display loot for various ojects?
		{
			iw.components.emplace_back(
				Component::EComponentType::ARTIFACT, art->artType->getId(),
				art->artType->getId() == ArtifactID::SPELL_SCROLL? art->getScrollSpellID() : 0, 0);
			if (iw.components.size() >= 14)
			{
				gameHandler->sendAndApply(&iw);
				iw.components.clear();
			}
		}
		if (iw.components.size())
		{
			gameHandler->sendAndApply(&iw);
		}
	}
	//Eagle Eye secondary skill handling
	if (!cs.spells.empty())
	{
		cs.learn = 1;
		cs.hid = finishingBattle->winnerHero->id;

		InfoWindow iw;
		iw.player = finishingBattle->winnerHero->tempOwner;
		iw.text.appendLocalString(EMetaText::GENERAL_TXT, 221); //Through eagle-eyed observation, %s is able to learn %s
		iw.text.replaceRawString(finishingBattle->winnerHero->getNameTranslated());

		std::ostringstream names;
		for (int i = 0; i < cs.spells.size(); i++)
		{
			names << "%s";
			if (i < cs.spells.size() - 2)
				names << ", ";
			else if (i < cs.spells.size() - 1)
				names << "%s";
		}
		names << ".";

		iw.text.replaceRawString(names.str());

		auto it = cs.spells.begin();
		for (int i = 0; i < cs.spells.size(); i++, it++)
		{
			iw.text.replaceLocalString(EMetaText::SPELL_NAME, it->toEnum());
			if (i == cs.spells.size() - 2) //we just added pre-last name
				iw.text.replaceLocalString(EMetaText::GENERAL_TXT, 141); // " and "
			iw.components.emplace_back(Component::EComponentType::SPELL, *it, 0, 0);
		}
		gameHandler->sendAndApply(&iw);
		gameHandler->sendAndApply(&cs);
	}
	cab1.updateArmy(gameHandler);
	cab2.updateArmy(gameHandler); //take casualties after battle is deleted

	if(finishingBattle->loserHero) //remove beaten hero
	{
		RemoveObject ro(finishingBattle->loserHero->id);
		gameHandler->sendAndApply(&ro);
	}
	if(finishingBattle->isDraw() && finishingBattle->winnerHero) //for draw case both heroes should be removed
	{
		RemoveObject ro(finishingBattle->winnerHero->id);
		gameHandler->sendAndApply(&ro);
	}

	if(battleResult.data->winner == BattleSide::DEFENDER
	   && finishingBattle->winnerHero
	   && finishingBattle->winnerHero->visitedTown
	   && !finishingBattle->winnerHero->inTownGarrison
	   && finishingBattle->winnerHero->visitedTown->garrisonHero == finishingBattle->winnerHero)
	{
		gameHandler->swapGarrisonOnSiege(finishingBattle->winnerHero->visitedTown->id); //return defending visitor from garrison to its rightful place
	}
	//give exp
	if(!finishingBattle->isDraw() && battleResult.data->exp[finishingBattle->winnerSide] && finishingBattle->winnerHero)
		gameHandler->changePrimSkill(finishingBattle->winnerHero, PrimarySkill::EXPERIENCE, battleResult.data->exp[finishingBattle->winnerSide]);

	BattleResultAccepted raccepted;
	raccepted.heroResult[0].army = const_cast<CArmedInstance*>(battleInfo->sides.at(0).armyObject);
	raccepted.heroResult[1].army = const_cast<CArmedInstance*>(battleInfo->sides.at(1).armyObject);
	raccepted.heroResult[0].hero = const_cast<CGHeroInstance*>(battleInfo->sides.at(0).hero);
	raccepted.heroResult[1].hero = const_cast<CGHeroInstance*>(battleInfo->sides.at(1).hero);
	raccepted.heroResult[0].exp = battleResult.data->exp[0];
	raccepted.heroResult[1].exp = battleResult.data->exp[1];
	raccepted.winnerSide = finishingBattle->winnerSide;
	gameHandler->sendAndApply(&raccepted);

	gameHandler->queries->popIfTop(battleQuery);
	//--> continuation (battleAfterLevelUp) occurs after level-up gameHandler->queries are handled or on removing query
}

void BattleProcessor::battleAfterLevelUp(const BattleResult &result)
{
	LOG_TRACE(logGlobal);

	if(!finishingBattle)
		return;

	finishingBattle->remainingBattleQueriesCount--;
	logGlobal->trace("Decremented gameHandler->queries count to %d", finishingBattle->remainingBattleQueriesCount);

	if (finishingBattle->remainingBattleQueriesCount > 0)
		//Battle results will be handled when all battle gameHandler->queries are closed
		return;

	//TODO consider if we really want it to work like above. ATM each player as unblocked as soon as possible
	// but the battle consequences are applied after final player is unblocked. Hard to abuse...
	// Still, it looks like a hole.

	// Necromancy if applicable.
	const CStackBasicDescriptor raisedStack = finishingBattle->winnerHero ? finishingBattle->winnerHero->calculateNecromancy(*battleResult.data) : CStackBasicDescriptor();
	// Give raised units to winner and show dialog, if any were raised,
	// units will be given after casualties are taken
	const SlotID necroSlot = raisedStack.type ? finishingBattle->winnerHero->getSlotFor(raisedStack.type) : SlotID();

	if (necroSlot != SlotID())
	{
		finishingBattle->winnerHero->showNecromancyDialog(raisedStack, gameHandler->getRandomGenerator());
		gameHandler->addToSlot(StackLocation(finishingBattle->winnerHero, necroSlot), raisedStack.type, raisedStack.count);
	}

	BattleResultsApplied resultsApplied;
	resultsApplied.player1 = finishingBattle->victor;
	resultsApplied.player2 = finishingBattle->loser;
	gameHandler->sendAndApply(&resultsApplied);

	gameHandler->setBattle(nullptr);

	if (visitObjectAfterVictory && result.winner==0 && !finishingBattle->winnerHero->stacks.empty())
	{
		logGlobal->trace("post-victory visit");
		gameHandler->visitObjectOnTile(*gameHandler->getTile(finishingBattle->winnerHero->visitablePos()), finishingBattle->winnerHero);
	}
	visitObjectAfterVictory = false;

	//handle victory/loss of engaged players
	std::set<PlayerColor> playerColors = {finishingBattle->loser, finishingBattle->victor};
	gameHandler->checkVictoryLossConditions(playerColors);

	if (result.result == BattleResult::SURRENDER)
		gameHandler->heroPool->onHeroSurrendered(finishingBattle->loser, finishingBattle->loserHero);

	if (result.result == BattleResult::ESCAPE)
		gameHandler->heroPool->onHeroEscaped(finishingBattle->loser, finishingBattle->loserHero);

	if (result.winner != 2 && finishingBattle->winnerHero && finishingBattle->winnerHero->stacks.empty()
		&& (!finishingBattle->winnerHero->commander || !finishingBattle->winnerHero->commander->alive))
	{
		RemoveObject ro(finishingBattle->winnerHero->id);
		gameHandler->sendAndApply(&ro);

		if (VLC->settings()->getBoolean(EGameSettings::HEROES_RETREAT_ON_WIN_WITHOUT_TROOPS))
			gameHandler->heroPool->onHeroEscaped(finishingBattle->victor, finishingBattle->winnerHero);
	}

	finishingBattle.reset();
}

int BattleProcessor::moveStack(int stack, BattleHex dest)
{
	int ret = 0;

	const CStack *curStack = gameHandler->battleGetStackByID(stack),
		*stackAtEnd = gameHandler->gameState()->curB->battleGetStackByPos(dest);

	assert(curStack);
	assert(dest < GameConstants::BFIELD_SIZE);

	if (gameHandler->gameState()->curB->tacticDistance)
	{
		assert(gameHandler->gameState()->curB->isInTacticRange(dest));
	}

	auto start = curStack->getPosition();
	if (start == dest)
		return 0;

	//initing necessary tables
	auto accessibility = gameHandler->getAccesibility(curStack);
	std::set<BattleHex> passed;
	//Ignore obstacles on starting position
	passed.insert(curStack->getPosition());
	if(curStack->doubleWide())
		passed.insert(curStack->occupiedHex());

	//shifting destination (if we have double wide stack and we can occupy dest but not be exactly there)
	if(!stackAtEnd && curStack->doubleWide() && !accessibility.accessible(dest, curStack))
	{
		BattleHex shifted = dest.cloneInDirection(curStack->destShiftDir(), false);

		if(accessibility.accessible(shifted, curStack))
			dest = shifted;
	}

	if((stackAtEnd && stackAtEnd!=curStack && stackAtEnd->alive()) || !accessibility.accessible(dest, curStack))
	{
		gameHandler->complain("Given destination is not accessible!");
		return 0;
	}

	bool canUseGate = false;
	auto dbState = gameHandler->gameState()->curB->si.gateState;
	if(gameHandler->battleGetSiegeLevel() > 0 && curStack->unitSide() == BattleSide::DEFENDER &&
		dbState != EGateState::DESTROYED &&
		dbState != EGateState::BLOCKED)
	{
		canUseGate = true;
	}

	std::pair< std::vector<BattleHex>, int > path = gameHandler->gameState()->curB->getPath(start, dest, curStack);

	ret = path.second;

	int creSpeed = curStack->speed(0, true);

	if (gameHandler->gameState()->curB->tacticDistance > 0 && creSpeed > 0)
		creSpeed = GameConstants::BFIELD_SIZE;

	bool hasWideMoat = vstd::contains_if(gameHandler->battleGetAllObstaclesOnPos(BattleHex(ESiegeHex::GATE_BRIDGE), false), [](const std::shared_ptr<const CObstacleInstance> & obst)
	{
		return obst->obstacleType == CObstacleInstance::MOAT;
	});

	auto isGateDrawbridgeHex = [&](BattleHex hex) -> bool
	{
		if (hasWideMoat && hex == ESiegeHex::GATE_BRIDGE)
			return true;
		if (hex == ESiegeHex::GATE_OUTER)
			return true;
		if (hex == ESiegeHex::GATE_INNER)
			return true;

		return false;
	};

	auto occupyGateDrawbridgeHex = [&](BattleHex hex) -> bool
	{
		if (isGateDrawbridgeHex(hex))
			return true;

		if (curStack->doubleWide())
		{
			BattleHex otherHex = curStack->occupiedHex(hex);
			if (otherHex.isValid() && isGateDrawbridgeHex(otherHex))
				return true;
		}

		return false;
	};

	if (curStack->hasBonusOfType(BonusType::FLYING))
	{
		if (path.second <= creSpeed && path.first.size() > 0)
		{
			if (canUseGate && dbState != EGateState::OPENED &&
				occupyGateDrawbridgeHex(dest))
			{
				BattleUpdateGateState db;
				db.state = EGateState::OPENED;
				gameHandler->sendAndApply(&db);
			}

			//inform clients about move
			BattleStackMoved sm;
			sm.stack = curStack->unitId();
			std::vector<BattleHex> tiles;
			tiles.push_back(path.first[0]);
			sm.tilesToMove = tiles;
			sm.distance = path.second;
			sm.teleporting = false;
			gameHandler->sendAndApply(&sm);
		}
	}
	else //for non-flying creatures
	{
		std::vector<BattleHex> tiles;
		const int tilesToMove = std::max((int)(path.first.size() - creSpeed), 0);
		int v = (int)path.first.size()-1;
		path.first.push_back(start);

		// check if gate need to be open or closed at some point
		BattleHex openGateAtHex, gateMayCloseAtHex;
		if (canUseGate)
		{
			for (int i = (int)path.first.size()-1; i >= 0; i--)
			{
				auto needOpenGates = [&](BattleHex hex) -> bool
				{
					if (hasWideMoat && hex == ESiegeHex::GATE_BRIDGE)
						return true;
					if (hex == ESiegeHex::GATE_BRIDGE && i-1 >= 0 && path.first[i-1] == ESiegeHex::GATE_OUTER)
						return true;
					else if (hex == ESiegeHex::GATE_OUTER || hex == ESiegeHex::GATE_INNER)
						return true;

					return false;
				};

				auto hex = path.first[i];
				if (!openGateAtHex.isValid() && dbState != EGateState::OPENED)
				{
					if (needOpenGates(hex))
						openGateAtHex = path.first[i+1];

					//TODO we need find batter way to handle double-wide stacks
					//currently if only second occupied stack part is standing on gate / bridge hex then stack will start to wait for bridge to lower before it's needed. Though this is just a visual bug.
					if (curStack->doubleWide())
					{
						BattleHex otherHex = curStack->occupiedHex(hex);
						if (otherHex.isValid() && needOpenGates(otherHex))
							openGateAtHex = path.first[i+2];
					}

					//gate may be opened and then closed during stack movement, but not other way around
					if (openGateAtHex.isValid())
						dbState = EGateState::OPENED;
				}

				if (!gateMayCloseAtHex.isValid() && dbState != EGateState::CLOSED)
				{
					if (hex == ESiegeHex::GATE_INNER && i-1 >= 0 && path.first[i-1] != ESiegeHex::GATE_OUTER)
					{
						gateMayCloseAtHex = path.first[i-1];
					}
					if (hasWideMoat)
					{
						if (hex == ESiegeHex::GATE_BRIDGE && i-1 >= 0 && path.first[i-1] != ESiegeHex::GATE_OUTER)
						{
							gateMayCloseAtHex = path.first[i-1];
						}
						else if (hex == ESiegeHex::GATE_OUTER && i-1 >= 0 &&
							path.first[i-1] != ESiegeHex::GATE_INNER &&
							path.first[i-1] != ESiegeHex::GATE_BRIDGE)
						{
							gateMayCloseAtHex = path.first[i-1];
						}
					}
					else if (hex == ESiegeHex::GATE_OUTER && i-1 >= 0 && path.first[i-1] != ESiegeHex::GATE_INNER)
					{
						gateMayCloseAtHex = path.first[i-1];
					}
				}
			}
		}

		bool stackIsMoving = true;

		while(stackIsMoving)
		{
			if (v<tilesToMove)
			{
				logGlobal->error("Movement terminated abnormally");
				break;
			}

			bool gateStateChanging = false;
			//special handling for opening gate on from starting hex
			if (openGateAtHex.isValid() && openGateAtHex == start)
				gateStateChanging = true;
			else
			{
				for (bool obstacleHit = false; (!obstacleHit) && (!gateStateChanging) && (v >= tilesToMove); --v)
				{
					BattleHex hex = path.first[v];
					tiles.push_back(hex);

					if ((openGateAtHex.isValid() && openGateAtHex == hex) ||
						(gateMayCloseAtHex.isValid() && gateMayCloseAtHex == hex))
					{
						gateStateChanging = true;
					}

					//if we walked onto something, finalize this portion of stack movement check into obstacle
					if(!gameHandler->battleGetAllObstaclesOnPos(hex, false).empty())
						obstacleHit = true;

					if (curStack->doubleWide())
					{
						BattleHex otherHex = curStack->occupiedHex(hex);
						//two hex creature hit obstacle by backside
						auto obstacle2 = gameHandler->battleGetAllObstaclesOnPos(otherHex, false);
						if(otherHex.isValid() && !obstacle2.empty())
							obstacleHit = true;
					}
					if(!obstacleHit)
						passed.insert(hex);
				}
			}

			if (!tiles.empty())
			{
				//commit movement
				BattleStackMoved sm;
				sm.stack = curStack->unitId();
				sm.distance = path.second;
				sm.teleporting = false;
				sm.tilesToMove = tiles;
				gameHandler->sendAndApply(&sm);
				tiles.clear();
			}

			//we don't handle obstacle at the destination tile -> it's handled separately in the if at the end
			if (curStack->getPosition() != dest)
			{
				if(stackIsMoving && start != curStack->getPosition())
				{
					stackIsMoving = gameHandler->handleObstacleTriggersForUnit(*gameHandler->spellEnv, *curStack, passed);
					passed.insert(curStack->getPosition());
					if(curStack->doubleWide())
						passed.insert(curStack->occupiedHex());
				}
				if (gateStateChanging)
				{
					if (curStack->getPosition() == openGateAtHex)
					{
						openGateAtHex = BattleHex();
						//only open gate if stack is still alive
						if (curStack->alive())
						{
							BattleUpdateGateState db;
							db.state = EGateState::OPENED;
							gameHandler->sendAndApply(&db);
						}
					}
					else if (curStack->getPosition() == gateMayCloseAtHex)
					{
						gateMayCloseAtHex = BattleHex();
						updateGateState();
					}
				}
			}
			else
				//movement finished normally: we reached destination
				stackIsMoving = false;
		}
	}
	//handle last hex separately for deviation
	if (VLC->settings()->getBoolean(EGameSettings::COMBAT_ONE_HEX_TRIGGERS_OBSTACLES))
	{
		if (dest == battle::Unit::occupiedHex(start, curStack->doubleWide(), curStack->unitSide())
			|| start == battle::Unit::occupiedHex(dest, curStack->doubleWide(), curStack->unitSide()))
			passed.clear(); //Just empty passed, obstacles will handled automatically
	}
	//handling obstacle on the final field (separate, because it affects both flying and walking stacks)
	gameHandler->handleObstacleTriggersForUnit(*gameHandler->spellEnv, *curStack, passed);

	return ret;
}

void BattleProcessor::setupBattle(int3 tile, const CArmedInstance *armies[2], const CGHeroInstance *heroes[2], bool creatureBank, const CGTownInstance *town)
{
	battleResult.set(nullptr);

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

	engageIntoBattle(bs.info->sides[0].color);
	engageIntoBattle(bs.info->sides[1].color);

	auto lastBattleQuery = std::dynamic_pointer_cast<CBattleQuery>(gameHandler->queries->topQuery(bs.info->sides[0].color));
	bs.info->replayAllowed = lastBattleQuery == nullptr && !bs.info->sides[1].color.isValidPlayer();

	gameHandler->sendAndApply(&bs);
}

void BattleProcessor::checkBattleStateChanges()
{
	//check if drawbridge state need to be changes
	if (gameHandler->battleGetSiegeLevel() > 0)
		updateGateState();

	//check if battle ended
	if (auto result = gameHandler->battleIsFinished())
	{
		setBattleResult(BattleResult::NORMAL, *result);
	}
}

bool BattleProcessor::makeBattleAction(BattleAction &ba)
{
	bool ok = true;

	battle::Target target = ba.getTarget(gameHandler->gameState()->curB);

	const CStack * stack = gameHandler->battleGetStackByID(ba.stackNumber); //may be nullptr if action is not about stack

	const bool isAboutActiveStack = stack && (ba.stackNumber == gameHandler->gameState()->curB->getActiveStackID());

	logGlobal->trace("Making action: %s", ba.toString());

	switch(ba.actionType)
	{
	case EActionType::WALK: //walk
	case EActionType::DEFEND: //defend
	case EActionType::WAIT: //wait
	case EActionType::WALK_AND_ATTACK: //walk or attack
	case EActionType::SHOOT: //shoot
	case EActionType::CATAPULT: //catapult
	case EActionType::STACK_HEAL: //healing with First Aid Tent
	case EActionType::MONSTER_SPELL:

		if (!stack)
		{
			gameHandler->complain("No such stack!");
			return false;
		}
		if (!stack->alive())
		{
			gameHandler->complain("This stack is dead: " + stack->nodeName());
			return false;
		}

		if (gameHandler->battleTacticDist())
		{
			if (stack && stack->unitSide() != gameHandler->battleGetTacticsSide())
			{
				gameHandler->complain("This is not a stack of side that has tactics!");
				return false;
			}
		}
		else if (!isAboutActiveStack)
		{
			gameHandler->complain("Action has to be about active stack!");
			return false;
		}
	}

	auto wrapAction = [this](BattleAction &ba)
	{
		StartAction startAction(ba);
		gameHandler->sendAndApply(&startAction);

		return vstd::makeScopeGuard([&]()
		{
			gameHandler->sendAndApply(&end_action);
		});
	};

	switch(ba.actionType)
	{
	case EActionType::END_TACTIC_PHASE: //wait
	case EActionType::BAD_MORALE:
	case EActionType::NO_ACTION:
		{
			auto wrapper = wrapAction(ba);
			break;
		}
	case EActionType::WALK:
		{
			auto wrapper = wrapAction(ba);
			if(target.size() < 1)
			{
				gameHandler->complain("Destination required for move action.");
				ok = false;
				break;
			}
			int walkedTiles = moveStack(ba.stackNumber, target.at(0).hexValue); //move
			if (!walkedTiles)
				gameHandler->complain("Stack failed movement!");
			break;
		}
	case EActionType::DEFEND:
		{
			//defensive stance, TODO: filter out spell boosts from bonus (stone skin etc.)
			SetStackEffect sse;
			Bonus defenseBonusToAdd(BonusDuration::STACK_GETS_TURN, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 20, -1, PrimarySkill::DEFENSE, BonusValueType::PERCENT_TO_ALL);
			Bonus bonus2(BonusDuration::STACK_GETS_TURN, BonusType::PRIMARY_SKILL, BonusSource::OTHER, stack->valOfBonuses(BonusType::DEFENSIVE_STANCE),
				 -1, PrimarySkill::DEFENSE, BonusValueType::ADDITIVE_VALUE);
			Bonus alternativeWeakCreatureBonus(BonusDuration::STACK_GETS_TURN, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 1, -1, PrimarySkill::DEFENSE, BonusValueType::ADDITIVE_VALUE);

			BonusList defence = *stack->getBonuses(Selector::typeSubtype(BonusType::PRIMARY_SKILL, PrimarySkill::DEFENSE));
			int oldDefenceValue = defence.totalValue();

			defence.push_back(std::make_shared<Bonus>(defenseBonusToAdd));
			defence.push_back(std::make_shared<Bonus>(bonus2));

			int difference = defence.totalValue() - oldDefenceValue;
			std::vector<Bonus> buffer;
			if(difference == 0) //give replacement bonus for creatures not reaching 5 defense points (20% of def becomes 0)
			{
				difference = 1;
				buffer.push_back(alternativeWeakCreatureBonus);
			}
			else
			{
				buffer.push_back(defenseBonusToAdd);
			}

			buffer.push_back(bonus2);

			sse.toUpdate.push_back(std::make_pair(ba.stackNumber, buffer));
			gameHandler->sendAndApply(&sse);

			BattleLogMessage message;

			MetaString text;
			stack->addText(text, EMetaText::GENERAL_TXT, 120);
			stack->addNameReplacement(text);
			text.replaceNumber(difference);

			message.lines.push_back(text);

			gameHandler->sendAndApply(&message);
			//don't break - we share code with next case
		}
		[[fallthrough]];
	case EActionType::WAIT:
		{
			auto wrapper = wrapAction(ba);
			break;
		}
	case EActionType::RETREAT: //retreat/flee
		{
			if (!gameHandler->gameState()->curB->battleCanFlee(gameHandler->gameState()->curB->sides.at(ba.side).color))
				gameHandler->complain("Cannot retreat!");
			else
				setBattleResult(BattleResult::ESCAPE, !ba.side); //surrendering side loses
			break;
		}
	case EActionType::SURRENDER:
		{
			PlayerColor player = gameHandler->gameState()->curB->sides.at(ba.side).color;
			int cost = gameHandler->gameState()->curB->battleGetSurrenderCost(player);
			if (cost < 0)
				gameHandler->complain("Cannot surrender!");
			else if (gameHandler->getResource(player, EGameResID::GOLD) < cost)
				gameHandler->complain("Not enough gold to surrender!");
			else
			{
				gameHandler->giveResource(player, EGameResID::GOLD, -cost);
				setBattleResult(BattleResult::SURRENDER, !ba.side); //surrendering side loses
			}
			break;
		}
	case EActionType::WALK_AND_ATTACK: //walk or attack
		{
			auto wrapper = wrapAction(ba);

			if(!stack)
			{
				gameHandler->complain("No attacker");
				ok = false;
				break;
			}

			if(target.size() < 2)
			{
				gameHandler->complain("Two destinations required for attack action.");
				ok = false;
				break;
			}

			BattleHex attackPos = target.at(0).hexValue;
			BattleHex destinationTile = target.at(1).hexValue;
			const CStack * destinationStack = gameHandler->gameState()->curB->battleGetStackByPos(destinationTile, true);

			if(!destinationStack)
			{
				gameHandler->complain("Invalid target to attack");
				ok = false;
				break;
			}

			BattleHex startingPos = stack->getPosition();
			int distance = moveStack(ba.stackNumber, attackPos);

			logGlobal->trace("%s will attack %s", stack->nodeName(), destinationStack->nodeName());

			if(stack->getPosition() != attackPos
				&& !(stack->doubleWide() && (stack->getPosition() == attackPos.cloneInDirection(stack->destShiftDir(), false)))
				)
			{
				// we were not able to reach destination tile, nor occupy specified hex
				// abort attack attempt, but treat this case as legal - we may have stepped onto a quicksands/mine
				break;
			}

			if(destinationStack && stack->unitId() == destinationStack->unitId()) //we should just move, it will be handled by following check
			{
				destinationStack = nullptr;
			}

			if(!destinationStack)
			{
				gameHandler->complain("Unit can not attack itself");
				ok = false;
				break;
			}

			if(!CStack::isMeleeAttackPossible(stack, destinationStack))
			{
				gameHandler->complain("Attack cannot be performed!");
				ok = false;
				break;
			}

			//attack
			int totalAttacks = stack->totalAttacks.getMeleeValue();

			//TODO: move to CUnitState
			const auto * attackingHero = gameHandler->gameState()->curB->battleGetFightingHero(ba.side);
			if(attackingHero)
			{
				totalAttacks += attackingHero->valOfBonuses(BonusType::HERO_GRANTS_ATTACKS, stack->creatureIndex());
			}


			const bool firstStrike = destinationStack->hasBonusOfType(BonusType::FIRST_STRIKE);
			const bool retaliation = destinationStack->ableToRetaliate();
			for (int i = 0; i < totalAttacks; ++i)
			{
				//first strike
				if(i == 0 && firstStrike && retaliation)
				{
					makeAttack(destinationStack, stack, 0, stack->getPosition(), true, false, true);
				}

				//move can cause death, eg. by walking into the moat, first strike can cause death or paralysis/petrification
				if(stack->alive() && !stack->hasBonusOfType(BonusType::NOT_ACTIVE) && destinationStack->alive())
				{
					makeAttack(stack, destinationStack, (i ? 0 : distance), destinationTile, i==0, false, false);//no distance travelled on second attack
				}

				//counterattack
				//we check retaliation twice, so if it unblocked during attack it will work only on next attack
				if(stack->alive()
					&& !stack->hasBonusOfType(BonusType::BLOCKS_RETALIATION)
					&& (i == 0 && !firstStrike)
					&& retaliation && destinationStack->ableToRetaliate())
				{
					makeAttack(destinationStack, stack, 0, stack->getPosition(), true, false, true);
				}
			}

			//return
			if(stack->hasBonusOfType(BonusType::RETURN_AFTER_STRIKE)
				&& target.size() == 3
				&& startingPos != stack->getPosition()
				&& startingPos == target.at(2).hexValue
				&& stack->alive())
			{
				moveStack(ba.stackNumber, startingPos);
				//NOTE: curStack->unitId() == ba.stackNumber (rev 1431)
			}
			break;
		}
	case EActionType::SHOOT:
		{
			if(target.size() < 1)
			{
				gameHandler->complain("Destination required for shot action.");
				ok = false;
				break;
			}

			auto destination = target.at(0).hexValue;

			const CStack * destinationStack = gameHandler->gameState()->curB->battleGetStackByPos(destination);

			if (!gameHandler->gameState()->curB->battleCanShoot(stack, destination))
			{
				gameHandler->complain("Cannot shoot!");
				break;
			}
			if (!destinationStack)
			{
				gameHandler->complain("No target to shoot!");
				break;
			}

			auto wrapper = wrapAction(ba);

			makeAttack(stack, destinationStack, 0, destination, true, true, false);

			//ranged counterattack
			if (destinationStack->hasBonusOfType(BonusType::RANGED_RETALIATION)
				&& !stack->hasBonusOfType(BonusType::BLOCKS_RANGED_RETALIATION)
				&& destinationStack->ableToRetaliate()
				&& gameHandler->gameState()->curB->battleCanShoot(destinationStack, stack->getPosition())
				&& stack->alive()) //attacker may have died (fire shield)
			{
				makeAttack(destinationStack, stack, 0, stack->getPosition(), true, true, true);
			}
			//allow more than one additional attack

			int totalRangedAttacks = stack->totalAttacks.getRangedValue();

			//TODO: move to CUnitState
			const auto * attackingHero = gameHandler->gameState()->curB->battleGetFightingHero(ba.side);
			if(attackingHero)
			{
				totalRangedAttacks += attackingHero->valOfBonuses(BonusType::HERO_GRANTS_ATTACKS, stack->creatureIndex());
			}


			for(int i = 1; i < totalRangedAttacks; ++i)
			{
				if(
					stack->alive()
					&& destinationStack->alive()
					&& stack->shots.canUse()
					)
				{
					makeAttack(stack, destinationStack, 0, destination, false, true, false);
				}
			}
			break;
		}
	case EActionType::CATAPULT:
		{
			auto wrapper = wrapAction(ba);
			const CStack * shooter = gameHandler->gameState()->curB->battleGetStackByID(ba.stackNumber);
			std::shared_ptr<const Bonus> catapultAbility = stack->getBonusLocalFirst(Selector::type()(BonusType::CATAPULT));
			if(!catapultAbility || catapultAbility->subtype < 0)
			{
				gameHandler->complain("We do not know how to shoot :P");
			}
			else
			{
				const CSpell * spell = SpellID(catapultAbility->subtype).toSpell();
				spells::BattleCast parameters(gameHandler->gameState()->curB, shooter, spells::Mode::SPELL_LIKE_ATTACK, spell); //We can shot infinitely by catapult
				auto shotLevel = stack->valOfBonuses(Selector::typeSubtype(BonusType::CATAPULT_EXTRA_SHOTS, catapultAbility->subtype));
				parameters.setSpellLevel(shotLevel);
				parameters.cast(gameHandler->spellEnv, target);
			}
			//finish by scope guard
			break;
		}
		case EActionType::STACK_HEAL: //healing with First Aid Tent
		{
			auto wrapper = wrapAction(ba);
			const CStack * healer = gameHandler->gameState()->curB->battleGetStackByID(ba.stackNumber);

			if(target.size() < 1)
			{
				gameHandler->complain("Destination required for heal action.");
				ok = false;
				break;
			}

			const battle::Unit * destStack = nullptr;
			std::shared_ptr<const Bonus> healerAbility = stack->getBonusLocalFirst(Selector::type()(BonusType::HEALER));

			if(target.at(0).unitValue)
				destStack = target.at(0).unitValue;
			else
				destStack = gameHandler->gameState()->curB->battleGetUnitByPos(target.at(0).hexValue);

			if(healer == nullptr || destStack == nullptr || !healerAbility || healerAbility->subtype < 0)
			{
				gameHandler->complain("There is either no healer, no destination, or healer cannot heal :P");
			}
			else
			{
				const CSpell * spell = SpellID(healerAbility->subtype).toSpell();
				spells::BattleCast parameters(gameHandler->gameState()->curB, healer, spells::Mode::SPELL_LIKE_ATTACK, spell); //We can heal infinitely by first aid tent
				auto dest = battle::Destination(destStack, target.at(0).hexValue);
				parameters.setSpellLevel(0);
				parameters.cast(gameHandler->spellEnv, {dest});
			}
			break;
		}
		case EActionType::MONSTER_SPELL:
		{
			auto wrapper = wrapAction(ba);

			const CStack * stack = gameHandler->gameState()->curB->battleGetStackByID(ba.stackNumber);
			SpellID spellID = SpellID(ba.actionSubtype);

			std::shared_ptr<const Bonus> randSpellcaster = stack->getBonus(Selector::type()(BonusType::RANDOM_SPELLCASTER));
			std::shared_ptr<const Bonus> spellcaster = stack->getBonus(Selector::typeSubtype(BonusType::SPELLCASTER, spellID));

			//TODO special bonus for genies ability
			if (randSpellcaster && gameHandler->battleGetRandomStackSpell(gameHandler->getRandomGenerator(), stack, CBattleInfoCallback::RANDOM_AIMED) < 0)
				spellID = gameHandler->battleGetRandomStackSpell(gameHandler->getRandomGenerator(), stack, CBattleInfoCallback::RANDOM_GENIE);

			if (spellID < 0)
				gameHandler->complain("That stack can't cast spells!");
			else
			{
				const CSpell * spell = SpellID(spellID).toSpell();
				spells::BattleCast parameters(gameHandler->gameState()->curB, stack, spells::Mode::CREATURE_ACTIVE, spell);
				int32_t spellLvl = 0;
				if(spellcaster)
					vstd::amax(spellLvl, spellcaster->val);
				if(randSpellcaster)
					vstd::amax(spellLvl, randSpellcaster->val);
				parameters.setSpellLevel(spellLvl);
				parameters.cast(gameHandler->spellEnv, target);
			}
			break;
		}
	}
	if(ba.actionType == EActionType::WAIT || ba.actionType == EActionType::DEFEND
			|| ba.actionType == EActionType::SHOOT || ba.actionType == EActionType::MONSTER_SPELL)
		gameHandler->handleObstacleTriggersForUnit(*gameHandler->spellEnv, *stack);
	if(ba.stackNumber == gameHandler->gameState()->curB->activeStack || battleResult.get()) //active stack has moved or battle has finished
		battleMadeAction.setn(true);
	return ok;
}

bool BattleProcessor::makeCustomAction(BattleAction & ba)
{
	switch(ba.actionType)
	{
	case EActionType::HERO_SPELL:
		{
			COMPLAIN_RET_FALSE_IF(ba.side > 1, "Side must be 0 or 1!");

			const CGHeroInstance *h = gameHandler->gameState()->curB->battleGetFightingHero(ba.side);
			COMPLAIN_RET_FALSE_IF((!h), "Wrong caster!");

			const CSpell * s = SpellID(ba.actionSubtype).toSpell();
			if (!s)
			{
				logGlobal->error("Wrong spell id (%d)!", ba.actionSubtype);
				return false;
			}

			spells::BattleCast parameters(gameHandler->gameState()->curB, h, spells::Mode::HERO, s);

			spells::detail::ProblemImpl problem;

			auto m = s->battleMechanics(&parameters);

			if(!m->canBeCast(problem))//todo: should we check aimed cast?
			{
				logGlobal->warn("Spell cannot be cast!");
				std::vector<std::string> texts;
				problem.getAll(texts);
				for(auto s : texts)
					logGlobal->warn(s);
				return false;
			}

			StartAction start_action(ba);
			gameHandler->sendAndApply(&start_action); //start spell casting

			parameters.cast(gameHandler->spellEnv, ba.getTarget(gameHandler->gameState()->curB));

			gameHandler->sendAndApply(&end_action);
			if (!gameHandler->gameState()->curB->battleGetStackByID(gameHandler->gameState()->curB->activeStack))
			{
				battleMadeAction.setn(true);
			}
			checkBattleStateChanges();
			if (battleResult.get())
			{
				battleMadeAction.setn(true);
				//battle will be ended by startBattle function
				//endBattle(gameHandler->gameState()->curB->tile, gameHandler->gameState()->curB->heroes[0], gameHandler->gameState()->curB->heroes[1]);
			}

			return true;

		}
	}
	return false;
}

void BattleProcessor::stackEnchantedTrigger(const CStack * st)
{
	auto bl = *(st->getBonuses(Selector::type()(BonusType::ENCHANTED)));
	for(auto b : bl)
	{
		const CSpell * sp = SpellID(b->subtype).toSpell();
		if(!sp)
			continue;

		const int32_t val = bl.valOfBonuses(Selector::typeSubtype(b->type, b->subtype));
		const int32_t level = ((val > 3) ? (val - 3) : val);

		spells::BattleCast battleCast(gameHandler->gameState()->curB, st, spells::Mode::PASSIVE, sp);
		//this makes effect accumulate for at most 50 turns by default, but effect may be permanent and last till the end of battle
		battleCast.setEffectDuration(50);
		battleCast.setSpellLevel(level);
		spells::Target target;

		if(val > 3)
		{
			for(auto s : gameHandler->gameState()->curB->battleGetAllStacks())
				if(gameHandler->battleMatchOwner(st, s, true) && s->isValidTarget()) //all allied
					target.emplace_back(s);
		}
		else
		{
			target.emplace_back(st);
		}
		battleCast.applyEffects(gameHandler->spellEnv, target, false, true);
	}
}

void BattleProcessor::stackTurnTrigger(const CStack *st)
{
	BattleTriggerEffect bte;
	bte.stackID = st->unitId();
	bte.effect = -1;
	bte.val = 0;
	bte.additionalInfo = 0;
	if (st->alive())
	{
		//unbind
		if (st->hasBonus(Selector::type()(BonusType::BIND_EFFECT)))
		{
			bool unbind = true;
			BonusList bl = *(st->getBonuses(Selector::type()(BonusType::BIND_EFFECT)));
			auto adjacent = gameHandler->gameState()->curB->battleAdjacentUnits(st);

			for (auto b : bl)
			{
				if(b->additionalInfo != CAddInfo::NONE)
				{
					const CStack * stack = gameHandler->gameState()->curB->battleGetStackByID(b->additionalInfo[0]); //binding stack must be alive and adjacent
					if(stack)
					{
						if(vstd::contains(adjacent, stack)) //binding stack is still present
							unbind = false;
					}
				}
				else
				{
					unbind = false;
				}
			}
			if (unbind)
			{
				BattleSetStackProperty ssp;
				ssp.which = BattleSetStackProperty::UNBIND;
				ssp.stackID = st->unitId();
				gameHandler->sendAndApply(&ssp);
			}
		}

		if (st->hasBonusOfType(BonusType::POISON))
		{
			std::shared_ptr<const Bonus> b = st->getBonusLocalFirst(Selector::source(BonusSource::SPELL_EFFECT, SpellID::POISON).And(Selector::type()(BonusType::STACK_HEALTH)));
			if (b) //TODO: what if not?...
			{
				bte.val = std::max (b->val - 10, -(st->valOfBonuses(BonusType::POISON)));
				if (bte.val < b->val) //(negative) poison effect increases - update it
				{
					bte.effect = vstd::to_underlying(BonusType::POISON);
					gameHandler->sendAndApply(&bte);
				}
			}
		}
		if(st->hasBonusOfType(BonusType::MANA_DRAIN) && !st->drainedMana)
		{
			const PlayerColor opponent = gameHandler->gameState()->curB->otherPlayer(gameHandler->gameState()->curB->battleGetOwner(st));
			const CGHeroInstance * opponentHero = gameHandler->gameState()->curB->getHero(opponent);
			if(opponentHero)
			{
				ui32 manaDrained = st->valOfBonuses(BonusType::MANA_DRAIN);
				vstd::amin(manaDrained, opponentHero->mana);
				if(manaDrained)
				{
					bte.effect = vstd::to_underlying(BonusType::MANA_DRAIN);
					bte.val = manaDrained;
					bte.additionalInfo = opponentHero->id.getNum(); //for sanity
					gameHandler->sendAndApply(&bte);
				}
			}
		}
		if (st->isLiving() && !st->hasBonusOfType(BonusType::FEARLESS))
		{
			bool fearsomeCreature = false;
			for (CStack * stack : gameHandler->gameState()->curB->stacks)
			{
				if (gameHandler->battleMatchOwner(st, stack) && stack->alive() && stack->hasBonusOfType(BonusType::FEAR))
				{
					fearsomeCreature = true;
					break;
				}
			}
			if (fearsomeCreature)
			{
				if (gameHandler->getRandomGenerator().nextInt(99) < 10) //fixed 10%
				{
					bte.effect = vstd::to_underlying(BonusType::FEAR);
					gameHandler->sendAndApply(&bte);
				}
			}
		}
		BonusList bl = *(st->getBonuses(Selector::type()(BonusType::ENCHANTER)));
		int side = gameHandler->gameState()->curB->whatSide(st->unitOwner());
		if(st->canCast() && gameHandler->gameState()->curB->battleGetEnchanterCounter(side) == 0)
		{
			bool cast = false;
			while(!bl.empty() && !cast)
			{
				auto bonus = *RandomGeneratorUtil::nextItem(bl, gameHandler->getRandomGenerator());
				auto spellID = SpellID(bonus->subtype);
				const CSpell * spell = SpellID(spellID).toSpell();
				bl.remove_if([&bonus](const Bonus * b)
				{
					return b == bonus.get();
				});
				spells::BattleCast parameters(gameHandler->gameState()->curB, st, spells::Mode::ENCHANTER, spell);
				parameters.setSpellLevel(bonus->val);
				parameters.massive = true;
				parameters.smart = true;
				//todo: recheck effect level
				if(parameters.castIfPossible(gameHandler->spellEnv, spells::Target(1, spells::Destination())))
				{
					cast = true;

					int cooldown = bonus->additionalInfo[0];
					BattleSetStackProperty ssp;
					ssp.which = BattleSetStackProperty::ENCHANTER_COUNTER;
					ssp.absolute = false;
					ssp.val = cooldown;
					ssp.stackID = st->unitId();
					gameHandler->sendAndApply(&ssp);
				}
			}
		}
	}
}

void BattleProcessor::makeStackDoNothing(const CStack * next)
{
	BattleAction doNothing;
	doNothing.actionType = EActionType::NO_ACTION;
	doNothing.side = next->unitSide();
	doNothing.stackNumber = next->unitId();

	makeAutomaticAction(next, doNothing);
}


void BattleProcessor::setBattleResult(BattleResult::EResult resultType, int victoriusSide)
{
	boost::unique_lock<boost::mutex> guard(battleResult.mx);
	if (battleResult.data)
	{
		gameHandler->complain((boost::format("The battle result has been already set (to %d, asked to %d)")
				  % battleResult.data->result % resultType).str());
		return;
	}
	auto br = new BattleResult();
	br->result = resultType;
	br->winner = victoriusSide; //surrendering side loses
	gameHandler->gameState()->curB->calculateCasualties(br->casualties);
	battleResult.data = br;
}

void BattleProcessor::removeObstacle(const CObstacleInstance & obstacle)
{
	BattleObstaclesChanged obsRem;
	obsRem.changes.emplace_back(obstacle.uniqueID, ObstacleChanges::EOperation::REMOVE);
	gameHandler->sendAndApply(&obsRem);
}

void BattleProcessor::updateGateState()
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

	bool hasForceFieldOnBridge = !gameHandler->battleGetAllObstaclesOnPos(BattleHex(ESiegeHex::GATE_BRIDGE), true).empty();
	bool hasStackAtGateInner   = gameHandler->gameState()->curB->battleGetUnitByPos(BattleHex(ESiegeHex::GATE_INNER), false) != nullptr;
	bool hasStackAtGateOuter   = gameHandler->gameState()->curB->battleGetUnitByPos(BattleHex(ESiegeHex::GATE_OUTER), false) != nullptr;
	bool hasStackAtGateBridge  = gameHandler->gameState()->curB->battleGetUnitByPos(BattleHex(ESiegeHex::GATE_BRIDGE), false) != nullptr;
	bool hasWideMoat           = vstd::contains_if(gameHandler->battleGetAllObstaclesOnPos(BattleHex(ESiegeHex::GATE_BRIDGE), false), [](const std::shared_ptr<const CObstacleInstance> & obst)
	{
		return obst->obstacleType == CObstacleInstance::MOAT;
	});

	BattleUpdateGateState db;
	db.state = gameHandler->gameState()->curB->si.gateState;
	if (gameHandler->gameState()->curB->si.wallState[EWallPart::GATE] == EWallState::DESTROYED)
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

	if (db.state != gameHandler->gameState()->curB->si.gateState)
		gameHandler->sendAndApply(&db);
}

