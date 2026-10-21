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
#include "../../lib/ScopeGuard.h"
#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/battle/CObstacleInstance.h"
#include "../../lib/battle/IBattleState.h"
#include "../../lib/battle/BattleAction.h"
#include "../../lib/battle/actions/BattleActionType.h"
#include "../../lib/battle/actions/IBattleActionEnvironment.h"
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

void BattleActionProcessor::performAttackSequence(const CBattleInfoCallback & battle, const CStack * attacker, const CStack * defender, const BattleHex & targetHex, int distance, bool longWeaponAttack)
{
	int totalAttacks = attacker->getTotalAttacks(false);

	//TODO: move to CUnitState
	const auto * attackingHero = battle.battleGetFightingHero(attacker->unitSide());
	if(attackingHero)
	{
		totalAttacks += attackingHero->valOfBonuses(BonusType::HERO_GRANTS_ATTACKS, BonusSubtypeID(attacker->creatureId()));
	}

	static const auto firstStrikeSelector = Selector::typeSubtype(BonusType::FIRST_STRIKE, BonusCustomSubtype::damageTypeAll).Or(Selector::typeSubtype(BonusType::FIRST_STRIKE, BonusCustomSubtype::damageTypeMelee));
	const bool firstStrike = defender->hasBonus(firstStrikeSelector) && !defender->hasBonusOfType(BonusType::NOT_ACTIVE);

	bool ferocityApplied = false;
	int32_t defenderInitialQuantity = defender->getCount();

	// Bonuses added during the sequence, such as Blind, must survive this cleanup.
	BonusList attackerBonusesToRemove = *attacker->getAllBonuses(Bonus::untilAfterAttackSequence);
	BonusList defenderBonusesToRemove = *defender->getAllBonuses(Bonus::untilAfterAttackSequence);

	for (int i = 0; i < totalAttacks; ++i)
	{
		if(i == 0 && firstStrike && defender->ableToRetaliate() && !attacker->hasBonusOfType(BonusType::BLOCKS_RETALIATION) && !attacker->isInvincible() && !longWeaponAttack)
		{
			makeAttack(battle, defender, attacker, {.targetHex = attacker->getPosition(), .first = true, .counter = true});
		}

		// Movement, FIRST_STRIKE, or their handlers may kill or disable either unit.
		if(attacker->alive() && !attacker->hasBonusOfType(BonusType::NOT_ACTIVE) && defender->alive())
		{
			makeAttack(battle, attacker, defender, {.targetHex = targetHex, .distance = (i ? 0 : distance), .attackIndex = i, .first = i == 0});

			if(!ferocityApplied && attacker->hasBonusOfType(BonusType::FEROCITY))
			{
				auto ferocityBonus = attacker->getBonus(Selector::type()(BonusType::FEROCITY));
				int32_t requiredCreaturesToKill = ferocityBonus->parameters ? ferocityBonus->parameters->toNumber() : 1;
				if(defenderInitialQuantity - defender->getCount() >= requiredCreaturesToKill)
				{
					ferocityApplied = true;
					int additionalAttacksCount = attacker->valOfBonuses(BonusType::FEROCITY);
					totalAttacks += additionalAttacksCount;
				}
			}
		}

		// FIRST_STRIKE eligibility is fixed before the attack; newly unblocked retaliation waits for the next sequence.
		if(attacker->alive()
			&& !attacker->hasBonusOfType(BonusType::BLOCKS_RETALIATION)
			&& !attacker->isInvincible()
			&& !longWeaponAttack
			&& (i == 0 && !firstStrike)
			&& defender->ableToRetaliate())
		{
			makeAttack(battle, defender, attacker, {.targetHex = attacker->getPosition(), .first = true, .counter = true});
		}
	}

	removeBonuses(battle, attacker, attackerBonusesToRemove);
	removeBonuses(battle, defender, defenderBonusesToRemove);
}

void BattleActionProcessor::removeBonuses(const CBattleInfoCallback & battle, const battle::Unit * stack, const BonusList & bonuses)
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

void BattleActionProcessor::makeRangedAttack(const CBattleInfoCallback & battle, const CStack * stack, const BattleHex & destination)
{
	const CStack * destinationStack = battle.battleGetStackByPos(destination);
	// ShootAction::validate guarantees a target unit for shooters that can't target empty hexes
	const bool emptyTileAreaAttack = battle.battleCanTargetEmptyHex(stack);

	bool firstStrike = false;
	if(!emptyTileAreaAttack)
	{
		static const auto firstStrikeSelector = Selector::typeSubtype(BonusType::FIRST_STRIKE, BonusCustomSubtype::damageTypeAll).Or(Selector::typeSubtype(BonusType::FIRST_STRIKE, BonusCustomSubtype::damageTypeRanged));
		firstStrike = destinationStack->hasBonus(firstStrikeSelector) && !destinationStack->hasBonusOfType(BonusType::NOT_ACTIVE);
	}

	if (!firstStrike)
		makeAttack(battle, stack, destinationStack, {.targetHex = destination, .first = true, .ranged = true});

	// Bonuses added during the sequence, such as Blind, must survive this cleanup.
	BonusList attackerBonusesToRemove = *stack->getAllBonuses(Bonus::untilAfterAttackSequence);
	BonusList defenderBonusesToRemove;
	if (destinationStack)
		defenderBonusesToRemove = *destinationStack->getAllBonuses(Bonus::untilAfterAttackSequence);

	if (!emptyTileAreaAttack
		&& destinationStack->hasBonusOfType(BonusType::RANGED_RETALIATION)
		&& !stack->hasBonusOfType(BonusType::BLOCKS_RANGED_RETALIATION)
		&& destinationStack->ableToRetaliate()
		&& battle.battleCanShoot(destinationStack, stack->getPosition())
		// Fire Shield may kill the attacker before retaliation.
		&& stack->alive())
	{
		makeAttack(battle, destinationStack, stack, {.targetHex = stack->getPosition(), .first = true, .ranged = true, .counter = true});
	}
	int totalRangedAttacks = stack->getTotalAttacks(true);

	//TODO: move to CUnitState
	const auto * attackingHero = battle.battleGetFightingHero(stack->unitSide());
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
}

void BattleActionProcessor::complain(const spells::Problem & problem, const std::string & fallback)
{
	std::vector<std::string> texts;
	problem.getAll(texts);
	if(texts.empty())
		gameHandler->complain(fallback);
	for(const auto & text : texts)
		gameHandler->complain(text);
}

class BattleActionProcessor::ActionEnvironment final : public IBattleActionEnvironment
{
	BattleActionProcessor & processor;
	const CBattleInfoCallback & battle;

public:
	ActionEnvironment(BattleActionProcessor & processor, const CBattleInfoCallback & battle)
		: processor(processor)
		, battle(battle)
	{
	}

	void updateUnitBonuses(const battle::Unit & unit, const std::vector<Bonus> & bonuses) override
	{
		SetStackEffect sse;
		sse.battleID = battle.getBattle()->getBattleID();
		sse.toUpdate.emplace_back(unit.unitId(), bonuses);
		processor.gameHandler->sendAndApply(sse);
	}

	void addBattleLogLine(const MetaString & line) override
	{
		BattleLogMessage message;
		message.battleID = battle.getBattle()->getBattleID();
		message.lines.push_back(line);
		processor.gameHandler->sendAndApply(message);
	}

	void fireCombatEvent(CombatEventType event, const battle::Unit * unit, const battle::Unit * other, const CSpell * spell) override
	{
		CombatEventPayload payload;
		payload.spell = spell;
		processor.processBattleEventTriggers(battle, event, unit, other, payload);
	}

	void triggerObstaclesUnder(const battle::Unit & unit) override
	{
		battle.handleObstacleTriggersForUnit(*processor.gameHandler->spellEnv, unit);
	}

	UnitMoveResult moveUnit(const battle::Unit & unit, const BattleHex & destination) override
	{
		// the action was validated, so moveStack refuses the move only if a BEFORE_MOVE handler changed the battle;
		// it complains then, and the action still counts as made
		const auto result = processor.moveStack(battle, unit.unitId(), destination);
		return {result.distance, !result.obstacleHit && !result.invalidRequest};
	}

	void meleeAttack(const battle::Unit & attacker, const battle::Unit & defender, const BattleHex & targetHex, int distance, bool longWeapon) override
	{
		processor.performAttackSequence(battle, battle.battleGetStackByID(attacker.unitId()), battle.battleGetStackByID(defender.unitId()), targetHex, distance, longWeapon);
	}

	void rangedAttack(const battle::Unit & attacker, const BattleHex & destination) override
	{
		processor.makeRangedAttack(battle, battle.battleGetStackByID(attacker.unitId()), destination);
	}

	void castSpell(const battle::Unit & caster, const CSpell * spell, spells::Mode mode, int level, const battle::Target & target) override
	{
		spells::BattleCast parameters(&battle, &caster, mode, spell);
		parameters.setSpellLevel(level);
		parameters.cast(processor.gameHandler->spellcastEnvironment(), target);
	}

	void castHeroSpell(BattleSide side, const CSpell * spell, const battle::Target & target) override
	{
		const CGHeroInstance * hero = battle.battleGetFightingHero(side);
		spells::BattleCast parameters(&battle, hero, spells::Mode::HERO, spell);
		parameters.cast(processor.gameHandler->spellcastEnvironment(), target);
		processor.gameHandler->useChargeBasedSpell(hero->id, spell->getId());
	}

	vstd::RNG & getRandom() override
	{
		return processor.gameHandler->getRandomGenerator();
	}

	void endBattle(EBattleResult result, BattleSide winner) override
	{
		processor.owner->setBattleResult(battle, result, winner);
	}

	void giveResource(PlayerColor player, GameResID resource, int amount) override
	{
		processor.gameHandler->giveResource(player, resource, amount);
	}

	void endTacticsPhase() override
	{
		BattleTacticsEnded pack;
		pack.battleID = battle.getBattle()->getBattleID();
		processor.gameHandler->sendAndApply(pack);
	}
};

bool BattleActionProcessor::makeBattleActionImpl(const CBattleInfoCallback & battle, const BattleAction & requested)
{
	logGlobal->trace("Making action: %s", requested.toString());
	const CStack * stack = battle.battleGetStackByID(requested.stackNumber);

	const auto * requestedType = BattleActionType::find(requested);
	if(!requestedType)
	{
		gameHandler->complain("Unrecognized action type received!!");
		return false;
	}

	spells::detail::ProblemImpl problem;
	const bool valid = requestedType->validate(gameHandler->gameInfo(), battle, requested, problem);
	if(!valid)
	{
		complain(problem, "Battle action refused without a reason: " + requested.toString());

		if(!requested.isUnitAction() || !stack)
			return false;
	}

	// an invalid unit action skips the turn of the unit, so that an AI that repeats the action can't stall the battle
	const BattleAction ba = valid ? requested : BattleAction::makeNoAction(stack);

	// Events outside an action, such as round start or battle setup, add no action participants.
	actionParticipants.clear();
	if(stack && ba.isUnitAction())
		actionParticipants.push_back(stack->unitId());
	actionInProgress = true;
	auto finishAction = vstd::makeScopeGuard([this]() { actionInProgress = false; });

	// for these events client does not expects StartAction/EndAction wrapper
	if (!ba.isBattleEndAction())
	{
		StartAction startAction = BattleActionType::makeStartAction(battle, ba);
		gameHandler->sendAndApply(startAction);
	}

	ActionEnvironment env(*this, battle);
	BattleActionType::find(ba)->apply(env, battle, ba);

	if(ba.isBattleEndAction())
		return valid;

	// Resolve deaths first so ACTION_FINISHED observes final alive states.
	flushPendingDeaths(battle);

	// Obstacle triggers may kill a moving unit before ACTION_FINISHED.
	processActionFinishedTriggers(battle, stack);

	// EndAction closes the client animation batch, so send it after all action effects.
	EndAction endAction;
	endAction.battleID = battle.getBattle()->getBattleID();
	gameHandler->sendAndApply(endAction);

	return valid;
}

BattleActionProcessor::MovementResult BattleActionProcessor::moveStack(const CBattleInfoCallback & battle, int stack, BattleHex dest)
{
	const CStack *currentUnit = battle.battleGetStackByID(stack);

	assert(currentUnit);
	assert(dest < GameConstants::BFIELD_SIZE);

	if (battle.battleGetTacticDist())
	{
		assert(battle.isInTacticRange(dest));
	}

	auto start = currentUnit->getPosition();
	if (start == dest)
		return { 0, false, false };

	// Dispatch here to cover every move call. Handlers run before path calculation and may change
	// movement range.
	processBattleEventTriggers(battle, CombatEventType::BEFORE_MOVE, currentUnit, nullptr);

	// A handler may kill, replace or displace the unit, so reload all movement state.
	currentUnit = battle.battleGetStackByID(stack);

	// A removed unit has no matching AFTER_MOVE event.
	if(!currentUnit)
		return { 0, false, false };

	// Every exit after BEFORE_MOVE dispatches the matching AFTER_MOVE.
	auto moveEnded = [&](const MovementResult & result)
	{
		processBattleEventTriggers(battle, CombatEventType::AFTER_MOVE, currentUnit, nullptr);
		return result;
	};

	if(!currentUnit->alive() || currentUnit->getPosition() != start)
		return moveEnded({ 0, false, false });

	const CStack *stackAtEnd = battle.battleGetStackByPos(dest);

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
		gameHandler->complain(boost::str(boost::format("Given destination is not accessible! %s can not move from %d to %d%s")
			% currentUnit->getDescription() % start.toInt() % dest.toInt() % (stackAtEnd ? ", occupied by " + stackAtEnd->getDescription() : "")));
		return moveEnded({ 0, false, true });
	}

	bool canUseGate = false;
	auto dbState = battle.battleGetGateState();
	if(battle.battleGetFortifications().wallsHealth > 0 && currentUnit->controllingSide() == BattleSide::DEFENDER &&
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

	logGlobal->trace("%s moves from %d to %d, distance %d, movement range %d, path from destination: %s",
		currentUnit->getDescription(), start.toInt(), dest.toInt(), pathDistance, unitMovementRange, unitPath.toString());

	if (pathDistance > unitMovementRange)
	{
		gameHandler->complain(boost::str(boost::format("Given destination is not reachable! %s can not move from %d to %d: distance %d, movement range %d, path from destination: %s")
			% currentUnit->getDescription() % start.toInt() % dest.toInt() % pathDistance % unitMovementRange % unitPath.toString()));
		return moveEnded({ 0, false, true });
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

	return moveEnded({ static_cast<int16_t>(pathDistance), !movementSuccess, false });
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
		// a reaction to the upcoming attack may have killed it before the hit landed
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

	// collected before the hit lands: a stack that dies to it loses its spell effects, and a shield
	// that was up when the stack was struck answers the strike that killed it
	std::vector<PendingTrigger> reactions;
	collectEventTriggers(battle, reactions, CombatEventType::AFTER_ATTACK, attacker, defender);

	for(const AttackedTarget & target : payload.targets)
		collectEventTriggers(battle, reactions, CombatEventType::AFTER_ATTACKED, target.unit, attacker);

	noteDeaths(battle, bat.bsa);
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
	// alive: a reflecting ability answers a lethal hit while dying, so each reaction decides for itself
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
	// SoD: lucky strike only affects creature that was directly attacked; HotA: affects every target of a multi-target attack
	bai.luckyStrike  = bat.lucky() && (!secondary || gameHandler->gameInfo().getSettings().getBoolean(EGameSettings::COMBAT_LUCKY_STRIKE_AFFECTS_ALL_TARGETS));
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
	// scripts that reflect a strike, such as fire shield, work from the hit that actually landed,
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
	MetaString text = attackerState->formatGeneralMessage(376);
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

void BattleActionProcessor::collectEventTriggers(const CBattleInfoCallback & battle, std::vector<PendingTrigger> & pending, CombatEventType event, const battle::Unit * self, const battle::Unit * other)
{
	// Record all event targets because a unit may implement only ACTION_FINISHED.
	if(event != CombatEventType::ACTION_FINISHED && !vstd::contains(actionParticipants, self->unitId()))
		actionParticipants.push_back(self->unitId());

	auto add = [&pending, event, self, other](const std::shared_ptr<const Bonus> & bonus, int priority, const ICombatEventScript * script)
	{
		PendingTrigger trigger;
		trigger.event = event;
		trigger.self = self->unitId();
		trigger.other = other ? other->unitId() : noUnit;
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

		const battle::Unit * other = trigger.other == noUnit ? nullptr : battle.battleGetUnitByID(trigger.other);

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

void BattleActionProcessor::processActionFinishedTriggers(const CBattleInfoCallback & battle, const battle::Unit * actor)
{
	// Clear the member before collecting ACTION_FINISHED handlers to avoid extending this iteration.
	const std::vector<uint32_t> participants = std::exchange(actionParticipants, {});

	std::vector<PendingTrigger> pending;

	for(uint32_t participant : participants)
	{
		const battle::Unit * unit = battle.battleGetUnitByID(participant);

		// Removed units cannot receive ACTION_FINISHED.
		if(unit)
			collectEventTriggers(battle, pending, CombatEventType::ACTION_FINISHED, unit, actor);
	}

	runEventTriggers(battle, pending, CombatEventPayload());

	// Deaths caused by ACTION_FINISHED handlers still belong to this action.
	flushPendingDeaths(battle);
}

void BattleActionProcessor::noteDeaths(const CBattleInfoCallback & battle, const std::vector<BattleStackAttacked> & casualties)
{
	for(const BattleStackAttacked & casualty : casualties)
	{
		// Ignore zero-casualty hits, including subsequent hits on a corpse.
		if(!casualty.killed() || casualty.killedAmount == 0)
			continue;

		const battle::Unit * unit = battle.battleGetUnitByID(casualty.stackAttacked);
		if(!unit)
			continue;

		PendingDeath death{
			battle.getBattle()->getBattleID(),
			casualty.stackAttacked,
			casualty.attackerID,
			casualty.killedAmount,
			casualty.damageAmount,
			{}
		};
		collectEventTriggers(battle, death.triggers, CombatEventType::UNIT_DEATH, unit, battle.battleGetUnitByID(death.killer));
		pendingDeaths.push_back(std::move(death));
	}
}

void BattleActionProcessor::flushPendingDeaths(const CBattleInfoCallback & battle)
{
	const BattleID battleID = battle.getBattle()->getBattleID();

	while(true)
	{
		// Process newly queued deaths in the next batch and retain deaths from other battles.
		std::vector<PendingDeath> batch;
		std::ranges::copy_if(pendingDeaths, std::back_inserter(batch), [battleID](const PendingDeath & death){ return death.battle == battleID; });

		if(batch.empty())
			return;

		forgetPendingDeaths(battleID);

		CombatEventPayload payload;
		std::vector<PendingTrigger> pending;

		for(const PendingDeath & death : batch)
		{
			const battle::Unit * unit = battle.battleGetUnitByID(death.unit);

			// An earlier death handler may remove the unit.
			if(!unit)
				continue;

			AttackedTarget target;
			target.unit = unit;
			target.damage = death.damage;
			target.killed = death.killed;
			payload.targets.push_back(target);

			pending.insert(pending.end(), death.triggers.begin(), death.triggers.end());
		}

		// One dispatch preserves priority ordering across units, including rebirth before final-death reactions.
		runEventTriggers(battle, pending, payload);
	}
}

void BattleActionProcessor::forgetPendingDeaths(const BattleID & battleID)
{
	std::erase_if(pendingDeaths, [battleID](const PendingDeath & death){ return death.battle == battleID; });
}

void BattleActionProcessor::processSpellHitTriggers(const CBattleInfoCallback & battle, const spells::Spell & spell, const battle::Unit * casterUnit, const std::vector<std::shared_ptr<const battle::CUnitState>> & unitsBefore)
{
	if(!actionInProgress)
	{
		actionParticipants.clear();
		if(casterUnit)
			actionParticipants.push_back(casterUnit->unitId());
	}

	CombatEventPayload payload;
	payload.spell = &spell;

	for(const auto & before : unitsBefore)
	{
		AttackedTarget target;
		target.unitBefore = before.get();
		target.unit = battle.battleGetUnitByID(before->unitId());
		target.healthBeforeAttack = before->getAvailableHealth();

		// Healing and resurrection report zero damage; compare with the snapshot for their effect.
		if(target.unit)
		{
			target.damage = std::max<int64_t>(0, target.healthBeforeAttack - target.unit->getAvailableHealth());
			target.killed = std::max<int32_t>(0, before->getCount() - target.unit->getCount());
		}

		payload.targets.push_back(target);
	}

	std::vector<PendingTrigger> pending;

	for(const AttackedTarget & target : payload.targets)
		if(target.unit)
			collectEventTriggers(battle, pending, CombatEventType::SPELL_HIT, target.unit, casterUnit);

	runEventTriggers(battle, pending, payload);

	// Enchanter casts run during stack activation, outside a battle action.
	if(!actionInProgress)
	{
		flushPendingDeaths(battle);
		processActionFinishedTriggers(battle, casterUnit);
	}
}

void BattleActionProcessor::processAttackTriggers(const CBattleInfoCallback & battle, CombatEventType attackerEvent, CombatEventType targetEvent, const CStack * attacker, const CStack * defender, const CombatEventPayload & payload)
{
	std::vector<PendingTrigger> pending;
	collectEventTriggers(battle, pending, attackerEvent, attacker, defender);

	for(const AttackedTarget & target : payload.targets)
		collectEventTriggers(battle, pending, targetEvent, target.unit, attacker);

	runEventTriggers(battle, pending, payload);
}
