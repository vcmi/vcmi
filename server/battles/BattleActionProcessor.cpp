/*
 * BattleActionProcessor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleActionProcessor.h"

#include "BattleProcessor.h"

#include "../CGameHandler.h"

#include "../../lib/CStack.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/battle/CObstacleInstance.h"
#include "../../lib/battle/IBattleState.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/bonuses/BonusParameters.h"
#include "../../lib/scripting/ScriptService.h"
#include "../../lib/combatScripts/ICombatEventScript.h"
#include "../../lib/callback/IGameInfoCallback.h"
#include "../../lib/callback/GameRandomizer.h"
#include "../../lib/entities/building/TownFortifications.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/networkPacks/SetStackEffect.h"
#include "../../lib/spells/AbilityCaster.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/spells/Problem.h"
#include "../../lib/spells/CSpell.h"

#include <vstd/RNG.h>

/// What a script reacting before an attack learns about a unit that is about to be hit. The damage
/// is not rolled yet, so only the identity of the unit and the health it has left are known.
static AttackedTarget unitAboutToBeAttacked(const battle::Unit * unit)
{
	AttackedTarget target;
	target.unit = unit;
	target.healthBeforeAttack = unit->getAvailableHealth();
	return target;
}

BattleActionProcessor::BattleActionProcessor(BattleProcessor * owner, CGameHandler * newGameHandler)
	: owner(owner)
	, gameHandler(newGameHandler)
{
}

bool BattleActionProcessor::doEmptyAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	return true;
}

bool BattleActionProcessor::doEndTacticsAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	return true;
}

bool BattleActionProcessor::doWaitAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	const CStack * stack = battle.battleGetStackByID(ba.stackNumber);

	if (!canStackAct(battle, stack))
		return false;

	processBattleEventTriggers(battle, CombatEventType::WAIT, stack, nullptr);
	return true;
}

bool BattleActionProcessor::doRetreatAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	if (!battle.battleCanFlee(battle.sideToPlayer(ba.side)))
	{
		gameHandler->complain("Cannot retreat!");
		return false;
	}

	owner->setBattleResult(battle, EBattleResult::ESCAPE, battle.otherSide(ba.side));
	return true;
}

bool BattleActionProcessor::doSurrenderAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	PlayerColor player = battle.sideToPlayer(ba.side);
	int cost = battle.battleGetSurrenderCost(player);
	if (cost < 0)
	{
		gameHandler->complain("Cannot surrender!");
		return false;
	}

	if (gameHandler->gameInfo().getResource(player, EGameResID::GOLD) < cost)
	{
		gameHandler->complain("Not enough gold to surrender!");
		return false;
	}

	gameHandler->giveResource(player, EGameResID::GOLD, -cost);
	owner->setBattleResult(battle, EBattleResult::SURRENDER, battle.otherSide(ba.side));
	return true;
}

bool BattleActionProcessor::doHeroSpellAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	const CGHeroInstance *h = battle.battleGetFightingHero(ba.side);
	if (!h)
	{
		logGlobal->error("Wrong caster!");
		return false;
	}

	if (!ba.spell.hasValue())
	{
		logGlobal->error("Wrong spell id (%d)!", ba.spell.getNum());
		return false;
	}

	const CSpell * s = ba.spell.toSpell();
	spells::BattleCast parameters(&battle, h, spells::Mode::HERO, s);

	spells::detail::ProblemImpl problem;

	auto m = s->battleMechanics(&parameters);

	if(!m->canBeCast(problem))//todo: should we check aimed cast?
	{
		logGlobal->warn("Spell cannot be cast!");
		std::vector<std::string> texts;
		problem.getAll(texts);
		for(const auto & text : texts)
			logGlobal->warn(text);
		return false;
	}

	parameters.cast(gameHandler->spellcastEnvironment(), ba.getTarget(&battle));
	gameHandler->useChargeBasedSpell(h->id, ba.spell);

	return true;
}

bool BattleActionProcessor::doWalkAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	const CStack * stack = battle.battleGetStackByID(ba.stackNumber);
	battle::Target target = ba.getTarget(&battle);

	if (!canStackAct(battle, stack))
		return false;

	if(target.empty())
	{
		gameHandler->complain("Destination required for move action.");
		return false;
	}

	processBattleEventTriggers(battle, CombatEventType::BEFORE_MOVE, stack, nullptr);

	auto movementResult = moveStack(battle, ba.stackNumber, target.at(0).hexValue); //move
	if (movementResult.invalidRequest)
	{
		gameHandler->complain("Stack failed movement!");
		return false;
	}
	processBattleEventTriggers(battle, CombatEventType::AFTER_MOVE, stack, nullptr);
	return true;
}

bool BattleActionProcessor::doDefendAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	const CStack * stack = battle.battleGetStackByID(ba.stackNumber);

	if (!canStackAct(battle, stack))
		return false;

	//defensive stance, TODO: filter out spell boosts from bonus (stone skin etc.)
	SetStackEffect sse;
	sse.battleID = battle.getBattle()->getBattleID();

	Bonus defenseBonusToAdd(BonusDuration::STACK_GETS_TURN, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 20, BonusSourceID(), BonusSubtypeID(PrimarySkill::DEFENSE), BonusValueType::PERCENT_TO_ALL);
	Bonus bonus2(BonusDuration::STACK_GETS_TURN, BonusType::PRIMARY_SKILL, BonusSource::OTHER, stack->valOfBonuses(BonusType::DEFENSIVE_STANCE), BonusSourceID(), BonusSubtypeID(PrimarySkill::DEFENSE), BonusValueType::ADDITIVE_VALUE);
	Bonus alternativeWeakCreatureBonus(BonusDuration::STACK_GETS_TURN, BonusType::PRIMARY_SKILL, BonusSource::OTHER, 1, BonusSourceID(), BonusSubtypeID(PrimarySkill::DEFENSE), BonusValueType::ADDITIVE_VALUE);
	Bonus tagBonus(BonusDuration::STACK_GETS_TURN, BonusType::UNIT_DEFENDING, BonusSource::OTHER, 0, BonusSourceID());

	BonusList defence = *stack->getBonuses(Selector::typeSubtype(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::DEFENSE)));
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
	buffer.push_back(tagBonus);

	sse.toUpdate.emplace_back(ba.stackNumber, buffer);
	gameHandler->sendAndApply(sse);

	BattleLogMessage message;
	message.battleID = battle.getBattle()->getBattleID();

	MetaString text;
	stack->addText(text, EMetaText::GENERAL_TXT, 120);
	stack->addNameReplacement(text);
	text.replaceNumber(difference);

	message.lines.push_back(text);

	gameHandler->sendAndApply(message);

	processBattleEventTriggers(battle, CombatEventType::DEFEND, stack, nullptr);
	return true;
}

bool BattleActionProcessor::doAttackAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	const CStack * stack = battle.battleGetStackByID(ba.stackNumber);
	battle::Target target = ba.getTarget(&battle);

	if (!canStackAct(battle, stack))
		return false;

	if(target.size() < 2)
	{
		gameHandler->complain("Two destinations required for attack action.");
		return false;
	}

	BattleHex attackPos = target.at(0).hexValue;
	BattleHex destinationTile = target.at(1).hexValue;
	const CStack * destinationStack = battle.battleGetStackByPos(destinationTile, true);

	if(!destinationStack)
	{
		gameHandler->complain("Invalid target to attack");
		return false;
	}

	BattleHex startingPos = stack->getPosition();
	int beforeAttackSpeed = stack->getMovementRange(0);
	const auto movementResult = moveStack(battle, ba.stackNumber, attackPos);

	logGlobal->trace("%s will attack %s", stack->nodeName(), destinationStack->nodeName());

	if (movementResult.invalidRequest)
	{
		gameHandler->complain("Stack failed attack - unable to reach target!");
		return false;
	}

	if(movementResult.obstacleHit)
	{
		// we were not able to reach destination tile, nor occupy specified hex
		// abort attack attempt, but treat this case as legal - we have stepped onto a quicksands/mine
		return true;
	}

	if(destinationStack && stack->unitId() == destinationStack->unitId()) //we should just move, it will be handled by following check
	{
		destinationStack = nullptr;
	}

	if(!destinationStack)
	{
		gameHandler->complain("Unit can not attack itself");
		return false;
	}

	const bool regularMeleeAttack = battle.isMeleeAttackPossible(stack, destinationStack);
	const bool longWeaponAttack = battle.isLongWeaponAttack(stack, destinationStack);

	if(!regularMeleeAttack && !longWeaponAttack)
	{
		gameHandler->complain("Attack cannot be performed!");
		return false;
	}

	//attack
	int totalAttacks = stack->getTotalAttacks(false);

	//TODO: move to CUnitState
	const auto * attackingHero = battle.battleGetFightingHero(ba.side);
	if(attackingHero)
	{
		totalAttacks += attackingHero->valOfBonuses(BonusType::HERO_GRANTS_ATTACKS, BonusSubtypeID(stack->creatureId()));
	}

	static const auto firstStrikeSelector = Selector::typeSubtype(BonusType::FIRST_STRIKE, BonusCustomSubtype::damageTypeAll).Or(Selector::typeSubtype(BonusType::FIRST_STRIKE, BonusCustomSubtype::damageTypeMelee));
	const bool firstStrike = destinationStack->hasBonus(firstStrikeSelector) && !destinationStack->hasBonusOfType(BonusType::NOT_ACTIVE);

	bool ferocityApplied = false;
	int32_t defenderInitialQuantity = destinationStack->getCount();

	BonusList attackerBonusesToRemove = *stack->getAllBonuses(Bonus::untilAfterAttackSequence);	//they need to be gathered here since bonuses with this duration added during attack (like blind) should not be removed
	BonusList defenderBonusesToRemove = *destinationStack->getAllBonuses(Bonus::untilAfterAttackSequence);

	for (int i = 0; i < totalAttacks; ++i)
	{
		//first strike
		if(i == 0 && firstStrike && destinationStack->ableToRetaliate() && !stack->hasBonusOfType(BonusType::BLOCKS_RETALIATION) && !stack->isInvincible() && !longWeaponAttack)
		{
			makeAttack(battle, destinationStack, stack, {.targetHex = stack->getPosition(), .first = true, .counter = true});
		}

		//move can cause death, eg. by walking into the moat, first strike can cause death or paralysis/petrification
		if(stack->alive() && !stack->hasBonusOfType(BonusType::NOT_ACTIVE) && destinationStack->alive())
		{
			//no distance travelled on second attack
			makeAttack(battle, stack, destinationStack, {.targetHex = destinationTile, .distance = (i ? 0 : movementResult.distance), .attackIndex = i, .first = i == 0});

			if(!ferocityApplied && stack->hasBonusOfType(BonusType::FEROCITY))
			{
				auto ferocityBonus = stack->getBonus(Selector::type()(BonusType::FEROCITY));
				int32_t requiredCreaturesToKill = ferocityBonus->parameters ? ferocityBonus->parameters->toNumber() : 1;
				if(defenderInitialQuantity - destinationStack->getCount() >= requiredCreaturesToKill)
				{
					ferocityApplied = true;
					int additionalAttacksCount = stack->valOfBonuses(BonusType::FEROCITY);
					totalAttacks += additionalAttacksCount;
				}
			}
		}

		//counterattack
		//we check retaliation twice, so if it unblocked during attack it will work only on next attack
		if(stack->alive()
			&& !stack->hasBonusOfType(BonusType::BLOCKS_RETALIATION)
			&& !stack->isInvincible()
			&& !longWeaponAttack
			&& (i == 0 && !firstStrike)
			&& destinationStack->ableToRetaliate())
		{
			makeAttack(battle, destinationStack, stack, {.targetHex = stack->getPosition(), .first = true, .counter = true});
		}
	}

	//return
	if(stack->hasBonusOfType(BonusType::RETURN_AFTER_STRIKE)
		&& !stack->hasBonusOfType(BonusType::NOT_ACTIVE)
		&& !stack->hasBonusOfType(BonusType::BIND_EFFECT)
		&& target.size() == 3
		&& startingPos != stack->getPosition()
		&& startingPos == target.at(2).hexValue
		&& stack->alive())
	{
		assert(stack->unitId() == ba.stackNumber);
		int afterAttackSpeed = stack->getMovementRange(0);
		std::pair<BattleHexArray, int> path = battle.getPath(stack->getPosition(), startingPos, stack);
		size_t maxReachbleIndex = std::max(0, beforeAttackSpeed - afterAttackSpeed);
		if(maxReachbleIndex < path.first.size())
			moveStack(battle, ba.stackNumber, path.first[maxReachbleIndex]);
	}

	removeBonuses(battle, stack, attackerBonusesToRemove);
	removeBonuses(battle, destinationStack, defenderBonusesToRemove);

	// attacking without moving still triggers the obstacle the unit stands on (e.g. moat damage);
	// units that moved into the obstacle were already charged during the movement above
	if(movementResult.distance == 0)
		battle.handleObstacleTriggersForUnit(*gameHandler->spellEnv, *stack);

	return true;
}

void BattleActionProcessor::removeBonuses(const CBattleInfoCallback & battle, const battle::Unit * stack, BonusList bonuses)
{
	if (!stack)
	{
		logGlobal->error("Attempt at removing bonuses from nullptr!");
		return;
	}	

	if (bonuses.empty())
		return;

	SetStackEffect sse;
	sse.battleID = battle.getBattle()->getBattleID();
	std::vector<Bonus> buffer;
	for (const auto & bonus : bonuses)
		buffer.push_back(*bonus);
	sse.toRemove.emplace_back(stack->unitId(), buffer);

	gameHandler->sendAndApply(sse);
}

bool BattleActionProcessor::doShootAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	const CStack * stack = battle.battleGetStackByID(ba.stackNumber);
	battle::Target target = ba.getTarget(&battle);

	if (!canStackAct(battle, stack))
		return false;

	if(target.empty())
	{
		gameHandler->complain("Destination required for shot action.");
		return false;
	}

	auto destination = target.at(0).hexValue;

	const CStack * destinationStack = battle.battleGetStackByPos(destination);

	if (!battle.battleCanShoot(stack, destination))
	{
		gameHandler->complain("Cannot shoot!");
		return false;
	}

	const bool emptyTileAreaAttack = battle.battleCanTargetEmptyHex(stack);

	if (!destinationStack && !emptyTileAreaAttack)
	{
		gameHandler->complain("No target to shoot!");
		return false;
	}

	bool firstStrike = false;
	if(!emptyTileAreaAttack)
	{
		static const auto firstStrikeSelector = Selector::typeSubtype(BonusType::FIRST_STRIKE, BonusCustomSubtype::damageTypeAll).Or(Selector::typeSubtype(BonusType::FIRST_STRIKE, BonusCustomSubtype::damageTypeRanged));
		firstStrike = destinationStack->hasBonus(firstStrikeSelector) && !destinationStack->hasBonusOfType(BonusType::NOT_ACTIVE);
	}

	if (!firstStrike)
		makeAttack(battle, stack, destinationStack, {.targetHex = destination, .first = true, .ranged = true});

	BonusList attackerBonusesToRemove = *stack->getAllBonuses(Bonus::untilAfterAttackSequence);	//they need to be gathered here since bonuses with this duration added during attack (like blind) should not be removed
	BonusList defenderBonusesToRemove;
	if (destinationStack)
		defenderBonusesToRemove = *destinationStack->getAllBonuses(Bonus::untilAfterAttackSequence);

	//ranged counterattack
	if (!emptyTileAreaAttack
		&& destinationStack->hasBonusOfType(BonusType::RANGED_RETALIATION)
		&& !stack->hasBonusOfType(BonusType::BLOCKS_RANGED_RETALIATION)
		&& destinationStack->ableToRetaliate()
		&& battle.battleCanShoot(destinationStack, stack->getPosition())
		&& stack->alive()) //attacker may have died (fire shield)
	{
		makeAttack(battle, destinationStack, stack, {.targetHex = stack->getPosition(), .first = true, .ranged = true, .counter = true});
	}
	//allow more than one additional attack

	int totalRangedAttacks = stack->getTotalAttacks(true);

	//TODO: move to CUnitState
	const auto * attackingHero = battle.battleGetFightingHero(ba.side);
	if(attackingHero)
	{
		totalRangedAttacks += attackingHero->valOfBonuses(BonusType::HERO_GRANTS_ATTACKS, BonusSubtypeID(stack->creatureId()));
	}

	for(int i = firstStrike ? 0:1; i < totalRangedAttacks; ++i)
	{
		if(stack->alive()
			&& (emptyTileAreaAttack || destinationStack->alive())
			&& stack->shots.canUse())
		{
			// when the defender strikes first the opening shot above is skipped and this loop makes
			// it instead, so the shot that abilities fire on is the first one this loop makes
			makeAttack(battle, stack, destinationStack, {.targetHex = destination, .attackIndex = i, .first = i == 0, .ranged = true});
		}
	}

	removeBonuses(battle, stack, attackerBonusesToRemove);
	removeBonuses(battle, destinationStack, defenderBonusesToRemove);

	return true;
}

bool BattleActionProcessor::doCatapultAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	const CStack * stack = battle.battleGetStackByID(ba.stackNumber);
	battle::Target target = ba.getTarget(&battle);

	if (!canStackAct(battle, stack))
		return false;

	std::shared_ptr<const Bonus> catapultAbility = stack->getFirstBonus(Selector::type()(BonusType::CATAPULT));
	if(!catapultAbility || catapultAbility->subtype == BonusSubtypeID())
	{
		gameHandler->complain("We do not know how to shoot :P");
	}
	else
	{
		const CSpell * spell = catapultAbility->subtype.as<SpellID>().toSpell();
		spells::BattleCast parameters(&battle, stack, spells::Mode::SPELL_LIKE_ATTACK, spell); //We can shot infinitely by catapult
		auto shotLevel = stack->valOfBonuses(Selector::typeSubtype(BonusType::CATAPULT_EXTRA_SHOTS, catapultAbility->subtype));
		parameters.setSpellLevel(shotLevel);
		parameters.cast(gameHandler->spellcastEnvironment(), target);
	}
	return true;
}

bool BattleActionProcessor::doUnitSpellAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	const CStack * stack = battle.battleGetStackByID(ba.stackNumber);
	battle::Target target = ba.getTarget(&battle);
	SpellID spellID = ba.spell;

	if (!canStackAct(battle, stack))
		return false;

	std::shared_ptr<const Bonus> randSpellcaster = stack->getBonus(Selector::type()(BonusType::RANDOM_SPELLCASTER));
	std::shared_ptr<const Bonus> spellcaster = stack->getBonus(Selector::typeSubtype(BonusType::SPELLCASTER, BonusSubtypeID(spellID)));

	if (!spellcaster && !randSpellcaster)
	{
		gameHandler->complain("That stack can't cast spells!");
		return false;
	}

	if (randSpellcaster)
	{
		if (target.size() != 1)
		{
			gameHandler->complain("Invalid target for random spellcaster!");
			return false;
		}

		const battle::Unit * subject = target[0].unitValue;
		if (target[0].unitValue == nullptr)
			subject = battle.battleGetStackByPos(target[0].hexValue, true);

		if (subject == nullptr)
		{
			gameHandler->complain("Invalid target for random spellcaster!");
			return false;
		}

		spellID = battle.getRandomBeneficialSpell(gameHandler->getRandomGenerator(), stack, subject);

		if (spellID == SpellID::NONE)
		{
			gameHandler->complain("That stack can't cast spells!");
			return false;
		}
	}

	const CSpell * spell = SpellID(spellID).toSpell();
	spells::BattleCast parameters(&battle, stack, spells::Mode::CREATURE_ACTIVE, spell);
	int32_t spellLvl = 0;
	if(spellcaster)
		vstd::amax(spellLvl, spellcaster->val);
	if(randSpellcaster)
		vstd::amax(spellLvl, randSpellcaster->val);
	parameters.setSpellLevel(spellLvl);
	parameters.cast(gameHandler->spellcastEnvironment(), target);

	processBattleEventTriggers(battle, CombatEventType::UNIT_SPELLCAST, stack, nullptr);
	return true;
}

bool BattleActionProcessor::doHealAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	const CStack * stack = battle.battleGetStackByID(ba.stackNumber);
	battle::Target target = ba.getTarget(&battle);

	if (!canStackAct(battle, stack))
		return false;

	if(target.empty())
	{
		gameHandler->complain("Destination required for heal action.");
		return false;
	}

	const battle::Unit * destStack = nullptr;
	std::shared_ptr<const Bonus> healerAbility = stack->getFirstBonus(Selector::type()(BonusType::HEALER));

	if(target.at(0).unitValue)
		destStack = target.at(0).unitValue;
	else
		destStack = battle.battleGetUnitByPos(target.at(0).hexValue);

	if(stack == nullptr || destStack == nullptr || !healerAbility || !healerAbility->subtype.hasValue())
	{
		gameHandler->complain("There is either no healer, no destination, or healer cannot heal :P");
	}
	else
	{
		const CSpell * spell = healerAbility->subtype.as<SpellID>().toSpell();
		spells::BattleCast parameters(&battle, stack, spells::Mode::SPELL_LIKE_ATTACK, spell); //We can heal infinitely by first aid tent
		auto dest = battle::Destination(destStack, target.at(0).hexValue);
		parameters.setSpellLevel(0);
		parameters.cast(gameHandler->spellcastEnvironment(), {dest});
	}
	return true;
}

bool BattleActionProcessor::doWalkAndSpellcastAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	const CStack * stack = battle.battleGetStackByID(ba.stackNumber);
	battle::Target target = ba.getTarget(&battle);
	SpellID spellID = ba.spell;

	if (!canStackAct(battle, stack))
		return false;

	if(target.size() < 2)
	{
		gameHandler->complain("Two destinations required for walk and spellcast action.");
		return false;
	}

	BattleHex movementDestinationTile = target.at(0).hexValue;
	BattleHex targetUnitTile = target.at(1).hexValue;
	const CStack * destinationStack = battle.battleGetStackByPos(targetUnitTile, false);

	if(!destinationStack)
	{
		gameHandler->complain("Invalid target for walk and spellcast");
		return false;
	}

	auto bonus = stack->getBonus(Selector::typeSubtype(BonusType::ADJACENT_SPELLCASTER, BonusSubtypeID(spellID)));
	if (!bonus)
	{
		gameHandler->complain("Creature cannot walk and spellcast.");
		return false;
	}

	const auto movementResult = moveStack(battle, ba.stackNumber, movementDestinationTile);

	if (movementResult.invalidRequest)
	{
		gameHandler->complain("Stack failed walk and spellcast - unable to reach target!");
		return false;
	}

	if(movementResult.obstacleHit)
	{
		return true;
	}

	const CSpell * spell = spellID.toSpell();
	spells::BattleCast parameters(&battle, stack, spells::Mode::CREATURE_ACTIVE, spell);
	battle::Target spellTarget;
	spellTarget.emplace_back(destinationStack);
	parameters.setSpellLevel(std::max(0, bonus->val));
	parameters.cast(gameHandler->spellcastEnvironment(), spellTarget);

	processBattleEventTriggers(battle, CombatEventType::UNIT_SPELLCAST, stack, nullptr);

	return true;
}

bool BattleActionProcessor::canStackAct(const CBattleInfoCallback & battle, const CStack * stack)
{
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

	if (battle.battleTacticDist())
	{
		if (stack && stack->unitSide() != battle.battleGetTacticsSide())
		{
			gameHandler->complain("This is not a stack of side that has tactics!");
			return false;
		}
	}
	else
	{
		if (stack != battle.battleActiveUnit())
		{
			gameHandler->complain("Action has to be about active stack!");
			return false;
		}
	}
	return true;
}

bool BattleActionProcessor::dispatchBattleAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	switch(ba.actionType)
	{
		case EActionType::BAD_MORALE:
		case EActionType::NO_ACTION:
			return doEmptyAction(battle, ba);
		case EActionType::END_TACTIC_PHASE:
			return doEndTacticsAction(battle, ba);
		case EActionType::RETREAT:
			return doRetreatAction(battle, ba);
		case EActionType::SURRENDER:
			return doSurrenderAction(battle, ba);
		case EActionType::HERO_SPELL:
			return doHeroSpellAction(battle, ba);
		case EActionType::WALK:
			return doWalkAction(battle, ba);
		case EActionType::WAIT:
			return doWaitAction(battle, ba);
		case EActionType::DEFEND:
			return doDefendAction(battle, ba);
		case EActionType::WALK_AND_ATTACK:
			return doAttackAction(battle, ba);
		case EActionType::WALK_AND_CAST:
			return doWalkAndSpellcastAction(battle, ba);
		case EActionType::SHOOT:
			return doShootAction(battle, ba);
		case EActionType::CATAPULT:
			return doCatapultAction(battle, ba);
		case EActionType::MONSTER_SPELL:
			return doUnitSpellAction(battle, ba);
		case EActionType::STACK_HEAL:
			return doHealAction(battle, ba);
	}
	gameHandler->complain("Unrecognized action type received!!");
	return false;
}

bool BattleActionProcessor::makeBattleActionImpl(const CBattleInfoCallback & battle, const BattleAction &ba)
{
	logGlobal->trace("Making action: %s", ba.toString());
	const CStack * stack = battle.battleGetStackByID(ba.stackNumber);

	// for these events client does not expects StartAction/EndAction wrapper
	if (!ba.isBattleEndAction())
	{
		StartAction startAction(ba);
		startAction.battleID = battle.getBattle()->getBattleID();
		gameHandler->sendAndApply(startAction);
	}

	bool result = dispatchBattleAction(battle, ba);

	if (!ba.isBattleEndAction())
	{
		EndAction endAction;
		endAction.battleID = battle.getBattle()->getBattleID();
		gameHandler->sendAndApply(endAction);
	}

	if(ba.actionType == EActionType::WAIT || ba.actionType == EActionType::DEFEND || ba.actionType == EActionType::SHOOT || ba.actionType == EActionType::MONSTER_SPELL)
		battle.handleObstacleTriggersForUnit(*gameHandler->spellEnv, *stack);

	return result;
}

BattleActionProcessor::MovementResult BattleActionProcessor::moveStack(const CBattleInfoCallback & battle, int stack, BattleHex dest)
{
	const CStack *currentUnit = battle.battleGetStackByID(stack);
	const CStack *stackAtEnd = battle.battleGetStackByPos(dest);

	assert(currentUnit);
	assert(dest < GameConstants::BFIELD_SIZE);

	if (battle.battleGetTacticDist())
	{
		assert(battle.isInTacticRange(dest));
	}

	auto start = currentUnit->getPosition();
	if (start == dest)
		return { 0, false, false };

	//initing necessary tables
	auto accessibility = battle.getAccessibility(currentUnit);
	BattleHexArray passed;
	//Ignore obstacles on starting position
	passed.insert(currentUnit->getPosition());
	if(currentUnit->doubleWide())
		passed.insert(currentUnit->occupiedHex());

	//shifting destination (if we have double wide stack and we can occupy dest but not be exactly there)
	if(!stackAtEnd && currentUnit->doubleWide() && !accessibility.accessible(dest, currentUnit))
	{
		BattleHex shifted = dest.cloneInDirection(currentUnit->headDirection(), false);

		if(accessibility.accessible(shifted, currentUnit))
			dest = shifted;
	}

	if((stackAtEnd && stackAtEnd!=currentUnit && stackAtEnd->alive()) || !accessibility.accessible(dest, currentUnit))
	{
		gameHandler->complain("Given destination is not accessible!");
		return { 0, false, true };
	}

	bool canUseGate = false;
	auto dbState = battle.battleGetGateState();
	if(battle.battleGetFortifications().wallsHealth > 0 && currentUnit->unitSide() == BattleSide::DEFENDER &&
		dbState != EGateState::DESTROYED &&
		dbState != EGateState::BLOCKED)
	{
		canUseGate = true;
	}

	auto [unitPath, pathDistance] = battle.getPath(start, dest, currentUnit);
	bool movementSuccess = true;

	int unitMovementRange = currentUnit->getMovementRange(0);

	if (battle.battleGetTacticDist() > 0 && unitMovementRange > 0)
		unitMovementRange = GameConstants::BFIELD_SIZE;

	if (pathDistance > unitMovementRange)
	{
		gameHandler->complain("Given destination is not reachable!");
		return { 0, false, true };
	}

	bool hasWideMoat = vstd::contains_if(battle.battleGetAllObstaclesOnPos(BattleHex(BattleHex::GATE_BRIDGE), false), [](const std::shared_ptr<const CObstacleInstance> & obst)
	{
		return obst->obstacleType == CObstacleInstance::MOAT;
	});

	auto isGateDrawbridgeHex = [&](const BattleHex & hex) -> bool
	{
		if (hasWideMoat && hex == BattleHex::GATE_BRIDGE)
			return true;
		if (hex == BattleHex::GATE_OUTER)
			return true;
		if (hex == BattleHex::GATE_INNER)
			return true;

		return false;
	};

	auto occupyGateDrawbridgeHex = [&](const BattleHex & hex) -> bool
	{
		if (isGateDrawbridgeHex(hex))
			return true;

		if (currentUnit->doubleWide())
		{
			BattleHex otherHex = currentUnit->occupiedHex(hex);
			if (otherHex.isValid() && isGateDrawbridgeHex(otherHex))
				return true;
		}

		return false;
	};

	if (currentUnit->hasBonusOfType(BonusType::FLYING))
	{
		if (pathDistance <= unitMovementRange && !unitPath.empty())
		{
			if (canUseGate && dbState != EGateState::OPENED &&
				occupyGateDrawbridgeHex(dest))
			{
				BattleUpdateGateState db;
				db.battleID = battle.getBattle()->getBattleID();
				db.state = EGateState::OPENED;
				gameHandler->sendAndApply(db);
			}

			//inform clients about move
			BattleStackMoved sm;
			sm.battleID = battle.getBattle()->getBattleID();
			sm.stack = currentUnit->unitId();
			BattleHexArray tiles;
			tiles.insert(unitPath[0]);
			sm.tilesToMove = tiles;
			sm.distance = pathDistance;
			sm.teleporting = false;
			gameHandler->sendAndApply(sm);
		}
	}
	else //for non-flying creatures
	{
		BattleHexArray tiles;
		const int tilesToMove = std::max<int>(unitPath.size() - unitMovementRange, 0);
		int movementsLeft = static_cast<int>(unitPath.size())-1;
		unitPath.insert(start);

		// check if gate need to be open or closed at some point
		BattleHex openGateAtHex;
		BattleHex gateMayCloseAtHex;
		if (canUseGate)
		{
			for (int i = static_cast<int>(unitPath.size())-1; i >= 0; i--)
			{
				auto needOpenGates = [hasWideMoat, i, unitPath = unitPath](const BattleHex & hex) -> bool
				{
					if (hasWideMoat && hex == BattleHex::GATE_BRIDGE)
						return true;
					if (hex == BattleHex::GATE_BRIDGE && i-1 >= 0 && unitPath[i-1] == BattleHex::GATE_OUTER)
						return true;
					if (hex == BattleHex::GATE_OUTER || hex == BattleHex::GATE_INNER)
						return true;

					return false;
				};

				auto hex = unitPath[i];
				if (!openGateAtHex.isValid() && dbState != EGateState::OPENED)
				{
					if (needOpenGates(hex) || needOpenGates(currentUnit->occupiedHex(hex)))
						openGateAtHex = unitPath[i+1];

					//gate may be opened and then closed during stack movement, but not other way around
					if (openGateAtHex.isValid())
						dbState = EGateState::OPENED;
				}

				if (!gateMayCloseAtHex.isValid() && dbState != EGateState::CLOSED)
				{
					if (hex == BattleHex::GATE_INNER && i-1 >= 0 && unitPath[i-1] != BattleHex::GATE_OUTER)
					{
						gateMayCloseAtHex = unitPath[i-1];
					}
					if (hasWideMoat)
					{
						if (hex == BattleHex::GATE_BRIDGE && i-1 >= 0 && unitPath[i-1] != BattleHex::GATE_OUTER)
						{
							gateMayCloseAtHex = unitPath[i-1];
						}
						else if (hex == BattleHex::GATE_OUTER && i-1 >= 0 &&
							unitPath[i-1] != BattleHex::GATE_INNER &&
							unitPath[i-1] != BattleHex::GATE_BRIDGE)
						{
							gateMayCloseAtHex = unitPath[i-1];
						}
					}
					else if (hex == BattleHex::GATE_OUTER && i-1 >= 0 && unitPath[i-1] != BattleHex::GATE_INNER)
					{
						gateMayCloseAtHex = unitPath[i-1];
					}
				}
			}
		}

		while(movementSuccess)
		{
			if (movementsLeft<tilesToMove)
				throw std::runtime_error("Movement terminated abnormally");

			bool gateStateChanging = false;
			//special handling for opening gate on from starting hex
			if (openGateAtHex.isValid() && openGateAtHex == start)
				gateStateChanging = true;
			else
			{
				for (bool obstacleHit = false; (!obstacleHit) && (!gateStateChanging) && (movementsLeft >= tilesToMove); --movementsLeft)
				{
					BattleHex hex = unitPath[movementsLeft];
					tiles.insert(hex);

					if ((openGateAtHex.isValid() && openGateAtHex == hex) ||
						(gateMayCloseAtHex.isValid() && gateMayCloseAtHex == hex))
					{
						gateStateChanging = true;
					}

					//if we walked onto something, finalize this portion of stack movement check into obstacle
					if(!battle.battleGetAllObstaclesOnPos(hex, false).empty())
						obstacleHit = true;

					if (currentUnit->doubleWide())
					{
						BattleHex otherHex = currentUnit->occupiedHex(hex);
						//two hex creature hit obstacle by backside
						auto obstacle2 = battle.battleGetAllObstaclesOnPos(otherHex, false);
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
				sm.battleID = battle.getBattle()->getBattleID();
				sm.stack = currentUnit->unitId();
				sm.distance = pathDistance;
				sm.teleporting = false;
				sm.tilesToMove = tiles;
				gameHandler->sendAndApply(sm);
				tiles.clear();
			}

			//we don't handle obstacle at the destination tile -> it's handled separately in the if at the end
			if (currentUnit->getPosition() != dest)
			{
				if(movementSuccess && start != currentUnit->getPosition())
				{
					movementSuccess &= battle.handleObstacleTriggersForUnit(*gameHandler->spellEnv, *currentUnit, passed);
					passed.insert(currentUnit->getPosition());
					if(currentUnit->doubleWide())
						passed.insert(currentUnit->occupiedHex());
				}
				if (gateStateChanging)
				{
					if (currentUnit->getPosition() == openGateAtHex)
					{
						openGateAtHex = BattleHex();
						//only open gate if stack is still alive
						if (currentUnit->alive())
						{
							BattleUpdateGateState db;
							db.battleID = battle.getBattle()->getBattleID();
							db.state = EGateState::OPENED;
							gameHandler->sendAndApply(db);
						}
					}
					else if (currentUnit->getPosition() == gateMayCloseAtHex)
					{
						gateMayCloseAtHex = BattleHex();
						owner->updateGateState(battle);
					}
				}
			}
			else
			{
				//movement finished normally: we reached destination
				break;
			}
		}
	}
	//handle last hex separately for deviation
	if (gameHandler->gameInfo().getSettings().getBoolean(EGameSettings::COMBAT_ONE_HEX_TRIGGERS_OBSTACLES))
	{
		if (dest == battle::Unit::occupiedHex(start, currentUnit->doubleWide(), currentUnit->unitSide())
			|| start == battle::Unit::occupiedHex(dest, currentUnit->doubleWide(), currentUnit->unitSide()))
			passed.clear(); //Just empty passed, obstacles will handled automatically
	}
	if(dest == start) 	//If dest is equal to start, then we should handle obstacles for it anyway
		passed.clear();	//Just empty passed, obstacles will handled automatically
	//handling obstacle on the final field (separate, because it affects both flying and walking stacks)
	movementSuccess &= battle.handleObstacleTriggersForUnit(*gameHandler->spellEnv, *currentUnit, passed);

	return { static_cast<int16_t>(pathDistance), !movementSuccess, false };
}

void BattleActionProcessor::rollAttackFlags(const CBattleInfoCallback & battle, const CStack * attacker, BattleAttack & bat) const
{
	const int attackerLuck = attacker->luckVal();
	ObjectInstanceID ownerArmy = battle.getBattle()->getSideArmy(attacker->unitSide())->id;

	if(attackerLuck > 0 && gameHandler->randomizer->rollGoodLuck(ownerArmy, attackerLuck))
		bat.flags |= BattleAttack::LUCKY;

	if(attackerLuck < 0 && gameHandler->randomizer->rollBadLuck(ownerArmy, -attackerLuck))
		bat.flags |= BattleAttack::UNLUCKY;

	if (gameHandler->randomizer->rollCombatAbility(ownerArmy, attacker->valOfBonuses(BonusType::DOUBLE_DAMAGE_CHANCE)))
		bat.flags |= BattleAttack::DEATH_BLOW;

	const auto * ownerHero = battle.battleGetFightingHero(attacker->unitSide());
	if(ownerHero)
	{
		int chance = ownerHero->valOfBonuses(BonusType::BONUS_DAMAGE_CHANCE, BonusSubtypeID(attacker->creatureId()));
		if (gameHandler->randomizer->rollCombatAbility(ownerArmy, chance))
			bat.flags |= BattleAttack::BALLISTA_DOUBLE_DMG;
	}
}

void BattleActionProcessor::describeUpcomingAttack(CombatEventPayload & payload, const CStack * defender, const battle::Units & secondaryTargets) const
{
	if(defender && defender->alive())
		payload.targets.push_back(unitAboutToBeAttacked(defender));

	for(const auto * unit : secondaryTargets)
		payload.targets.push_back(unitAboutToBeAttacked(unit));
}

battle::Units BattleActionProcessor::collectSecondaryTargets(const CBattleInfoCallback & battle, const CStack * attacker, const CStack * defender, const AttackDescriptor & attack, BattleAttack & bat) const
{
	battle::Units result;

	const auto & addOnce = [&result, defender](const battle::Unit * unit)
	{
		if(unit != defender && unit->alive() && !vstd::contains(result, unit)) //do not hit same unit twice
			result.push_back(unit);
	};

	//multiple-hex normal attack
	const auto & [attackedCreatures, useCustomAnimation] = battle.getAttackedCreatures(attacker, attack.targetHex, attack.ranged);

	for(const auto * unit : attackedCreatures)
		addOnce(unit);

	if (useCustomAnimation)
		bat.flags |= BattleAttack::CUSTOM_ANIMATION;

	std::shared_ptr<const Bonus> bonus = attacker->getBonus(Selector::type()(BonusType::SPELL_LIKE_ATTACK));

	if(!bonus || !attack.ranged || !bonus->subtype.hasValue()) //TODO: make it work in melee?
		return result;

	//this is need for displaying hit animation
	bat.flags |= BattleAttack::SPELL_LIKE;
	bat.spellID = bonus->subtype.as<SpellID>();

	//TODO: should spell override creature`s projectile?

	const auto * spell = bat.spellID.toSpell();

	battle::Target target;
	target.emplace_back(defender, attack.targetHex);

	spells::BattleCast event(&battle, attacker, spells::Mode::SPELL_LIKE_ATTACK, spell);
	event.setSpellLevel(bonus->val);

	//TODO: get exact attacked hex for defender

	for(const CStack * stack : spell->battleMechanics(&event)->getAffectedStacks(target))
		addOnce(stack);

	return result;
}

void BattleActionProcessor::markSpellLikeAttack(const CStack * attacker, BattleAttack & bat) const
{
	if(!bat.spellLike())
		return;

	//now add effect info for all attacked stacks
	for (BattleStackAttacked & bsa : bat.bsa)
	{
		if (bsa.attackerID == attacker->unitId()) //this is our attack and not f.e. fire shield
		{
			//this is need for displaying affect animation
			bsa.flags |= BattleStackAttacked::SPELL_EFFECT;
			bsa.spellID = bat.spellID;
		}
	}
}

void BattleActionProcessor::makeAttack(const CBattleInfoCallback & battle, const CStack * attacker, const CStack * defender, const AttackDescriptor & attack)
{
	// spell-casting abilities keep their own rule - once per attack action and never on a counter.
	// Combat event reactions are notified before every attack instead, further below
	if(defender && attack.first && !attack.counter)
		attackCasting(battle, attack.ranged, BonusType::SPELL_BEFORE_ATTACK, attacker, defender);

	// If the attacker or defender is not alive before the attack action, the action should be skipped.
	if((!attacker->alive()) || (defender && !defender->alive()))
		return;

	BattleAttack bat;
	BattleLogMessage blm;
	blm.battleID = battle.getBattle()->getBattleID();
	bat.battleID = battle.getBattle()->getBattleID();
	bat.attackerChanges.battleID = battle.getBattle()->getBattleID();
	bat.stackAttacking = attacker->unitId();
	bat.tile = attack.targetHex;

	if(attack.ranged)
		bat.flags |= BattleAttack::SHOT;
	if(attack.counter)
		bat.flags |= BattleAttack::COUNTER;

	// the same units feed the notification below and the damage further down
	const battle::Units secondaryTargets = collectSecondaryTargets(battle, attacker, defender, attack, bat);

	CombatEventPayload payload;
	payload.ranged = attack.ranged;
	payload.isCounter = attack.counter;
	payload.attackIndex = attack.attackIndex;

	CombatEventPayload upcoming = payload;
	describeUpcomingAttack(upcoming, defender, secondaryTargets);

	processAttackTriggers(battle, CombatEventType::BEFORE_ATTACK, CombatEventType::BEFORE_ATTACKED, attacker, defender, upcoming);

	// a reaction to the upcoming attack may have killed either side of it - a unit that died before
	// striking does not strike, and one that died before being hit is not hit again
	if((!attacker->alive()) || (defender && !defender->alive()))
		return;

	std::shared_ptr<battle::CUnitState> attackerState = attacker->acquireState();

	rollAttackFlags(battle, attacker, bat);

	// only primary target
	if(defender && defender->alive())
		applyBattleEffects(battle, bat, attackerState, payload, defender, attack.distance, false);

	for(const auto * unit : secondaryTargets)
	{
		// a reaction to the upcoming attack may have killed it before the blow landed
		if(!unit->alive())
			continue;

		applyBattleEffects(battle, bat, attackerState, payload, unit, attack.distance, true);
		removeBonuses(battle, unit, *unit->getAllBonuses(Bonus::UntilTakingIndirectDamage));
	}

	markSpellLikeAttack(attacker, bat);

	attackerState->afterAttack(attack.ranged, attack.counter);

	{
		UnitChanges info(attackerState->unitId(), UnitChanges::EOperation::UPDATE);
		info.data = attackerState->save();
		bat.attackerChanges.changedStacks.push_back(info);
	}

	// collected before the blow lands: a stack that dies to it loses its spell effects, and a shield
	// that was up when the stack was struck answers the strike that killed it
	std::vector<PendingTrigger> reactions;
	collectEventTriggers(battle, reactions, CombatEventType::AFTER_ATTACK, attacker, defender);

	for(const AttackedTarget & target : payload.targets)
		collectEventTriggers(battle, reactions, CombatEventType::AFTER_ATTACKED, target.unit, attacker);

	gameHandler->sendAndApply(bat);

	{
		const bool multipleTargets = bat.bsa.size() > 1;

		int64_t totalDamage = 0;
		int32_t totalKills = 0;

		for(const BattleStackAttacked & bsa : bat.bsa)
		{
			totalDamage += bsa.damageAmount;
			totalKills += bsa.killedAmount;
		}

		addGenericDamageLog(blm, attackerState, totalDamage);

		if(defender)
			addGenericKilledLog(blm, defender, totalKills, multipleTargets);
	}

	// sent before the triggers below so that anything they log lands after the attack description
	gameHandler->sendAndApply(blm);

	if(defender)
		handleAfterAttackCasting(battle, attacker, defender, payload);

	// priority alone decides what runs first, which is how life drain heals before a fire shield can
	// burn the attacker down and how a death stare only lands after it. Not gated on anyone being
	// alive: a reflecting ability answers a lethal blow while dying, so each reaction decides for itself
	runEventTriggers(battle, reactions, payload);
}

void BattleActionProcessor::attackCasting(const CBattleInfoCallback & battle, bool ranged, BonusType attackMode, const battle::Unit * attacker, const CStack * defender)
{
	ObjectInstanceID ownerArmy = battle.getBattle()->getSideArmy(attacker->unitSide())->id;

	if(attacker->hasBonusOfType(attackMode))
	{
		TConstBonusListPtr spells = attacker->getBonuses(Selector::type()(attackMode));
		std::set<SpellID> spellsToCast = getSpellsForAttackCasting(spells, defender);

		for(SpellID spellID : spellsToCast)
		{
			bool castMe = false;
			if(!defender->alive())
			{
				logGlobal->debug("attackCasting: all attacked creatures have been killed");
				return;
			}
			int32_t spellLevel = 0;
			TConstBonusListPtr spellsByType = attacker->getBonuses(Selector::typeSubtype(attackMode, BonusSubtypeID(spellID)));
			for(const auto & sf : *spellsByType)
			{
				int meleeRanged = -1;
				if (sf->parameters)
				{
					vstd::amax(spellLevel, sf->parameters->toVector()[0]);
					meleeRanged = sf->parameters->toVector()[1];
				}

				if (meleeRanged == -1 || meleeRanged == 0 || (meleeRanged == 1 && ranged) || (meleeRanged == 2 && !ranged))
					castMe = true;
			}
			int chance = attacker->valOfBonuses(Selector::typeSubtype(attackMode, BonusSubtypeID(spellID)));
			vstd::amin(chance, 100);

			const CSpell * spell = SpellID(spellID).toSpell();
			spells::AbilityCaster caster(attacker, spellLevel);

			spells::Target target;
			target.emplace_back(defender);

			spells::BattleCast parameters(&battle, &caster, spells::Mode::PASSIVE, spell);

			auto m = spell->battleMechanics(&parameters);

			if(!m->canBeCastAt(target))
				continue;

			//check if spell should be cast (probability handling)
			if (!gameHandler->randomizer->rollCombatAbility(ownerArmy, chance))
				continue;

			//casting
			if(castMe)
			{
				parameters.cast(gameHandler->spellcastEnvironment(), target);
			}
		}
	}
}

std::set<SpellID> BattleActionProcessor::getSpellsForAttackCasting(const TConstBonusListPtr & spells, const CStack *defender)
{
	std::set<SpellID> spellsToCast;
	constexpr int unlayeredItemsInternalLayer = -1;

	std::map<int, std::vector<std::shared_ptr<Bonus>>> spellsWithBackupLayers;

	for(int i = 0; i < spells->size(); i++)
	{
		std::shared_ptr<Bonus> bonus = spells->operator[](i);
		int layer = bonus->parameters ? bonus->parameters->toVector()[2] : -1;
		vstd::amax(layer, -1);
		spellsWithBackupLayers[layer].push_back(bonus);
	}

	auto addSpellsFromLayer = [&](int layer) -> void
	{
		assert(spellsWithBackupLayers.find(layer) != spellsWithBackupLayers.end());

		for(const auto & spell : spellsWithBackupLayers[layer])
		{
			if (spell->subtype.as<SpellID>() != SpellID())
				spellsToCast.insert(spell->subtype.as<SpellID>());
			else
				logGlobal->error("Invalid spell to cast during attack!");
		}
	};

	if(spellsWithBackupLayers.find(unlayeredItemsInternalLayer) != spellsWithBackupLayers.end())
	{
		addSpellsFromLayer(unlayeredItemsInternalLayer);
		spellsWithBackupLayers.erase(unlayeredItemsInternalLayer);
	}

	for(auto item : spellsWithBackupLayers)
	{
		bool areCurrentLayerSpellsApplied = std::all_of(item.second.begin(), item.second.end(),
			[&](const std::shared_ptr<Bonus> spell)
			{
				std::vector<SpellID> activeSpells = defender->activeSpells();
				return vstd::find(activeSpells, spell->subtype.as<SpellID>()) != activeSpells.end();
			});

		if(!areCurrentLayerSpellsApplied || item.first == spellsWithBackupLayers.rbegin()->first)
		{
			addSpellsFromLayer(item.first);
			break;
		}
	}

	return spellsToCast;
}

/// Legacy spell-casting abilities only - combat event reactions run from makeAttack instead, and
/// deliberately without these gates.
void BattleActionProcessor::handleAfterAttackCasting(const CBattleInfoCallback & battle, const CStack * attacker, const CStack * defender, const CombatEventPayload & payload)
{
	if(!attacker->alive()) // can be already dead, e.g. from retaliation
		return;

	if(defender->alive())
		attackCasting(battle, payload.ranged, BonusType::SPELL_AFTER_ATTACK, attacker, defender);
}

void BattleActionProcessor::applyBattleEffects(const CBattleInfoCallback & battle, BattleAttack & bat, std::shared_ptr<battle::CUnitState> attackerState, CombatEventPayload & payload, const battle::Unit * def, int distance, bool secondary) const
{
	BattleStackAttacked bsa;
	if(secondary)
		bsa.flags |= BattleStackAttacked::SECONDARY; //all other targets do not suffer from spells & spell-like abilities

	bsa.attackerID = attackerState->unitId();
	bsa.stackAttacked = def->unitId();

	BattleAttackInfo bai(attackerState.get(), def, distance, bat.shot());
	bai.deathBlow = bat.deathBlow();
	bai.doubleDamage = bat.ballistaDoubleDmg();
	bai.luckyStrike  = bat.lucky();
	bai.unluckyStrike  = bat.unlucky();

	auto range = battle.calculateDmgRange(bai);
	{
		bsa.damageAmount = battle.getBattle()->getActualDamage(range.damage, attackerState->getCount(), gameHandler->getRandomGenerator());
		CStack::prepareAttacked(bsa, gameHandler->getRandomGenerator(), bai.defender->acquireState()); //calculate casualties
	}

	bat.bsa.push_back(bsa); //add this stack to the list of victims after drain life has been calculated

	AttackedTarget target;
	target.unit = def;
	target.damage = bsa.damageAmount;
	target.killed = bsa.killedAmount;
	// scripts that reflect a strike, such as fire shield, work from the blow that actually landed,
	// so the roll is scaled back up by what the defences took off it rather than rolled again
	target.damageBeforeDefense = range.damage.max > 0
		? bsa.damageAmount * range.damageBeforeDefense.max / range.damage.max
		: 0;
	target.healthBeforeAttack = def->getAvailableHealth();
	payload.targets.push_back(target);
}

void BattleActionProcessor::addGenericKilledLog(BattleLogMessage & blm, const CStack * defender, int32_t killed, bool multiple) const
{
	if(killed > 0)
	{
		MetaString line;

		if (killed > 1)
		{
			line.appendTextID("core.genrltxt.379"); // %d %s perished
			line.replaceNumber(killed);
		}
		else
			line.appendTextID("core.genrltxt.378"); // One %s perishes

		if (multiple)
		{
			if (killed > 1)
				line.replaceTextID("core.genrltxt.43"); // creatures
			else
				line.replaceTextID("core.genrltxt.42"); // creature
		}
		else
			line.replaceName(defender->unitType()->getId(), killed);

		blm.lines.push_back(line);
	}
}

void BattleActionProcessor::addGenericDamageLog(BattleLogMessage& blm, const std::shared_ptr<battle::CUnitState> &attackerState, int64_t damageDealt) const
{
	MetaString text;
	attackerState->addText(text, EMetaText::GENERAL_TXT, 376);
	attackerState->addNameReplacement(text);
	text.replaceNumber(damageDealt);
	blm.lines.push_back(std::move(text));
}

bool BattleActionProcessor::makeAutomaticBattleAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	return makeBattleActionImpl(battle, ba);
}

bool BattleActionProcessor::makePlayerBattleAction(const CBattleInfoCallback & battle, PlayerColor player, const BattleAction &ba)
{
	if (ba.side != BattleSide::ATTACKER && ba.side != BattleSide::DEFENDER && gameHandler->complain("Can not make action - invalid battle side!"))
		return false;

	if(battle.battleGetTacticDist() != 0)
	{
		if(!ba.isTacticsAction())
		{
			gameHandler->complain("Can not make actions while in tactics mode!");
			return false;
		}

		if(player != battle.sideToPlayer(ba.side))
		{
			gameHandler->complain("Can not make actions in battles you are not part of!");
			return false;
		}
	}
	else
	{
		const auto * active = battle.battleActiveUnit();
		if(!active)
		{
			gameHandler->complain("No active unit in battle!");
			return false;
		}

		if (ba.isUnitAction() && ba.stackNumber != active->unitId())
		{
			gameHandler->complain("Can not make actions - stack is not active!");
			return false;
		}

		auto unitOwner = battle.battleGetOwner(active);

		if(player != unitOwner)
		{
			gameHandler->complain("Can not make actions in battles you are not part of!");
			return false;
		}
	}

	return makeBattleActionImpl(battle, ba);
}

void BattleActionProcessor::runPredefinedReaction(const CBattleInfoCallback & battle, const Bonus & bonus, const battle::Unit * self, const battle::Unit * other)
{
	const auto parameters = bonus.parameters->toCustom<BonusParametersOnCombatEvent>();

	for (const auto & effect : parameters.effects)
	{
		const auto * bonusEffect = std::get_if<BonusParametersOnCombatEvent::CombatEffectBonus>(&effect);
		const auto * spellEffect = std::get_if<BonusParametersOnCombatEvent::CombatEffectSpell>(&effect);

		if (bonusEffect)
		{
			SetStackEffect sse;
			sse.battleID = battle.getBattle()->getBattleID();
			std::vector<Bonus> bonuses{*bonusEffect->bonus};
			if (bonusEffect->targetEnemy && other)
				sse.toAdd.emplace_back(other->unitId(), bonuses);
			if (!bonusEffect->targetEnemy)
				sse.toAdd.emplace_back(self->unitId(), bonuses);
			gameHandler->sendAndApply(sse);
		}
		if (spellEffect)
		{
			const CSpell * spell = spellEffect->spell.toSpell();
			spells::AbilityCaster spellCaster(self, spellEffect->masteryLevel);

			spells::Target spellTarget;
			if (spellEffect->targetEnemy && other)
				spellTarget.emplace_back(other);
			if (!spellEffect->targetEnemy)
				spellTarget.emplace_back(self);

			spells::BattleCast castParameters(&battle, &spellCaster, spells::Mode::PASSIVE, spell);

			auto m = spell->battleMechanics(&castParameters);

			if(m->canBeCastAt(spellTarget))
				castParameters.cast(gameHandler->spellcastEnvironment(), spellTarget);
		}
	}
}

void BattleActionProcessor::collectEventTriggers(const CBattleInfoCallback & battle, std::vector<PendingTrigger> & pending, CombatEventType event, const battle::Unit * self, const battle::Unit * other) const
{
	auto add = [&pending, event, self, other](const std::shared_ptr<const Bonus> & bonus, int priority, const ICombatEventScript * script)
	{
		PendingTrigger trigger;
		trigger.event = event;
		trigger.self = self->unitId();
		trigger.other = other ? other->unitId() : -1;
		trigger.priority = priority;
		trigger.bonus = bonus;
		trigger.script = script;
		pending.push_back(trigger);
	};

	// predefined reactions name the event they react to in their subtype, and declare no priority
	for (const auto & bonus : *self->getBonusesOfType(BonusType::ON_COMBAT_EVENT, BonusCustomSubtype(static_cast<int>(event))))
	{
		// what to do is the entire content of such a bonus, and the schema requires it, so one
		// without it is content that already failed validation and reacts to nothing
		if (bonus->parameters)
			add(bonus, 0, nullptr);
	}

	// a script instead reacts to every event it implements, so which script to run - the subtype -
	// is not part of the selector here
	for (const auto & bonus : *self->getBonusesOfType(BonusType::COMBAT_EVENT_TRIGGER))
	{
		ScriptID scriptID = bonus->subtype.as<ScriptID>();

		// content declaring a script that does not exist, or one of another kind - reported on load,
		// and there is nothing to run for it here
		if (!scriptID.hasValue())
		{
			logMod->warn("Unit '%s' carries a combat event trigger that names no script!", self->getDescription());
			continue;
		}

		const auto & script = LIBRARY->scriptTypes()->getById(scriptID);

		if (!script.combatEventScript)
		{
			logMod->warn("Unit '%s' carries a combat event trigger running script '%s', which is not a combat event script!", self->getDescription(), script.scriptId);
			continue;
		}

		if (script.combatEventScript->handlesEvent(battle, event))
			add(bonus, script.priority, script.combatEventScript.get());
	}
}

void BattleActionProcessor::runEventTriggers(const CBattleInfoCallback & battle, std::vector<PendingTrigger> & pending, const CombatEventPayload & payload)
{
	std::ranges::stable_sort(pending, {}, &PendingTrigger::priority);

	// Combat events are only fired from battle actions, and neither the script API nor the spell
	// casts below can start one - both only emit netpacks. Adding a binding that re-enters this
	// class (making a unit attack or move) would make this recursive and need a depth guard.
	for (const auto & trigger : pending)
	{
		// an earlier reaction may have removed either unit - transmutation replaces the stack it
		// hits - so both are looked up again rather than kept as pointers. A removed unit is left
		// behind as a ghost with no bonuses and no health, which is why every script that acts on
		// one has to check that it is still alive
		const battle::Unit * self = battle.battleGetUnitByID(trigger.self);
		if (!self)
			continue;

		const battle::Unit * other = trigger.other == -1 ? nullptr : battle.battleGetUnitByID(trigger.other);

		if (!trigger.script)
		{
			runPredefinedReaction(battle, *trigger.bonus, self, other);
			continue;
		}

		JsonNode parameters;
		if (trigger.bonus->parameters)
			parameters = trigger.bonus->parameters->toCustom<JsonNode>();

		// value of this bonus alone - another bonus running the same script is a reaction of its own,
		// with its own value, rather than something to sum into this one
		parameters["val"].Integer() = trigger.bonus->val;

		trigger.script->run(gameHandler->spellcastEnvironment(), battle, trigger.event, self, other, parameters, payload);
	}
}

void BattleActionProcessor::processBattleEventTriggers(const CBattleInfoCallback & battle, CombatEventType event, const battle::Unit * target, const battle::Unit * secondary, const CombatEventPayload & payload)
{
	std::vector<PendingTrigger> pending;
	collectEventTriggers(battle, pending, event, target, secondary);
	runEventTriggers(battle, pending, payload);
}

void BattleActionProcessor::processAttackTriggers(const CBattleInfoCallback & battle, CombatEventType attackerEvent, CombatEventType targetEvent, const CStack * attacker, const CStack * defender, const CombatEventPayload & payload)
{
	std::vector<PendingTrigger> pending;
	collectEventTriggers(battle, pending, attackerEvent, attacker, defender);

	for(const AttackedTarget & target : payload.targets)
		collectEventTriggers(battle, pending, targetEvent, target.unit, attacker);

	runEventTriggers(battle, pending, payload);
}
