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

#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/CStack.h"
#include "../../lib/IGameSettings.h"
#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/battle/CObstacleInstance.h"
#include "../../lib/battle/IBattleState.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/entities/building/TownFortifications.h"
#include "../../lib/gameState/CGameState.h"
#include "../../lib/networkPacks/PacksForClientBattle.h"
#include "../../lib/networkPacks/SetStackEffect.h"
#include "../../lib/spells/AbilityCaster.h"
#include "../../lib/spells/CSpellHandler.h"
#include "../../lib/spells/ISpellMechanics.h"
#include "../../lib/spells/Problem.h"

#include <vstd/RNG.h>

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

	if (gameHandler->getResource(player, EGameResID::GOLD) < cost)
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
		for(auto s : texts)
			logGlobal->warn(s);
		return false;
	}

	parameters.cast(gameHandler->spellEnv, ba.getTarget(&battle));

	return true;
}

bool BattleActionProcessor::doWalkAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	const CStack * stack = battle.battleGetStackByID(ba.stackNumber);
	battle::Target target = ba.getTarget(&battle);

	if (!canStackAct(battle, stack))
		return false;

	if(target.size() < 1)
	{
		gameHandler->complain("Destination required for move action.");
		return false;
	}

	int walkedTiles = moveStack(battle, ba.stackNumber, target.at(0).hexValue); //move
	if (!walkedTiles)
	{
		gameHandler->complain("Stack failed movement!");
		return false;
	}
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

	sse.toUpdate.push_back(std::make_pair(ba.stackNumber, buffer));
	gameHandler->sendAndApply(sse);

	BattleLogMessage message;
	message.battleID = battle.getBattle()->getBattleID();

	MetaString text;
	stack->addText(text, EMetaText::GENERAL_TXT, 120);
	stack->addNameReplacement(text);
	text.replaceNumber(difference);

	message.lines.push_back(text);

	gameHandler->sendAndApply(message);
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
	int distance = moveStack(battle, ba.stackNumber, attackPos);

	logGlobal->trace("%s will attack %s", stack->nodeName(), destinationStack->nodeName());

	if(stack->getPosition() != attackPos && !(stack->doubleWide() && (stack->getPosition() == attackPos.cloneInDirection(stack->destShiftDir(), false))) )
	{
		// we were not able to reach destination tile, nor occupy specified hex
		// abort attack attempt, but treat this case as legal - we may have stepped onto a quicksands/mine
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

	if(!CStack::isMeleeAttackPossible(stack, destinationStack))
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
	const bool firstStrike = destinationStack->hasBonus(firstStrikeSelector);

	const bool retaliation = destinationStack->ableToRetaliate();
	bool ferocityApplied = false;
	int32_t defenderInitialQuantity = destinationStack->getCount();

	for (int i = 0; i < totalAttacks; ++i)
	{
		//first strike
		if(i == 0 && firstStrike && retaliation && !stack->hasBonusOfType(BonusType::BLOCKS_RETALIATION) && !stack->isInvincible())
		{
			makeAttack(battle, destinationStack, stack, 0, stack->getPosition(), true, false, true);
		}

		//move can cause death, eg. by walking into the moat, first strike can cause death or paralysis/petrification
		if(stack->alive() && !stack->hasBonusOfType(BonusType::NOT_ACTIVE) && destinationStack->alive())
		{
			makeAttack(battle, stack, destinationStack, (i ? 0 : distance), destinationTile, i==0, false, false);//no distance travelled on second attack

			if(!ferocityApplied && stack->hasBonusOfType(BonusType::FEROCITY))
			{
				auto ferocityBonus = stack->getBonus(Selector::type()(BonusType::FEROCITY));
				int32_t requiredCreaturesToKill = ferocityBonus->additionalInfo != CAddInfo::NONE ? ferocityBonus->additionalInfo[0] : 1;
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
			&& (i == 0 && !firstStrike)
			&& retaliation && destinationStack->ableToRetaliate())
		{
			makeAttack(battle, destinationStack, stack, 0, stack->getPosition(), true, false, true);
		}
	}

	//return
	if(stack->hasBonusOfType(BonusType::RETURN_AFTER_STRIKE)
		&& target.size() == 3
		&& startingPos != stack->getPosition()
		&& startingPos == target.at(2).hexValue
		&& stack->alive())
	{
		moveStack(battle, ba.stackNumber, startingPos);
		//NOTE: curStack->unitId() == ba.stackNumber (rev 1431)
	}
	return true;
}

bool BattleActionProcessor::doShootAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	const CStack * stack = battle.battleGetStackByID(ba.stackNumber);
	battle::Target target = ba.getTarget(&battle);

	if (!canStackAct(battle, stack))
		return false;

	if(target.size() < 1)
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
		firstStrike = destinationStack->hasBonus(firstStrikeSelector);
	}

	if (!firstStrike)
		makeAttack(battle, stack, destinationStack, 0, destination, true, true, false);

	//ranged counterattack
	if (!emptyTileAreaAttack
		&& destinationStack->hasBonusOfType(BonusType::RANGED_RETALIATION)
		&& !stack->hasBonusOfType(BonusType::BLOCKS_RANGED_RETALIATION)
		&& destinationStack->ableToRetaliate()
		&& battle.battleCanShoot(destinationStack, stack->getPosition())
		&& stack->alive()) //attacker may have died (fire shield)
	{
		makeAttack(battle, destinationStack, stack, 0, stack->getPosition(), true, true, true);
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
			makeAttack(battle, stack, destinationStack, 0, destination, false, true, false);
		}
	}

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
		parameters.cast(gameHandler->spellEnv, target);
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
	parameters.cast(gameHandler->spellEnv, target);
	return true;
}

bool BattleActionProcessor::doHealAction(const CBattleInfoCallback & battle, const BattleAction & ba)
{
	const CStack * stack = battle.battleGetStackByID(ba.stackNumber);
	battle::Target target = ba.getTarget(&battle);

	if (!canStackAct(battle, stack))
		return false;

	if(target.size() < 1)
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
		parameters.cast(gameHandler->spellEnv, {dest});
	}
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

int BattleActionProcessor::moveStack(const CBattleInfoCallback & battle, int stack, BattleHex dest)
{
	int ret = 0;

	const CStack *curStack = battle.battleGetStackByID(stack);
	const CStack *stackAtEnd = battle.battleGetStackByPos(dest);

	assert(curStack);
	assert(dest < GameConstants::BFIELD_SIZE);

	if (battle.battleGetTacticDist())
	{
		assert(battle.isInTacticRange(dest));
	}

	auto start = curStack->getPosition();
	if (start == dest)
		return 0;

	//initing necessary tables
	auto accessibility = battle.getAccessibility(curStack);
	BattleHexArray passed;
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
	auto dbState = battle.battleGetGateState();
	if(battle.battleGetFortifications().wallsHealth > 0 && curStack->unitSide() == BattleSide::DEFENDER &&
		dbState != EGateState::DESTROYED &&
		dbState != EGateState::BLOCKED)
	{
		canUseGate = true;
	}

	std::pair< BattleHexArray, int > path = battle.getPath(start, dest, curStack);

	ret = path.second;

	int creSpeed = curStack->getMovementRange(0);

	if (battle.battleGetTacticDist() > 0 && creSpeed > 0)
		creSpeed = GameConstants::BFIELD_SIZE;

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
				db.battleID = battle.getBattle()->getBattleID();
				db.state = EGateState::OPENED;
				gameHandler->sendAndApply(db);
			}

			//inform clients about move
			BattleStackMoved sm;
			sm.battleID = battle.getBattle()->getBattleID();
			sm.stack = curStack->unitId();
			BattleHexArray tiles;
			tiles.insert(path.first[0]);
			sm.tilesToMove = tiles;
			sm.distance = path.second;
			sm.teleporting = false;
			gameHandler->sendAndApply(sm);
		}
	}
	else //for non-flying creatures
	{
		BattleHexArray tiles;
		const int tilesToMove = std::max((int)(path.first.size() - creSpeed), 0);
		int v = (int)path.first.size()-1;
		path.first.insert(start);

		// check if gate need to be open or closed at some point
		BattleHex openGateAtHex, gateMayCloseAtHex;
		if (canUseGate)
		{
			for (int i = (int)path.first.size()-1; i >= 0; i--)
			{
				auto needOpenGates = [&](const BattleHex & hex) -> bool
				{
					if (hasWideMoat && hex == BattleHex::GATE_BRIDGE)
						return true;
					if (hex == BattleHex::GATE_BRIDGE && i-1 >= 0 && path.first[i-1] == BattleHex::GATE_OUTER)
						return true;
					else if (hex == BattleHex::GATE_OUTER || hex == BattleHex::GATE_INNER)
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
					if (curStack->doubleWide() && i + 2 < path.first.size())
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
					if (hex == BattleHex::GATE_INNER && i-1 >= 0 && path.first[i-1] != BattleHex::GATE_OUTER)
					{
						gateMayCloseAtHex = path.first[i-1];
					}
					if (hasWideMoat)
					{
						if (hex == BattleHex::GATE_BRIDGE && i-1 >= 0 && path.first[i-1] != BattleHex::GATE_OUTER)
						{
							gateMayCloseAtHex = path.first[i-1];
						}
						else if (hex == BattleHex::GATE_OUTER && i-1 >= 0 &&
							path.first[i-1] != BattleHex::GATE_INNER &&
							path.first[i-1] != BattleHex::GATE_BRIDGE)
						{
							gateMayCloseAtHex = path.first[i-1];
						}
					}
					else if (hex == BattleHex::GATE_OUTER && i-1 >= 0 && path.first[i-1] != BattleHex::GATE_INNER)
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
					tiles.insert(hex);

					if ((openGateAtHex.isValid() && openGateAtHex == hex) ||
						(gateMayCloseAtHex.isValid() && gateMayCloseAtHex == hex))
					{
						gateStateChanging = true;
					}

					//if we walked onto something, finalize this portion of stack movement check into obstacle
					if(!battle.battleGetAllObstaclesOnPos(hex, false).empty())
						obstacleHit = true;

					if (curStack->doubleWide())
					{
						BattleHex otherHex = curStack->occupiedHex(hex);
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
				sm.stack = curStack->unitId();
				sm.distance = path.second;
				sm.teleporting = false;
				sm.tilesToMove = tiles;
				gameHandler->sendAndApply(sm);
				tiles.clear();
			}

			//we don't handle obstacle at the destination tile -> it's handled separately in the if at the end
			if (curStack->getPosition() != dest)
			{
				if(stackIsMoving && start != curStack->getPosition())
				{
					stackIsMoving = battle.handleObstacleTriggersForUnit(*gameHandler->spellEnv, *curStack, passed);
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
							db.battleID = battle.getBattle()->getBattleID();
							db.state = EGateState::OPENED;
							gameHandler->sendAndApply(db);
						}
					}
					else if (curStack->getPosition() == gateMayCloseAtHex)
					{
						gateMayCloseAtHex = BattleHex();
						owner->updateGateState(battle);
					}
				}
			}
			else
				//movement finished normally: we reached destination
				stackIsMoving = false;
		}
	}
	//handle last hex separately for deviation
	if (gameHandler->getSettings().getBoolean(EGameSettings::COMBAT_ONE_HEX_TRIGGERS_OBSTACLES))
	{
		if (dest == battle::Unit::occupiedHex(start, curStack->doubleWide(), curStack->unitSide())
			|| start == battle::Unit::occupiedHex(dest, curStack->doubleWide(), curStack->unitSide()))
			passed.clear(); //Just empty passed, obstacles will handled automatically
	}
	if(dest == start) 	//If dest is equal to start, then we should handle obstacles for it anyway
		passed.clear();	//Just empty passed, obstacles will handled automatically
	//handling obstacle on the final field (separate, because it affects both flying and walking stacks)
	battle.handleObstacleTriggersForUnit(*gameHandler->spellEnv, *curStack, passed);

	return ret;
}

void BattleActionProcessor::makeAttack(const CBattleInfoCallback & battle, const CStack * attacker, const CStack * defender, int distance, const BattleHex & targetHex, bool first, bool ranged, bool counter)
{
	if(defender && first && !counter)
		handleAttackBeforeCasting(battle, ranged, attacker, defender);

	// If the attacker or defender is not alive before the attack action, the action should be skipped.
	if((!attacker->alive()) || (defender && !defender->alive()))
		return;

	FireShieldInfo fireShield;
	BattleAttack bat;
	BattleLogMessage blm;
	blm.battleID = battle.getBattle()->getBattleID();
	bat.battleID = battle.getBattle()->getBattleID();
	bat.attackerChanges.battleID = battle.getBattle()->getBattleID();
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
		auto goodLuckChanceVector = gameHandler->getSettings().getVector(EGameSettings::COMBAT_GOOD_LUCK_CHANCE);
		int luckDiceSize = gameHandler->getSettings().getInteger(EGameSettings::COMBAT_LUCK_DICE_SIZE);
		size_t chanceIndex = std::min<size_t>(goodLuckChanceVector.size(), attackerLuck) - 1; // array index, so 0-indexed

		if(goodLuckChanceVector.size() > 0 && gameHandler->getRandomGenerator().nextInt(1, luckDiceSize) <= goodLuckChanceVector[chanceIndex])
			bat.flags |= BattleAttack::LUCKY;
	}

	if(attackerLuck < 0)
	{
		auto badLuckChanceVector = gameHandler->getSettings().getVector(EGameSettings::COMBAT_BAD_LUCK_CHANCE);
		int luckDiceSize = gameHandler->getSettings().getInteger(EGameSettings::COMBAT_LUCK_DICE_SIZE);
		size_t chanceIndex = std::min<size_t>(badLuckChanceVector.size(), -attackerLuck) - 1; // array index, so 0-indexed

		if(badLuckChanceVector.size() > 0 && gameHandler->getRandomGenerator().nextInt(1, luckDiceSize) <= badLuckChanceVector[chanceIndex])
			bat.flags |= BattleAttack::UNLUCKY;
	}

	if (gameHandler->getRandomGenerator().nextInt(99) < attacker->valOfBonuses(BonusType::DOUBLE_DAMAGE_CHANCE))
	{
		bat.flags |= BattleAttack::DEATH_BLOW;
	}

	const auto * owner = battle.battleGetFightingHero(attacker->unitSide());
	if(owner)
	{
		int chance = owner->valOfBonuses(BonusType::BONUS_DAMAGE_CHANCE, BonusSubtypeID(attacker->creatureId()));
		if (chance > gameHandler->getRandomGenerator().nextInt(99))
			bat.flags |= BattleAttack::BALLISTA_DOUBLE_DMG;
	}

	battle::HealInfo healInfo;

	// only primary target
	if(defender && defender->alive())
		applyBattleEffects(battle, bat, attackerState, fireShield, defender, healInfo, distance, false);

	//multiple-hex normal attack
	std::set<const CStack*> attackedCreatures = battle.getAttackedCreatures(attacker, targetHex, bat.shot()); //creatures other than primary target
	for(const CStack * stack : attackedCreatures)
	{
		if(stack != defender && stack->alive()) //do not hit same stack twice
			applyBattleEffects(battle, bat, attackerState, fireShield, stack, healInfo, distance, true);
	}

	std::shared_ptr<const Bonus> bonus = attacker->getFirstBonus(Selector::type()(BonusType::SPELL_LIKE_ATTACK));
	if(bonus && ranged && bonus->subtype.hasValue()) //TODO: make it work in melee?
	{
		//this is need for displaying hit animation
		bat.flags |= BattleAttack::SPELL_LIKE;
		bat.spellID = bonus->subtype.as<SpellID>();

		//TODO: should spell override creature`s projectile?

		auto spell = bat.spellID.toSpell();

		battle::Target target;
		target.emplace_back(defender, targetHex);

		spells::BattleCast event(&battle, attacker, spells::Mode::SPELL_LIKE_ATTACK, spell);
		event.setSpellLevel(bonus->val);

		auto attackedCreatures = spell->battleMechanics(&event)->getAffectedStacks(target);

		//TODO: get exact attacked hex for defender

		for(const CStack * stack : attackedCreatures)
		{
			if(stack != defender && stack->alive()) //do not hit same stack twice
			{
				applyBattleEffects(battle, bat, attackerState, fireShield, stack, healInfo, distance, true);
			}
		}

		//now add effect info for all attacked stacks
		for (BattleStackAttacked & bsa : bat.bsa)
		{
			if (bsa.attackerID == attacker->unitId()) //this is our attack and not f.e. fire shield
			{
				//this is need for displaying affect animation
				bsa.flags |= BattleStackAttacked::SPELL_EFFECT;
				bsa.spellID = bonus->subtype.as<SpellID>();
			}
		}
	}

	attackerState->afterAttack(ranged, counter);

	{
		UnitChanges info(attackerState->unitId(), UnitChanges::EOperation::RESET_STATE);
		attackerState->save(info.data);
		bat.attackerChanges.changedStacks.push_back(info);
	}

	if (healInfo.healedHealthPoints > 0)
		bat.flags |= BattleAttack::LIFE_DRAIN;

	for (BattleStackAttacked & bsa : bat.bsa)
		bsa.battleID = battle.getBattle()->getBattleID();

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

	// drain life effect (as well as log entry) must be applied after the attack
	if(healInfo.healedHealthPoints > 0)
	{
		addGenericDrainedLifeLog(blm, attackerState, defender, healInfo.healedHealthPoints);
		addGenericResurrectedLog(blm, attackerState, defender, healInfo.resurrectedCount);
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

			const CGHeroInstance * actorOwner = battle.battleGetFightingHero(actor->unitSide());

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

			bsa.battleID = battle.getBattle()->getBattleID();
			bsa.flags |= BattleStackAttacked::FIRE_SHIELD;
			bsa.stackAttacked = attacker->unitId(); //invert
			bsa.attackerID = defender->unitId();
			bsa.damageAmount = totalDamage;
			attacker->prepareAttacked(bsa, gameHandler->getRandomGenerator());

			StacksInjured pack;
			pack.battleID = battle.getBattle()->getBattleID();
			pack.stacks.push_back(bsa);
			gameHandler->sendAndApply(pack);

			// TODO: this is already implemented in Damage::describeEffect()
			{
				MetaString text;
				text.appendLocalString(EMetaText::GENERAL_TXT, 376);
				text.replaceName(SpellID(SpellID::FIRE_SHIELD));
				text.replaceNumber(totalDamage);
				blm.lines.push_back(std::move(text));
			}
			addGenericKilledLog(blm, attacker, bsa.killedAmount, false);
		}
	}

	gameHandler->sendAndApply(blm);

	if(defender)
		handleAfterAttackCasting(battle, ranged, attacker, defender);
}

void BattleActionProcessor::attackCasting(const CBattleInfoCallback & battle, bool ranged, BonusType attackMode, const battle::Unit * attacker, const CStack * defender)
{
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
				int meleeRanged;
				vstd::amax(spellLevel, sf->additionalInfo[0]);
				meleeRanged = sf->additionalInfo[1];

				if (meleeRanged == CAddInfo::NONE || meleeRanged == 0 || (meleeRanged == 1 && ranged) || (meleeRanged == 2 && !ranged))
					castMe = true;
			}
			int chance = attacker->valOfBonuses((Selector::typeSubtype(attackMode, BonusSubtypeID(spellID))));
			vstd::amin(chance, 100);

			const CSpell * spell = SpellID(spellID).toSpell();
			spells::AbilityCaster caster(attacker, spellLevel);

			spells::Target target;
			target.emplace_back(defender);

			spells::BattleCast parameters(&battle, &caster, spells::Mode::PASSIVE, spell);

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

std::set<SpellID> BattleActionProcessor::getSpellsForAttackCasting(TConstBonusListPtr spells, const CStack *defender)
{
	std::set<SpellID> spellsToCast;
	constexpr int unlayeredItemsInternalLayer = -1;

	std::map<int, std::vector<std::shared_ptr<Bonus>>> spellsWithBackupLayers;

	for(int i = 0; i < spells->size(); i++)
	{
		std::shared_ptr<Bonus> bonus = spells->operator[](i);
		int layer = bonus->additionalInfo[2];
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

void BattleActionProcessor::handleAttackBeforeCasting(const CBattleInfoCallback & battle, bool ranged, const CStack * attacker, const CStack * defender)
{
	attackCasting(battle, ranged, BonusType::SPELL_BEFORE_ATTACK, attacker, defender); //no death stare / acid breath needed?
}

void BattleActionProcessor::handleDeathStare(const CBattleInfoCallback & battle, bool ranged, const CStack * attacker, const CStack * defender)
{
	// mechanics of Death Stare as in H3:
	// each gorgon have 10% chance to kill (counted separately in H3) -> binomial distribution
	//original formula x = min(x, (gorgons_count + 9)/10);

	/* mechanics of Accurate Shot as in HotA:
		* each creature in an attacking stack has a X% chance of killing a creature in the attacked squad,
		* but the total number of killed creatures cannot be more than (number of creatures in an attacking squad) * X/100 (rounded up).
		* X = 3 multiplier for shooting without penalty and X = 2 if shooting with penalty. Ability doesn't work if shooting at creatures behind walls.
		*/

	auto subtype = BonusCustomSubtype::deathStareGorgon;

	if (ranged)
	{
		bool rangePenalty = battle.battleHasDistancePenalty(attacker, attacker->getPosition(), defender->getPosition());
		bool obstaclePenalty = battle.battleHasWallPenalty(attacker, attacker->getPosition(), defender->getPosition());

		if(rangePenalty)
		{
			if(obstaclePenalty)
				subtype = BonusCustomSubtype::deathStareRangeObstaclePenalty;
			else
				subtype = BonusCustomSubtype::deathStareRangePenalty;
		}
		else
		{
			if(obstaclePenalty)
				subtype = BonusCustomSubtype::deathStareObstaclePenalty;
			else
				subtype = BonusCustomSubtype::deathStareNoRangePenalty;
		}
	}

	int singleCreatureKillChancePercent = attacker->valOfBonuses(BonusType::DEATH_STARE, subtype);
	double chanceToKill = singleCreatureKillChancePercent / 100.0;
	vstd::amin(chanceToKill, 1); //cap at 100%
	int killedCreatures = gameHandler->getRandomGenerator().nextBinomialInt(attacker->getCount(), chanceToKill);

	int maxToKill = vstd::divideAndCeil(attacker->getCount() * singleCreatureKillChancePercent, 100);
	vstd::amin(killedCreatures, maxToKill);

	killedCreatures += (attacker->level() * attacker->valOfBonuses(BonusType::DEATH_STARE, BonusCustomSubtype::deathStareCommander)) / defender->level();

	if(killedCreatures)
	{
		//TODO: death stare or accurate shot was not originally available for multiple-hex attacks, but...

		SpellID spellID = SpellID(SpellID::DEATH_STARE); //also used as fallback spell for ACCURATE_SHOT
		auto bonus = attacker->getBonus(Selector::typeSubtype(BonusType::DEATH_STARE, subtype));
		if(bonus && bonus->additionalInfo[0] != SpellID::NONE)
			spellID = SpellID(bonus->additionalInfo[0]);

		const CSpell * spell = spellID.toSpell();
		spells::AbilityCaster caster(attacker, 0);

		spells::BattleCast parameters(&battle, &caster, spells::Mode::PASSIVE, spell);
		spells::Target target;
		target.emplace_back(defender);
		parameters.setEffectValue(killedCreatures);
		parameters.cast(gameHandler->spellEnv, target);
	}
}

void BattleActionProcessor::handleAfterAttackCasting(const CBattleInfoCallback & battle, bool ranged, const CStack * attacker, const CStack * defender)
{
	if(!attacker->alive() || !defender->alive()) // can be already dead
		return;

	attackCasting(battle, ranged, BonusType::SPELL_AFTER_ATTACK, attacker, defender);

	if(!defender->alive())
	{
		//don't try death stare or acid breath on dead stack (crash!)
		return;
	}

	if(attacker->hasBonusOfType(BonusType::DEATH_STARE))
		handleDeathStare(battle, ranged, attacker, defender);

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

		spells::BattleCast parameters(&battle, &caster, spells::Mode::PASSIVE, spell);
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

		if(gameHandler->getRandomGenerator().nextDouble(0, 1) > chanceToTrigger)
			return;

		int bonusAdditionalInfo = attacker->getBonus(Selector::type()(BonusType::TRANSMUTATION))->additionalInfo[0];

		if(defender->unitType()->getIndex() == bonusAdditionalInfo ||
			(bonusAdditionalInfo == CAddInfo::NONE && defender->unitType()->getId() == attacker->unitType()->getId()))
			return;

		battle::UnitInfo resurrectInfo;
		resurrectInfo.id = battle.battleNextUnitId();
		resurrectInfo.summoned = false;
		resurrectInfo.position = defender->getPosition();
		resurrectInfo.side = defender->unitSide();

		if(bonusAdditionalInfo != CAddInfo::NONE)
			resurrectInfo.type = CreatureID(bonusAdditionalInfo);
		else
			resurrectInfo.type = attacker->creatureId();

		if(attacker->hasBonusOfType((BonusType::TRANSMUTATION), BonusCustomSubtype::transmutationPerHealth))
			resurrectInfo.count = std::max((defender->getCount() * defender->getMaxHealth()) / resurrectInfo.type.toCreature()->getMaxHealth(), 1u);
		else if (attacker->hasBonusOfType((BonusType::TRANSMUTATION), BonusCustomSubtype::transmutationPerUnit))
			resurrectInfo.count = defender->getCount();
		else
			return; //wrong subtype

		BattleUnitsChanged addUnits;
		addUnits.battleID = battle.getBattle()->getBattleID();
		addUnits.changedStacks.emplace_back(resurrectInfo.id, UnitChanges::EOperation::ADD);
		resurrectInfo.save(addUnits.changedStacks.back().data);

		BattleUnitsChanged removeUnits;
		removeUnits.battleID = battle.getBattle()->getBattleID();
		removeUnits.changedStacks.emplace_back(defender->unitId(), UnitChanges::EOperation::REMOVE);
		gameHandler->sendAndApply(removeUnits);
		gameHandler->sendAndApply(addUnits);

		// send empty event to client
		// temporary(?) workaround to force animations to trigger
		StacksInjured fakeEvent;
		fakeEvent.battleID = battle.getBattle()->getBattleID();
		gameHandler->sendAndApply(fakeEvent);
	}

	if(attacker->hasBonusOfType(BonusType::DESTRUCTION, BonusCustomSubtype::destructionKillPercentage) || attacker->hasBonusOfType(BonusType::DESTRUCTION, BonusCustomSubtype::destructionKillAmount))
	{
		double chanceToTrigger = 0;
		int amountToDie = 0;

		if(attacker->hasBonusOfType(BonusType::DESTRUCTION, BonusCustomSubtype::destructionKillPercentage)) //killing by percentage
		{
			chanceToTrigger = attacker->valOfBonuses(BonusType::DESTRUCTION, BonusCustomSubtype::destructionKillPercentage) / 100.0f;
			int percentageToDie = attacker->getBonus(Selector::type()(BonusType::DESTRUCTION).And(Selector::subtype()(BonusCustomSubtype::destructionKillPercentage)))->additionalInfo[0];
			amountToDie = static_cast<int>(defender->getCount() * percentageToDie * 0.01f);
		}
		else if(attacker->hasBonusOfType(BonusType::DESTRUCTION, BonusCustomSubtype::destructionKillAmount)) //killing by count
		{
			chanceToTrigger = attacker->valOfBonuses(BonusType::DESTRUCTION, BonusCustomSubtype::destructionKillAmount) / 100.0f;
			amountToDie = attacker->getBonus(Selector::type()(BonusType::DESTRUCTION).And(Selector::subtype()(BonusCustomSubtype::destructionKillAmount)))->additionalInfo[0];
		}

		vstd::amin(chanceToTrigger, 1); //cap trigger chance at 100%

		if(gameHandler->getRandomGenerator().nextDouble(0, 1) > chanceToTrigger)
			return;

		BattleStackAttacked bsa;
		bsa.attackerID = -1;
		bsa.stackAttacked = defender->unitId();
		bsa.damageAmount = amountToDie * defender->getMaxHealth();
		bsa.flags = BattleStackAttacked::SPELL_EFFECT;
		bsa.spellID = SpellID::SLAYER;
		defender->prepareAttacked(bsa, gameHandler->getRandomGenerator());

		StacksInjured si;
		si.battleID = battle.getBattle()->getBattleID();
		si.stacks.push_back(bsa);

		gameHandler->sendAndApply(si);
		sendGenericKilledLog(battle, defender, bsa.killedAmount, false);
	}
}

void BattleActionProcessor::applyBattleEffects(const CBattleInfoCallback & battle, BattleAttack & bat, std::shared_ptr<battle::CUnitState> attackerState, FireShieldInfo & fireShield, const CStack * def, battle::HealInfo & healInfo, int distance, bool secondary) const
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

		auto range = battle.calculateDmgRange(bai);
		bsa.damageAmount = battle.getBattle()->getActualDamage(range.damage, attackerState->getCount(), gameHandler->getRandomGenerator());
		CStack::prepareAttacked(bsa, gameHandler->getRandomGenerator(), bai.defender->acquireState()); //calculate casualties
	}

	//life drain handling
	if(attackerState->hasBonusOfType(BonusType::LIFE_DRAIN) && def->isLiving())
	{
		int64_t toHeal = bsa.damageAmount * attackerState->valOfBonuses(BonusType::LIFE_DRAIN) / 100;
		healInfo += attackerState->heal(toHeal, EHealLevel::RESURRECT, EHealPower::PERMANENT);
	}

	//soul steal handling
	if(attackerState->hasBonusOfType(BonusType::SOUL_STEAL) && def->isLiving())
	{
		//we can have two bonuses - one with subtype 0 and another with subtype 1
		//try to use permanent first, use only one of two
		for(const auto & subtype : { BonusCustomSubtype::soulStealBattle, BonusCustomSubtype::soulStealPermanent})
		{
			if(attackerState->hasBonusOfType(BonusType::SOUL_STEAL, subtype))
			{
				int64_t toHeal = bsa.killedAmount * attackerState->valOfBonuses(BonusType::SOUL_STEAL, subtype) * attackerState->getMaxHealth();
				bool permanent = subtype == BonusCustomSubtype::soulStealPermanent;
				healInfo += attackerState->heal(toHeal, EHealLevel::OVERHEAL, (permanent ? EHealPower::PERMANENT : EHealPower::ONE_BATTLE));
				break;
			}
		}
	}
	bat.bsa.push_back(bsa); //add this stack to the list of victims after drain life has been calculated

	//fire shield handling
	if(!bat.shot() &&
		!def->isClone() &&
		def->hasBonusOfType(BonusType::FIRE_SHIELD) &&
		!attackerState->hasBonusOfType(BonusType::SPELL_SCHOOL_IMMUNITY, BonusSubtypeID(SpellSchool::FIRE)) &&
		!attackerState->hasBonusOfType(BonusType::NEGATIVE_EFFECTS_IMMUNITY, BonusSubtypeID(SpellSchool::FIRE)) &&
		attackerState->valOfBonuses(BonusType::SPELL_DAMAGE_REDUCTION, BonusSubtypeID(SpellSchool::FIRE)) < 100 &&
		CStack::isMeleeAttackPossible(attackerState.get(), def) // attacked needs to be adjacent to defender for fire shield to trigger (e.g. Dragon Breath attack)
			)
	{
		//TODO: use damage with bonus but without penalties
		auto fireShieldDamage = (std::min<int64_t>(def->getAvailableHealth(), bsa.damageAmount) * def->valOfBonuses(BonusType::FIRE_SHIELD)) / 100;
		fireShield.push_back(std::make_pair(def, fireShieldDamage));
	}
}

void BattleActionProcessor::sendGenericKilledLog(const CBattleInfoCallback & battle, const CStack * defender, int32_t killed, bool multiple)
{
	if(killed > 0)
	{
		BattleLogMessage blm;
		blm.battleID = battle.getBattle()->getBattleID();
		addGenericKilledLog(blm, defender, killed, multiple);
		gameHandler->sendAndApply(blm);
	}
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

void BattleActionProcessor::addGenericDrainedLifeLog(BattleLogMessage& blm, const std::shared_ptr<battle::CUnitState>& attackerState, const CStack* defender, int64_t drainedLife) const
{
	MetaString text;
	attackerState->addText(text, EMetaText::GENERAL_TXT, 361);
	attackerState->addNameReplacement(text);
	text.replaceNumber(drainedLife);

	if (defender)
		defender->addNameReplacement(text);
	else
		text.replaceTextID("core.genrltxt.43"); // creatures

	blm.lines.push_back(std::move(text));
}

void BattleActionProcessor::addGenericResurrectedLog(BattleLogMessage& blm, const std::shared_ptr<battle::CUnitState>& attackerState, const CStack* defender, int64_t resurrected) const
{
	if (resurrected > 0)
	{
		MetaString & ms = blm.lines.back();

		if (resurrected == 1)
		{
			ms.appendLocalString(EMetaText::GENERAL_TXT, 363);		// "\n and one rises from the dead."
		}
		else
		{
			ms.appendLocalString(EMetaText::GENERAL_TXT, 364);		// "\n and %d rise from the dead."
			ms.replaceNumber(resurrected);
		}
	}
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
		auto active = battle.battleActiveUnit();
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
