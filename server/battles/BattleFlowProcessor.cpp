/*
 * BattleFlowProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleFlowProcessor.h"

#include "BattleProcessor.h"

#include "../CGameHandler.h"

#include "../../lib/CStack.h"
#include "../../lib/GameSettings.h"
#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/battle/IBattleState.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/spells/BonusCaster.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/spells/ObstacleCasterProxy.h"

BattleFlowProcessor::BattleFlowProcessor(BattleProcessor * owner)
	: owner(owner)
	, gameHandler(nullptr)
{
}

void BattleFlowProcessor::setGameHandler(CGameHandler * newGameHandler)
{
	gameHandler = newGameHandler;
}

void BattleFlowProcessor::summonGuardiansHelper(const CBattleInfoCallback & battle, std::vector<BattleHex> & output, const BattleHex & targetPosition, ui8 side, bool targetIsTwoHex) //return hexes for summoning two hex monsters in output, target = unit to guard
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

void BattleFlowProcessor::tryPlaceMoats(const CBattleInfoCallback & battle)
{
	const auto * town = battle.battleGetDefendedTown();

	//Moat should be initialized here, because only here we can use spellcasting
	if (town && town->fortLevel() >= CGTownInstance::CITADEL)
	{
		const auto * h = battle.battleGetFightingHero(BattleSide::DEFENDER);
		const auto * actualCaster = h ? static_cast<const spells::Caster*>(h) : nullptr;
		auto moatCaster = spells::SilentCaster(battle.sideToPlayer(BattleSide::DEFENDER), actualCaster);
		auto cast = spells::BattleCast(&battle, &moatCaster, spells::Mode::PASSIVE, town->town->moatAbility.toSpell());
		auto target = spells::Target();
		cast.cast(gameHandler->spellEnv, target);
	}
}

void BattleFlowProcessor::onBattleStarted(const CBattleInfoCallback & battle)
{
	tryPlaceMoats(battle);
	
	gameHandler->turnTimerHandler.onBattleStart(battle.getBattle()->getBattleID());

	if (battle.battleGetTacticDist() == 0)
		onTacticsEnded(battle);
}

void BattleFlowProcessor::trySummonGuardians(const CBattleInfoCallback & battle, const CStack * stack)
{
	if (!stack->hasBonusOfType(BonusType::SUMMON_GUARDIANS))
		return;

	std::shared_ptr<const Bonus> summonInfo = stack->getBonus(Selector::type()(BonusType::SUMMON_GUARDIANS));
	auto accessibility = battle.getAccesibility();
	CreatureID creatureData = summonInfo->subtype.as<CreatureID>();
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
		summonGuardiansHelper(battle, targetHexes, stack->getPosition(), stack->unitSide(), targetIsBig);

	for(auto hex : targetHexes)
	{
		if(accessibility.accessible(hex, guardianIsBig, stack->unitSide())) //without this multiple creatures can occupy one hex
		{
			battle::UnitInfo info;
			info.id = battle.battleNextUnitId();
			info.count =  std::max(1, (int)(stack->getCount() * 0.01 * summonInfo->val));
			info.type = creatureData;
			info.side = stack->unitSide();
			info.position = hex;
			info.summoned = true;

			BattleUnitsChanged pack;
			pack.battleID = battle.getBattle()->getBattleID();
			pack.changedStacks.emplace_back(info.id, UnitChanges::EOperation::ADD);
			info.save(pack.changedStacks.back().data);
			gameHandler->sendAndApply(&pack);
		}
	}

	// send empty event to client
	// temporary(?) workaround to force animations to trigger
	StacksInjured fakeEvent;
	gameHandler->sendAndApply(&fakeEvent);
}

void BattleFlowProcessor::castOpeningSpells(const CBattleInfoCallback & battle)
{
	for (int i = 0; i < 2; ++i)
	{
		auto h = battle.battleGetFightingHero(i);
		if (!h)
			continue;

		TConstBonusListPtr bl = h->getBonuses(Selector::type()(BonusType::OPENING_BATTLE_SPELL));

		for (auto b : *bl)
		{
			spells::BonusCaster caster(h, b);

			const CSpell * spell = b->subtype.as<SpellID>().toSpell();

			spells::BattleCast parameters(&battle, &caster, spells::Mode::PASSIVE, spell);
			parameters.setSpellLevel(3);
			parameters.setEffectDuration(b->val);
			parameters.massive = true;
			parameters.castIfPossible(gameHandler->spellEnv, spells::Target());
		}
	}
}

void BattleFlowProcessor::onTacticsEnded(const CBattleInfoCallback & battle)
{
	//initial stacks appearance triggers, e.g. built-in bonus spells
	auto initialStacks = battle.battleGetAllStacks(true);

	for (const CStack * stack : initialStacks)
	{
		trySummonGuardians(battle, stack);
		stackEnchantedTrigger(battle, stack);
	}

	castOpeningSpells(battle);

	// it is possible that due to opening spells one side was eliminated -> check for end of battle
	if (owner->checkBattleStateChanges(battle))
		return;

	startNextRound(battle, true);
	activateNextStack(battle);
}

void BattleFlowProcessor::startNextRound(const CBattleInfoCallback & battle, bool isFirstRound)
{
	BattleNextRound bnr;
	bnr.battleID = battle.getBattle()->getBattleID();
	logGlobal->debug("Next round starts");
	gameHandler->sendAndApply(&bnr);

	// operate on copy - removing obstacles will invalidate iterator on 'battle' container
	auto obstacles = battle.battleGetAllObstacles();
	for (auto &obstPtr : obstacles)
	{
		if (const SpellCreatedObstacle *sco = dynamic_cast<const SpellCreatedObstacle *>(obstPtr.get()))
			if (sco->turnsRemaining == 0)
				removeObstacle(battle, *obstPtr);
	}

	for(auto stack : battle.battleGetAllStacks(true))
	{
		if(stack->alive() && !isFirstRound)
			stackEnchantedTrigger(battle, stack);
	}
}

const CStack * BattleFlowProcessor::getNextStack(const CBattleInfoCallback & battle)
{
	std::vector<battle::Units> q;
	battle.battleGetTurnOrder(q, 1, 0, -1); //todo: get rid of "turn -1"

	if(q.empty())
		return nullptr;

	if(q.front().empty())
		return nullptr;

	auto next = q.front().front();
	const auto stack = dynamic_cast<const CStack *>(next);

	// regeneration takes place before everything else but only during first turn attempt in each round
	// also works under blind and similar effects
	if(stack && stack->alive() && !stack->waiting)
	{
		BattleTriggerEffect bte;
		bte.battleID = battle.getBattle()->getBattleID();
		bte.stackID = stack->unitId();
		bte.effect = vstd::to_underlying(BonusType::HP_REGENERATION);

		const int32_t lostHealth = stack->getMaxHealth() - stack->getFirstHPleft();
		if(stack->hasBonusOfType(BonusType::HP_REGENERATION))
			bte.val = std::min(lostHealth, stack->valOfBonuses(BonusType::HP_REGENERATION));

		if(bte.val) // anything to heal
			gameHandler->sendAndApply(&bte);
	}

	if(!next || !next->willMove())
		return nullptr;

	return stack;
}

void BattleFlowProcessor::activateNextStack(const CBattleInfoCallback & battle)
{
	// Find next stack that requires manual control
	for (;;)
	{
		// battle has ended
		if (owner->checkBattleStateChanges(battle))
			return;

		const CStack * next = getNextStack(battle);

		if (!next)
		{
			// No stacks to move - start next round
			startNextRound(battle, false);
			next = getNextStack(battle);
			if (!next)
				throw std::runtime_error("Failed to find valid stack to act!");
		}

		BattleUnitsChanged removeGhosts;
		removeGhosts.battleID = battle.getBattle()->getBattleID();

		auto pendingGhosts = battle.battleGetStacksIf([](const CStack * stack){
			return stack->ghostPending;
		});

		for(auto stack : pendingGhosts)
			removeGhosts.changedStacks.emplace_back(stack->unitId(), UnitChanges::EOperation::REMOVE);

		if(!removeGhosts.changedStacks.empty())
			gameHandler->sendAndApply(&removeGhosts);
		
		gameHandler->turnTimerHandler.onBattleNextStack(battle.getBattle()->getBattleID(), *next);

		if (!tryMakeAutomaticAction(battle, next))
		{
			setActiveStack(battle, next);
			break;
		}
	}
}

bool BattleFlowProcessor::tryMakeAutomaticAction(const CBattleInfoCallback & battle, const CStack * next)
{
	// check for bad morale => freeze
	int nextStackMorale = next->moraleVal();
	if(!next->hadMorale && !next->waited() && nextStackMorale < 0)
	{
		auto diceSize = VLC->settings()->getVector(EGameSettings::COMBAT_BAD_MORALE_DICE);
		size_t diceIndex = std::min<size_t>(diceSize.size(), -nextStackMorale) - 1; // array index, so 0-indexed

		if(diceSize.size() > 0 && gameHandler->getRandomGenerator().nextInt(1, diceSize[diceIndex]) == 1)
		{
			//unit loses its turn - empty freeze action
			BattleAction ba;
			ba.actionType = EActionType::BAD_MORALE;
			ba.side = next->unitSide();
			ba.stackNumber = next->unitId();

			makeAutomaticAction(battle, next, ba);
			return true;
		}
	}

	if (next->hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE)) //while in berserk
	{
		logGlobal->trace("Handle Berserk effect");
		std::pair<const battle::Unit *, BattleHex> attackInfo = battle.getNearestStack(next);
		if (attackInfo.first != nullptr)
		{
			BattleAction attack;
			attack.actionType = EActionType::WALK_AND_ATTACK;
			attack.side = next->unitSide();
			attack.stackNumber = next->unitId();
			attack.aimToHex(attackInfo.second);
			attack.aimToUnit(attackInfo.first);

			makeAutomaticAction(battle, next, attack);
			logGlobal->trace("Attacked nearest target %s", attackInfo.first->getDescription());
		}
		else
		{
			makeStackDoNothing(battle, next);
			logGlobal->trace("No target found");
		}
		return true;
	}

	const CGHeroInstance * curOwner = battle.battleGetOwnerHero(next);
	const CreatureID stackCreatureId = next->unitType()->getId();

	if ((stackCreatureId == CreatureID::ARROW_TOWERS || stackCreatureId == CreatureID::BALLISTA)
		&& (!curOwner || gameHandler->getRandomGenerator().nextInt(99) >= curOwner->valOfBonuses(BonusType::MANUAL_CONTROL, BonusSubtypeID(stackCreatureId))))
	{
		BattleAction attack;
		attack.actionType = EActionType::SHOOT;
		attack.side = next->unitSide();
		attack.stackNumber = next->unitId();

		//TODO: select target by priority

		const battle::Unit * target = nullptr;

		for(auto & elem : battle.battleGetAllStacks(true))
		{
			if(elem->unitType()->getId() != CreatureID::CATAPULT
			   && elem->unitOwner() != next->unitOwner()
			   && elem->isValidTarget()
			   && battle.battleCanShoot(next, elem->getPosition()))
			{
				target = elem;
				break;
			}
		}

		if(target == nullptr)
		{
			makeStackDoNothing(battle, next);
		}
		else
		{
			attack.aimToUnit(target);
			makeAutomaticAction(battle, next, attack);
		}
		return true;
	}

	if (next->unitType()->getId() == CreatureID::CATAPULT)
	{
		const auto & attackableBattleHexes = battle.getAttackableBattleHexes();

		if (attackableBattleHexes.empty())
		{
			makeStackDoNothing(battle, next);
			return true;
		}

		if (!curOwner || gameHandler->getRandomGenerator().nextInt(99) >= curOwner->valOfBonuses(BonusType::MANUAL_CONTROL, BonusSubtypeID(CreatureID(CreatureID::CATAPULT))))
		{
			BattleAction attack;
			attack.actionType = EActionType::CATAPULT;
			attack.side = next->unitSide();
			attack.stackNumber = next->unitId();

			makeAutomaticAction(battle, next, attack);
			return true;
		}
	}

	if (next->unitType()->getId() == CreatureID::FIRST_AID_TENT)
	{
		TStacks possibleStacks = battle.battleGetStacksIf([=](const CStack * s)
		{
			return s->unitOwner() == next->unitOwner() && s->canBeHealed();
		});

		if (possibleStacks.empty())
		{
			makeStackDoNothing(battle, next);
			return true;
		}

		if (!curOwner || gameHandler->getRandomGenerator().nextInt(99) >= curOwner->valOfBonuses(BonusType::MANUAL_CONTROL, BonusSubtypeID(CreatureID(CreatureID::FIRST_AID_TENT))))
		{
			RandomGeneratorUtil::randomShuffle(possibleStacks, gameHandler->getRandomGenerator());
			const CStack * toBeHealed = possibleStacks.front();

			BattleAction heal;
			heal.actionType = EActionType::STACK_HEAL;
			heal.aimToUnit(toBeHealed);
			heal.side = next->unitSide();
			heal.stackNumber = next->unitId();

			makeAutomaticAction(battle, next, heal);
			return true;
		}
	}

	stackTurnTrigger(battle, next); //various effects

	if(next->fear)
	{
		makeStackDoNothing(battle, next); //end immediately if stack was affected by fear
		return true;
	}
	return false;
}

bool BattleFlowProcessor::rollGoodMorale(const CBattleInfoCallback & battle, const CStack * next)
{
	//check for good morale
	auto nextStackMorale = next->moraleVal();
	if(    !next->hadMorale
		&& !next->defending
		&& !next->waited()
		&& !next->fear
		&& next->alive()
		&& next->canMove()
		&& nextStackMorale > 0)
	{
		auto diceSize = VLC->settings()->getVector(EGameSettings::COMBAT_GOOD_MORALE_DICE);
		size_t diceIndex = std::min<size_t>(diceSize.size(), nextStackMorale) - 1; // array index, so 0-indexed

		if(diceSize.size() > 0 && gameHandler->getRandomGenerator().nextInt(1, diceSize[diceIndex]) == 1)
		{
			BattleTriggerEffect bte;
			bte.battleID = battle.getBattle()->getBattleID();
			bte.stackID = next->unitId();
			bte.effect = vstd::to_underlying(BonusType::MORALE);
			bte.val = 1;
			bte.additionalInfo = 0;
			gameHandler->sendAndApply(&bte); //play animation
			return true;
		}
	}
	return false;
}

void BattleFlowProcessor::onActionMade(const CBattleInfoCallback & battle, const BattleAction &ba)
{
	const auto * actedStack = battle.battleGetStackByID(ba.stackNumber, false);
	const auto * activeStack = battle.battleActiveUnit();
	if (ba.actionType == EActionType::END_TACTIC_PHASE)
	{
		onTacticsEnded(battle);
		return;
	}


	//we're after action, all results applied

	// check whether action has ended the battle
	if(owner->checkBattleStateChanges(battle))
		return;

	// tactics - next stack will be selected by player
	if(battle.battleGetTacticDist() != 0)
		return;

	if (ba.isUnitAction())
	{
		assert(activeStack != nullptr);
		assert(actedStack != nullptr);

		if (rollGoodMorale(battle, actedStack))
		{
			// Good morale - same stack makes 2nd turn
			setActiveStack(battle, actedStack);
			return;
		}
	}
	else
	{
		if (activeStack && activeStack->alive())
		{
			// this is action made by hero AND unit is alive (e.g. not killed by casted spell)
			// keep current active stack for next action
			setActiveStack(battle, activeStack);
			return;
		}
	}

	activateNextStack(battle);
}

void BattleFlowProcessor::makeStackDoNothing(const CBattleInfoCallback & battle, const CStack * next)
{
	BattleAction doNothing;
	doNothing.actionType = EActionType::NO_ACTION;
	doNothing.side = next->unitSide();
	doNothing.stackNumber = next->unitId();

	makeAutomaticAction(battle, next, doNothing);
}

bool BattleFlowProcessor::makeAutomaticAction(const CBattleInfoCallback & battle, const CStack *stack, BattleAction &ba)
{
	BattleSetActiveStack bsa;
	bsa.battleID = battle.getBattle()->getBattleID();
	bsa.stack = stack->unitId();
	bsa.askPlayerInterface = false;
	gameHandler->sendAndApply(&bsa);

	bool ret = owner->makeAutomaticBattleAction(battle, ba);
	return ret;
}

void BattleFlowProcessor::stackEnchantedTrigger(const CBattleInfoCallback & battle, const CStack * st)
{
	auto bl = *(st->getBonuses(Selector::type()(BonusType::ENCHANTED)));
	for(auto b : bl)
	{
		const CSpell * sp = b->subtype.as<SpellID>().toSpell();
		if(!sp)
			continue;

		const int32_t val = bl.valOfBonuses(Selector::typeSubtype(b->type, b->subtype));
		const int32_t level = ((val > 3) ? (val - 3) : val);

		spells::BattleCast battleCast(&battle, st, spells::Mode::PASSIVE, sp);
		//this makes effect accumulate for at most 50 turns by default, but effect may be permanent and last till the end of battle
		battleCast.setEffectDuration(50);
		battleCast.setSpellLevel(level);
		spells::Target target;

		if(val > 3)
		{
			for(auto s : battle.battleGetAllStacks())
				if(battle.battleMatchOwner(st, s, true) && s->isValidTarget()) //all allied
					target.emplace_back(s);
		}
		else
		{
			target.emplace_back(st);
		}
		battleCast.applyEffects(gameHandler->spellEnv, target, false, true);
	}
}

void BattleFlowProcessor::removeObstacle(const CBattleInfoCallback & battle, const CObstacleInstance & obstacle)
{
	BattleObstaclesChanged obsRem;
	obsRem.battleID = battle.getBattle()->getBattleID();
	obsRem.changes.emplace_back(obstacle.uniqueID, ObstacleChanges::EOperation::REMOVE);
	gameHandler->sendAndApply(&obsRem);
}

void BattleFlowProcessor::stackTurnTrigger(const CBattleInfoCallback & battle, const CStack *st)
{
	BattleTriggerEffect bte;
	bte.battleID = battle.getBattle()->getBattleID();
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
			auto adjacent = battle.battleAdjacentUnits(st);

			for (auto b : bl)
			{
				if(b->additionalInfo != CAddInfo::NONE)
				{
					const CStack * stack = battle.battleGetStackByID(b->additionalInfo[0]); //binding stack must be alive and adjacent
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
				ssp.battleID = battle.getBattle()->getBattleID();
				ssp.which = BattleSetStackProperty::UNBIND;
				ssp.stackID = st->unitId();
				gameHandler->sendAndApply(&ssp);
			}
		}

		if (st->hasBonusOfType(BonusType::POISON))
		{
			std::shared_ptr<const Bonus> b = st->getBonusLocalFirst(Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(SpellID(SpellID::POISON))).And(Selector::type()(BonusType::STACK_HEALTH)));
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
			const PlayerColor opponent = battle.otherPlayer(battle.battleGetOwner(st));
			const CGHeroInstance * opponentHero = battle.battleGetFightingHero(opponent);
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
			for (const CStack * stack : battle.battleGetAllStacks(true))
			{
				if (battle.battleMatchOwner(st, stack) && stack->alive() && stack->hasBonusOfType(BonusType::FEAR))
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
		int side = *battle.playerToSide(st->unitOwner());
		if(st->canCast() && battle.battleGetEnchanterCounter(side) == 0)
		{
			bool cast = false;
			while(!bl.empty() && !cast)
			{
				auto bonus = *RandomGeneratorUtil::nextItem(bl, gameHandler->getRandomGenerator());
				auto spellID = bonus->subtype.as<SpellID>();
				const CSpell * spell = SpellID(spellID).toSpell();
				bl.remove_if([&bonus](const Bonus * b)
				{
					return b == bonus.get();
				});
				spells::BattleCast parameters(&battle, st, spells::Mode::ENCHANTER, spell);
				parameters.setSpellLevel(bonus->val);
				parameters.massive = true;
				parameters.smart = true;
				//todo: recheck effect level
				if(parameters.castIfPossible(gameHandler->spellEnv, spells::Target(1, spells::Destination())))
				{
					cast = true;

					int cooldown = bonus->additionalInfo[0];
					BattleSetStackProperty ssp;
					ssp.battleID = battle.getBattle()->getBattleID();
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

void BattleFlowProcessor::setActiveStack(const CBattleInfoCallback & battle, const battle::Unit * stack)
{
	assert(stack);

	BattleSetActiveStack sas;
	sas.battleID = battle.getBattle()->getBattleID();
	sas.stack = stack->unitId();
	gameHandler->sendAndApply(&sas);
}
