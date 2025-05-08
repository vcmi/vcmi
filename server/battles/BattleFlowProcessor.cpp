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
#include "../TurnTimerHandler.h"

#include "../../lib/CStack.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/battle/IBattleState.h"
#include "../../lib/entities/building/TownFortifications.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/mapObjects/CGTownInstance.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/spells/BonusCaster.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/spells/ObstacleCasterProxy.h"

#include <vstd/RNG.h>

BattleFlowProcessor::BattleFlowProcessor(BattleProcessor * owner, CGameHandler * newGameHandler)
	: owner(owner)
	, gameHandler(newGameHandler)
{
}

void BattleFlowProcessor::summonGuardiansHelper(const CBattleInfoCallback & battle, BattleHexArray & output, const BattleHex & targetPosition, BattleSide side, bool targetIsTwoHex) //return hexes for summoning two hex monsters in output, target = unit to guard
{
	int x = targetPosition.getX();
	int y = targetPosition.getY();

	const bool targetIsAttacker = side == BattleSide::ATTACKER;

	if (targetIsAttacker) //handle front guardians, TODO: should we handle situation when units start battle near opposite side of the battlefield? Cannot happen in normal H3...
		output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::RIGHT, false));
	else
		output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::LEFT, false));

	//guardian spawn locations for four default position cases for attacker and defender, non-default starting location for att and def is handled in first two if's
	if (targetIsAttacker && ((y % 2 == 0) || (x > 1)))
	{
		if (targetIsTwoHex && (y % 2 == 1) && (x == 2)) //handle exceptional case
		{
			output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::TOP_RIGHT, false));
			output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::BOTTOM_RIGHT, false));
		}
		else
		{	//add back-side guardians for two-hex target, side guardians for one-hex
			output.checkAndPush(targetPosition.cloneInDirection(targetIsTwoHex ? BattleHex::EDir::TOP_LEFT : BattleHex::EDir::TOP_RIGHT, false));
			output.checkAndPush(targetPosition.cloneInDirection(targetIsTwoHex ? BattleHex::EDir::BOTTOM_LEFT : BattleHex::EDir::BOTTOM_RIGHT, false));

			if (!targetIsTwoHex && x > 2) //back guard for one-hex
				output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false));
			else if (targetIsTwoHex)//front-side guardians for two-hex target
			{
				output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::TOP_RIGHT, false));
				output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::BOTTOM_RIGHT, false));
				if (x > 3) //back guard for two-hex
					output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::LEFT, false));
			}
		}

	}

	else if (!targetIsAttacker && ((y % 2 == 1) || (x < GameConstants::BFIELD_WIDTH - 2)))
	{
		if (targetIsTwoHex && (y % 2 == 0) && (x == GameConstants::BFIELD_WIDTH - 3)) //handle exceptional case... equivalent for above for defender side
		{
			output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::TOP_LEFT, false));
			output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false));
		}
		else
		{
			output.checkAndPush(targetPosition.cloneInDirection(targetIsTwoHex ? BattleHex::EDir::TOP_RIGHT : BattleHex::EDir::TOP_LEFT, false));
			output.checkAndPush(targetPosition.cloneInDirection(targetIsTwoHex ? BattleHex::EDir::BOTTOM_RIGHT : BattleHex::EDir::BOTTOM_LEFT, false));

			if (!targetIsTwoHex && x < GameConstants::BFIELD_WIDTH - 3)
				output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false));
			else if (targetIsTwoHex)
			{
				output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::TOP_LEFT, false));
				output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false));
				if (x < GameConstants::BFIELD_WIDTH - 4)
					output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::RIGHT, false));
			}
		}
	}

	else if (!targetIsAttacker && y % 2 == 0)
	{
		output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::TOP_LEFT, false));
		output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::LEFT, false).cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false));
	}

	else if (targetIsAttacker && y % 2 == 1)
	{
		output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::TOP_RIGHT, false));
		output.checkAndPush(targetPosition.cloneInDirection(BattleHex::EDir::RIGHT, false).cloneInDirection(BattleHex::EDir::BOTTOM_RIGHT, false));
	}
}

void BattleFlowProcessor::tryPlaceMoats(const CBattleInfoCallback & battle)
{
	const auto * town = battle.battleGetDefendedTown();

	if (!town)
		return;

	const auto & fortifications = town->fortificationsLevel();

	//Moat should be initialized here, because only here we can use spellcasting
	if (fortifications.hasMoat)
	{
		const auto * h = battle.battleGetFightingHero(BattleSide::DEFENDER);
		const auto * actualCaster = h ? static_cast<const spells::Caster*>(h) : nullptr;
		auto moatCaster = spells::SilentCaster(battle.sideToPlayer(BattleSide::DEFENDER), actualCaster);
		auto cast = spells::BattleCast(&battle, &moatCaster, spells::Mode::PASSIVE, fortifications.moatSpell.toSpell());
		auto target = spells::Target();
		cast.cast(gameHandler->spellEnv, target);
	}
}

void BattleFlowProcessor::onBattleStarted(const CBattleInfoCallback & battle)
{
	tryPlaceMoats(battle);

	gameHandler->turnTimerHandler->onBattleStart(battle.getBattle()->getBattleID());

	if (battle.battleGetTacticDist() == 0)
		onTacticsEnded(battle);
}

void BattleFlowProcessor::trySummonGuardians(const CBattleInfoCallback & battle, const CStack * stack)
{
	if (!stack->hasBonusOfType(BonusType::SUMMON_GUARDIANS))
		return;

	std::shared_ptr<const Bonus> summonInfo = stack->getBonus(Selector::type()(BonusType::SUMMON_GUARDIANS));
	auto accessibility = battle.getAccessibility();
	CreatureID creatureData = summonInfo->subtype.as<CreatureID>();
	BattleHexArray targetHexes;
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

	for(const auto & hex : targetHexes)
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
			gameHandler->sendAndApply(pack);
		}
	}

	// send empty event to client
	// temporary(?) workaround to force animations to trigger
	StacksInjured fakeEvent;
	fakeEvent.battleID = battle.getBattle()->getBattleID();
	gameHandler->sendAndApply(fakeEvent);
}

void BattleFlowProcessor::castOpeningSpells(const CBattleInfoCallback & battle)
{
	for(auto i : {BattleSide::ATTACKER, BattleSide::DEFENDER})
	{
		auto h = battle.battleGetFightingHero(i);
		if (!h)
			continue;

		TConstBonusListPtr bl = h->getBonusesOfType(BonusType::OPENING_BATTLE_SPELL);

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
	gameHandler->sendAndApply(bnr);

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
			gameHandler->sendAndApply(bte);
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
			gameHandler->sendAndApply(removeGhosts);

		gameHandler->turnTimerHandler->onBattleNextStack(battle.getBattle()->getBattleID(), *next);

		if (!tryMakeAutomaticAction(battle, next))
		{
			if(next->alive()) {
				setActiveStack(battle, next, BattleUnitTurnReason::TURN_QUEUE);
				break;
			}
		}
	}
}

bool BattleFlowProcessor::tryMakeAutomaticAction(const CBattleInfoCallback & battle, const CStack * next)
{
	// check for bad morale => freeze
	int nextStackMorale = next->moraleVal();
	if(!next->hadMorale && !next->waited() && nextStackMorale < 0)
	{
		auto badMoraleChanceVector = gameHandler->getSettings().getVector(EGameSettings::COMBAT_BAD_MORALE_CHANCE);
		int moraleDiceSize = gameHandler->getSettings().getInteger(EGameSettings::COMBAT_MORALE_DICE_SIZE);
		size_t chanceIndex = std::min<size_t>(badMoraleChanceVector.size(), -nextStackMorale) - 1; // array index, so 0-indexed

		if(badMoraleChanceVector.size() > 0 && gameHandler->getRandomGenerator().nextInt(1, moraleDiceSize) <= badMoraleChanceVector[chanceIndex])
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

		// TODO: unify logic with AI?
		// Find best target using logic similar to H3 AI

		const auto & isBetterTarget = [&battle](const battle::Unit * candidate, const battle::Unit * current)
		{
			bool candidateInsideWalls = battle.battleIsInsideWalls(candidate->getPosition());
			bool currentInsideWalls = battle.battleIsInsideWalls(current->getPosition());

			if (candidateInsideWalls != currentInsideWalls)
				return candidateInsideWalls > currentInsideWalls;

			// also check for war machines - shooters are more dangerous than war machines, ballista or catapult
			bool candidateCanShoot = candidate->canShoot() && candidate->unitType()->warMachine == ArtifactID::NONE;
			bool currentCanShoot = current->canShoot() && current->unitType()->warMachine == ArtifactID::NONE;

			if (candidateCanShoot != currentCanShoot)
				return candidateCanShoot > currentCanShoot;

			int64_t candidateTargetValue = static_cast<int64_t>(candidate->unitType()->getAIValue() * candidate->getCount());
			int64_t currentTargetValue = static_cast<int64_t>(current->unitType()->getAIValue() * current->getCount());

			return candidateTargetValue > currentTargetValue;
		};

		const battle::Unit * target = nullptr;

		for(auto & elem : battle.battleGetAllStacks(true))
		{
			if (elem->unitOwner() == next->unitOwner())
				continue;

			if (!elem->isValidTarget())
				continue;

			if (!battle.battleCanShoot(next, elem->getPosition()))
				continue;

			if (target && !isBetterTarget(elem, target))
				continue;

			target = elem;
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
		auto goodMoraleChanceVector = gameHandler->getSettings().getVector(EGameSettings::COMBAT_GOOD_MORALE_CHANCE);
		int moraleDiceSize = gameHandler->getSettings().getInteger(EGameSettings::COMBAT_MORALE_DICE_SIZE);
		size_t chanceIndex = std::min<size_t>(goodMoraleChanceVector.size(), nextStackMorale) - 1; // array index, so 0-indexed

		if(goodMoraleChanceVector.size() > 0 && gameHandler->getRandomGenerator().nextInt(1, moraleDiceSize) <= goodMoraleChanceVector[chanceIndex])
		{
			BattleTriggerEffect bte;
			bte.battleID = battle.getBattle()->getBattleID();
			bte.stackID = next->unitId();
			bte.effect = vstd::to_underlying(BonusType::MORALE);
			bte.val = 1;
			bte.additionalInfo = 0;
			gameHandler->sendAndApply(bte); //play animation
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

	// creature will not skip the turn after casting a spell if spell uses canCastWithoutSkip
	if(ba.actionType == EActionType::MONSTER_SPELL)
	{
		assert(activeStack != nullptr);
		assert(actedStack != nullptr);

		// NOTE: in case of random spellcaster, (e.g. Master Genie) spell has been selected by server and was not present in action received from player
		if(actedStack->castSpellThisTurn && ba.spell.hasValue() && ba.spell.toSpell()->canCastWithoutSkip())
		{
			setActiveStack(battle, actedStack, BattleUnitTurnReason::UNIT_SPELLCAST);
			return;
		}
	}

	if (ba.isUnitAction())
	{
		assert(activeStack != nullptr);
		assert(actedStack != nullptr);

		if (rollGoodMorale(battle, actedStack))
		{
			// Good morale - same stack makes 2nd turn
			setActiveStack(battle, actedStack, BattleUnitTurnReason::MORALE);
			return;
		}
	}
	else
	{
		if (activeStack && activeStack->alive())
		{
			// this is action made by hero AND unit is alive (e.g. not killed by casted spell)
			// keep current active stack for next action
			setActiveStack(battle, activeStack, BattleUnitTurnReason::HERO_SPELLCAST);
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
	bsa.reason = BattleUnitTurnReason::AUTOMATIC_ACTION;
	gameHandler->sendAndApply(bsa);

	bool ret = owner->makeAutomaticBattleAction(battle, ba);
	return ret;
}

void BattleFlowProcessor::stackEnchantedTrigger(const CBattleInfoCallback & battle, const CStack * st)
{
	auto bl = *(st->getBonusesOfType(BonusType::ENCHANTED));
	for(auto b : bl)
	{
		if (!b->subtype.as<SpellID>().hasValue())
			continue;

		const CSpell * sp = b->subtype.as<SpellID>().toSpell();
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
	gameHandler->sendAndApply(obsRem);
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
		if (st->hasBonusOfType(BonusType::BIND_EFFECT))
		{
			bool unbind = true;
			BonusList bl = *(st->getBonusesOfType(BonusType::BIND_EFFECT));
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
				gameHandler->sendAndApply(ssp);
			}
		}

		if (st->hasBonusOfType(BonusType::POISON))
		{
			std::shared_ptr<const Bonus> b = st->getFirstBonus(Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(SpellID(SpellID::POISON))).And(Selector::type()(BonusType::STACK_HEALTH)));
			if (b) //TODO: what if not?...
			{
				bte.val = std::max (b->val - 10, -(st->valOfBonuses(BonusType::POISON)));
				if (bte.val < b->val) //(negative) poison effect increases - update it
				{
					bte.effect = vstd::to_underlying(BonusType::POISON);
					gameHandler->sendAndApply(bte);
				}
			}
		}
		if(st->hasBonusOfType(BonusType::MANA_DRAIN) && !st->drainedMana)
		{
			const CGHeroInstance * opponentHero = battle.battleGetFightingHero(battle.otherSide(st->unitSide()));
			if(opponentHero)
			{
				ui32 manaDrained = st->valOfBonuses(BonusType::MANA_DRAIN);
				vstd::amin(manaDrained, opponentHero->mana);
				if(manaDrained)
				{
					bte.effect = vstd::to_underlying(BonusType::MANA_DRAIN);
					bte.val = manaDrained;
					bte.additionalInfo = opponentHero->id.getNum(); //for sanity
					gameHandler->sendAndApply(bte);
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
					gameHandler->sendAndApply(bte);
				}
			}
		}
		BonusList bl = *(st->getBonuses(Selector::type()(BonusType::ENCHANTER)));
		bl.remove_if([](const Bonus * b)
		{
			return b->subtype.as<SpellID>() == SpellID::NONE;
		});

		BattleSide side = battle.playerToSide(st->unitOwner());
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

				auto &levelInfo = spell->getLevelInfo(bonus->val);
				bool isDamageSpell = spell->isDamage() || spell->isOffensive();
				if (!isDamageSpell || levelInfo.smartTarget || !levelInfo.range.empty())
				{
					parameters.massive = true;
					parameters.smart = true;
				}
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
					gameHandler->sendAndApply(ssp);
				}
			}
		}
	}
}

void BattleFlowProcessor::setActiveStack(const CBattleInfoCallback & battle, const battle::Unit * stack, BattleUnitTurnReason reason)
{
	assert(stack);

	BattleSetActiveStack sas;
	sas.battleID = battle.getBattle()->getBattleID();
	sas.stack = stack->unitId();
	sas.reason = reason;
	gameHandler->sendAndApply(sas);
}
