/*
 * ClassicSpellEvaluator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "../StdInc.h"
#include "ClassicSpellEvaluator.h"

#include "ClassicRulesAdapter.h"
#include "ClassicAttackEvaluator.h"
#include "ClassicBattleStateView.h"

#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/CStack.h"
#include "../../../lib/battle/CBattleInfoCallback.h"
#include "../../../lib/battle/ReachabilityInfo.h"
#include "../../../lib/bonuses/BonusCustomTypes.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/spells/CSpell.h"
#include "../../../lib/spells/ISpellMechanics.h"
#include "../StackWithBonuses.h"
#include "ClassicBattleRng.h"
#include "ClassicDecisionTrace.h"
#include <vcmi/Environment.h>

namespace
{
std::vector<spells::Target> enumerateTargets(const spells::Mechanics & mechanics)
{
	const auto aimTypes = mechanics.getTargetTypes();
	std::vector<spells::Target> partial(1);
	for(spells::AimType aim : aimTypes)
	{
		std::vector<battle::Destination> destinations;
		switch(aim)
		{
			case spells::AimType::NOTHING:
				break;
			case spells::AimType::CREATURE:
			{
				auto units = mechanics.battle()->battleGetAllUnits(false);
				std::stable_sort(
					units.begin(),
					units.end(),
					[](const battle::Unit * lhs, const battle::Unit * rhs)
					{
						return lhs->unitId() < rhs->unitId();
					}
				);
				for(const battle::Unit * unit : units)
					destinations.emplace_back(unit);
				break;
			}
			case spells::AimType::LOCATION:
				for(int32_t index = 0; index < GameConstants::BFIELD_SIZE; ++index)
					destinations.emplace_back(BattleHex(index));
				break;
			case spells::AimType::OBSTACLE:
				return {};
		}

		if(aim == spells::AimType::NOTHING)
			continue;
		std::vector<spells::Target> expanded;
		for(const auto & prefix : partial)
		{
			for(const auto & destination : destinations)
			{
				auto candidate = prefix;
				candidate.push_back(destination);
				expanded.push_back(std::move(candidate));
			}
		}
		partial = std::move(expanded);
	}

	std::erase_if(
		partial,
		[&](const spells::Target & target)
		{
			return !mechanics.canBeCastAt(target);
		}
	);
	return partial;
}

double effectiveAverageDamage(const battle::Unit * unit, bool ranged)
{
	const bool cursed = unit->hasBonusOfType(BonusType::ALWAYS_MINIMUM_DAMAGE);
	const bool blessed = unit->hasBonusOfType(BonusType::ALWAYS_MAXIMUM_DAMAGE);
	const int32_t shift = unit->valOfBonuses(BonusType::ALWAYS_MAXIMUM_DAMAGE)
		- unit->valOfBonuses(BonusType::ALWAYS_MINIMUM_DAMAGE);
	const int32_t minimum = std::max(1, ClassicRulesAdapter::minDamage(unit, ranged) + shift);
	const int32_t maximum = std::max(1, ClassicRulesAdapter::maxDamage(unit, ranged) + shift);
	if(cursed != blessed)
		return cursed ? minimum : maximum;
	return (minimum + maximum) / 2.0;
}

int64_t unitUtility(const battle::Unit * unit)
{
	if(!unit || !unit->alive() || !unit->unitType())
		return 0;
	const int64_t maxHealth = std::max<int64_t>(1, unit->getMaxHealth());
	const int64_t base = static_cast<int64_t>(unit->unitType()->getFightValue()) * unit->getAvailableHealth() / maxHealth;
	const int32_t combatStats =
		ClassicRulesAdapter::attack(unit, unit->isShooter()) + ClassicRulesAdapter::defense(unit);
	int64_t result = base + base * combatStats / 40;
	const double baseDamage = (unit->unitType()->getBaseDamageMin()
		+ unit->unitType()->getBaseDamageMax()) / 2.0;
	if(baseDamage > 0.0)
		result = static_cast<int64_t>(result * std::sqrt(
			effectiveAverageDamage(unit, unit->isShooter()) / baseDamage));
	const int32_t meleeReduction = std::clamp(unit->valOfBonuses(
		BonusType::GENERAL_DAMAGE_REDUCTION,
		BonusSubtypeID(BonusCustomSubtype::damageTypeMelee)), 0, 100);
	const int32_t rangedReduction = std::clamp(unit->valOfBonuses(
		BonusType::GENERAL_DAMAGE_REDUCTION,
		BonusSubtypeID(BonusCustomSubtype::damageTypeRanged)), 0, 100);
	result += base * (meleeReduction + rangedReduction) / 200;
	if(unit->isShooter())
		result -= base * std::clamp(unit->valOfBonuses(BonusType::FORGETFULL), 0, 100) / 200;
	result += base * unit->getMovementRange() / 20;
	if(!unit->canMove())
		result /= 2;
	return result;
}

int64_t signedUtility(const battle::Unit * unit, BattleSide owner, BattleSide decidingSide)
{
	return owner == decidingSide ? unitUtility(unit) : -unitUtility(unit);
}

bool affectsUnit(const spells::Target & target, const battle::Unit * unit)
{
	return unit && (target.empty() || std::ranges::any_of(target, [unit](const battle::Destination & destination)
	{
		return destination.unitValue && destination.unitValue->unitId() == unit->unitId();
	}));
}

int64_t scaleForDuration(
	int64_t value,
	const CStack * unit,
	int32_t duration,
	int32_t attackTime,
	int32_t roundsLeft)
{
	const int32_t horizon = std::max(1, roundsLeft);
	const int32_t usableDuration = std::max(0, duration - (unit->waited() ? 1 : 0));
	const int32_t lastUsefulRound = std::min(usableDuration, horizon);
	if(attackTime > lastUsefulRound)
		return 0;
	return value * (lastUsefulRound - attackTime + 1) / horizon;
}

struct AttackOpportunity
{
	const CStack * originalTarget = nullptr;
	BattleHex attackFrom = BattleHex::INVALID;
	int64_t value = 0;
	int32_t distance = 0;
	int32_t attackTime = std::numeric_limits<int32_t>::max();
	bool ranged = false;
};

int64_t scaledOpportunityValue(
	const AttackOpportunity & opportunity,
	const CStack * unit,
	int32_t duration,
	int32_t roundsLeft)
{
	if(!opportunity.originalTarget || opportunity.value <= 0)
		return 0;
	return scaleForDuration(
		opportunity.value, unit, duration, opportunity.attackTime, roundsLeft);
}

std::pair<uint32_t, BattleHex> closestAttack(
	const ReachabilityInfo & reachability,
	const battle::Unit * attacker,
	const battle::Unit * defender)
{
	std::pair<uint32_t, BattleHex> result(ReachabilityInfo::INFINITE_DIST, BattleHex::INVALID);
	for(const BattleHex & landing : defender->getAttackableHexes(attacker))
	{
		if(!landing.isAvailable() || !reachability.isReachable(landing))
			continue;
		const uint32_t distance = reachability.distances[landing.toInt()];
		if(distance < result.first)
			result = {distance, landing};
	}
	return result;
}

int32_t attackTime(uint32_t distance, uint32_t speed)
{
	if(distance == ReachabilityInfo::INFINITE_DIST || speed == 0)
		return std::numeric_limits<int32_t>::max();
	return std::max<int32_t>(1, (distance + speed - 1) / speed);
}

int64_t modeledHealth(const CStack * stack, const ClassicCombatParameters & parameters)
{
	const int64_t health = stack->isClone() ? 1 : stack->getAvailableHealth();
	if(!parameters.simulated)
		return health;
	const auto found = parameters.expectedDamage.find(stack->unitId());
	return found == parameters.expectedDamage.end()
		? health
		: std::max<int64_t>(0, health - found->second);
}

AttackOpportunity bestOpportunity(
	const CBattleInfoCallback & scenario,
	const battle::Unit * actor,
	const CStack * originalActor,
	const std::shared_ptr<CBattleInfoCallback> & originalBattle,
	const ClassicAttackEvaluator & attackEvaluator,
	const ClassicCombatParameters & parameters,
	bool allowBerserkedOriginal = false)
{
	AttackOpportunity best;
	int32_t firstActionOffset = originalActor && (originalActor->moved() || originalActor->defended()) ? 1 : 0;
	const int32_t horizon = std::max(1, parameters.roundsLeft);
	while(actor && firstActionOffset < horizon && !actor->canMove(firstActionOffset))
		++firstActionOffset;
	if(!actor
	   || !actor->alive()
	   || !originalActor
	   || modeledHealth(originalActor, parameters) <= 0
	   || (originalActor->hasBonusOfType(BonusType::SIEGE_WEAPON) && !originalActor->isBallista())
	   || firstActionOffset >= horizon
	   || originalActor->isHypnotized()
	   || (!allowBerserkedOriginal
		   && originalActor->hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE)))
	{
		return best;
	}
	const ReachabilityInfo reachability = scenario.getReachability(actor);
	const uint32_t movementRange = ClassicRulesAdapter::movementRangeAfterBindCleanup(scenario, actor);
	for(const CStack * originalTarget : originalBattle->battleGetAllStacks(false))
	{
		if(!originalTarget->alive()
		   || modeledHealth(originalTarget, parameters) <= 0
		   || originalTarget->isTurret()
		   || originalTarget->unitSide() == originalActor->unitSide())
			continue;
		const battle::Unit * target = scenario.battleGetUnitByID(originalTarget->unitId());
		if(!target || !target->alive())
			continue;

		AttackOpportunity candidate;
		candidate.originalTarget = originalTarget;
		candidate.ranged = actor->isShooter()
			&& scenario.battleCanShoot(actor, target->getPosition());
		if(candidate.ranged)
		{
			candidate.distance = 1;
			candidate.attackTime = 1 + firstActionOffset;
			candidate.attackFrom = actor->getPosition();
		}
		else
		{
			const auto [distance, landing] = closestAttack(reachability, actor, target);
			if(distance == ReachabilityInfo::INFINITE_DIST)
				continue;
			candidate.distance = static_cast<int32_t>(distance);
			const int32_t firstAttackTime = distance == 0
				? 1
				: attackTime(distance, movementRange);
			candidate.attackTime = firstAttackTime == std::numeric_limits<int32_t>::max()
				? firstAttackTime
				: firstAttackTime + firstActionOffset;
			candidate.attackFrom = landing;
		}
		const int32_t effectDistance = !candidate.ranged
			&& candidate.distance > static_cast<int32_t>(movementRange)
			? 0
			: candidate.distance;
		candidate.value = attackEvaluator.projectedExchangeValue(
			scenario,
			actor,
			target,
			originalActor,
			originalTarget,
			candidate.attackFrom,
			candidate.ranged,
			effectDistance,
			parameters);
		if(!best.originalTarget
		   || candidate.attackTime < best.attackTime
		   || (candidate.attackTime == best.attackTime && candidate.value > best.value))
		{
			best = candidate;
		}
	}
	return best;
}

int64_t projectedEnchantmentValue(
	const std::shared_ptr<CBattleInfoCallback> & battle,
	const HypotheticBattle & after,
	BattleSide side,
	const spells::Target & target,
	int32_t duration,
	const ClassicCombatParameters & parameters,
	const ClassicAttackEvaluator & attackEvaluator)
{
	int64_t result = 0;
	for(const CStack * originalActor : battle->battleGetAllStacks(false))
	{
		if(!originalActor->alive())
			continue;
		const battle::Unit * changedActor = after.battleGetUnitByID(originalActor->unitId());
		if(!changedActor || !changedActor->alive())
			continue;

		const AttackOpportunity before = bestOpportunity(
			*battle, originalActor, originalActor, battle, attackEvaluator, parameters);
		const AttackOpportunity changed = bestOpportunity(
			after, changedActor, originalActor, battle, attackEvaluator, parameters);
		const bool affectsExchange = affectsUnit(target, originalActor)
			|| affectsUnit(target, before.originalTarget)
			|| affectsUnit(target, changed.originalTarget);
		if(!affectsExchange)
			continue;

		const int64_t beforeValue = scaledOpportunityValue(
			before, originalActor, duration, parameters.roundsLeft);
		const int64_t afterValue = scaledOpportunityValue(
			changed, originalActor, duration, parameters.roundsLeft);
		const int64_t difference = afterValue - beforeValue;
		result += originalActor->unitSide() == side ? difference : -difference;
	}
	return result;
}

int32_t retaliationCapacity(const battle::Unit * unit, int32_t roundOffset)
{
	const CSelector active = Selector::turns(roundOffset);
	if(unit->hasBonus(Selector::type()(BonusType::NO_RETALIATION).And(active)))
		return 0;
	if(unit->hasBonus(Selector::type()(BonusType::UNLIMITED_RETALIATIONS).And(active)))
		return std::numeric_limits<int32_t>::max();
	if(roundOffset == 0)
		return std::max(0, unit->acquireState()->counterAttacks.available());
	return std::max(0, 1 + unit->valOfBonuses(
		Selector::type()(BonusType::ADDITIONAL_RETALIATION).And(active)));
}

int64_t counterstrikeValue(
	const std::shared_ptr<CBattleInfoCallback> & battle,
	const HypotheticBattle & after,
	BattleSide side,
	const spells::Target & target,
	int32_t duration,
	const ClassicCombatParameters & parameters,
	const ClassicAttackEvaluator & attackEvaluator)
{
	struct IncomingAttack
	{
		const CStack * originalAttacker;
		AttackOpportunity opportunity;
		int64_t retaliationValue;
	};

	int64_t result = 0;
	for(const CStack * originalDefender : battle->battleGetAllStacks(false))
	{
		if(!originalDefender->alive()
		   || originalDefender->unitSide() != side
		   || !affectsUnit(target, originalDefender))
		{
			continue;
		}
		const battle::Unit * changedDefender = after.battleGetUnitByID(originalDefender->unitId());
		if(!changedDefender || !changedDefender->alive())
			continue;
		std::vector<IncomingAttack> incoming;
		for(const CStack * originalAttacker : battle->battleGetAllStacks(false))
		{
			if(!originalAttacker->alive() || originalAttacker->unitSide() == side)
				continue;
			const AttackOpportunity opportunity = bestOpportunity(
				*battle,
				originalAttacker,
				originalAttacker,
				battle,
				attackEvaluator,
				parameters);
			if(opportunity.ranged
			   || opportunity.originalTarget != originalDefender
			   || opportunity.value <= 0)
			{
				continue;
			}
			const battle::Unit * changedAttacker = after.battleGetUnitByID(originalAttacker->unitId());
			if(!changedAttacker || !changedAttacker->alive())
				continue;
			const int32_t effectDistance = opportunity.distance
				> static_cast<int32_t>(ClassicRulesAdapter::movementRangeAfterBindCleanup(
					after, changedAttacker))
				? 0
				: opportunity.distance;
			const int64_t retaliationValue = attackEvaluator.projectedRetaliationValue(
				after,
				changedAttacker,
				changedDefender,
				originalAttacker,
				originalDefender,
				opportunity.attackFrom,
				effectDistance,
				parameters);
			if(retaliationValue > 0)
				incoming.push_back({originalAttacker, opportunity, retaliationValue});
		}
		std::stable_sort(incoming.begin(), incoming.end(), [](const auto & lhs, const auto & rhs)
		{
			return lhs.opportunity.attackTime < rhs.opportunity.attackTime;
		});
		const int32_t horizon = std::max(1, parameters.roundsLeft);
		int64_t scaledResult = 0;
		for(int32_t round = 1; round <= horizon; ++round)
		{
			std::vector<const IncomingAttack *> availableIncoming;
			for(const IncomingAttack & attack : incoming)
			{
				const int32_t usableDuration = std::max(
					0, duration - (attack.originalAttacker->waited() ? 1 : 0));
				if(attack.opportunity.attackTime <= round && round <= usableDuration)
					availableIncoming.push_back(&attack);
			}
			const int32_t roundBefore = retaliationCapacity(originalDefender, round - 1);
			const int32_t roundAfter = retaliationCapacity(changedDefender, round - 1);
			// Retaliations reset at the start of every round. Within one round,
			// existing counters cover the earliest projected melee attacks and
			// Counterstrike is worth only the later attacks it additionally covers.
			const size_t firstAdded = std::min<size_t>(roundBefore, availableIncoming.size());
			const size_t pastAdded = std::min<size_t>(roundAfter, availableIncoming.size());
			for(size_t index = firstAdded; index < pastAdded; ++index)
				scaledResult += availableIncoming[index]->retaliationValue;
		}
		result += scaledResult / horizon;
	}
	return result;
}

const CStack * originalStack(
	const std::shared_ptr<CBattleInfoCallback> & battle,
	uint32_t unitId)
{
	const auto stacks = battle->battleGetAllStacks(false);
	const auto found = std::ranges::find_if(stacks, [unitId](const CStack * stack)
	{
		return stack->unitId() == unitId;
	});
	return found == stacks.end() ? nullptr : *found;
}

AttackOpportunity berserkOpportunity(
	const CBattleInfoCallback & scenario,
	const battle::Unit * actor,
	const CStack * originalActor,
	const std::shared_ptr<CBattleInfoCallback> & battle,
	int32_t roundsLeft)
{
	AttackOpportunity result;
	const int32_t horizon = std::max(1, roundsLeft);
	int32_t firstActionOffset = originalActor->moved() || originalActor->defended() ? 1 : 0;
	while(firstActionOffset < horizon && !actor->canMove(firstActionOffset))
		++firstActionOffset;
	if(firstActionOffset >= horizon)
		return result;
	const ForcedAction forced = scenario.getBerserkForcedAction(actor);
	if(!forced.target
	   || (forced.type != EActionType::SHOOT
		   && forced.type != EActionType::WALK_AND_ATTACK
		   && forced.type != EActionType::WALK))
	{
		return result;
	}
	result.originalTarget = originalStack(battle, forced.target->unitId());
	if(!result.originalTarget)
		return {};
	result.ranged = forced.type == EActionType::SHOOT;
	if(result.ranged)
	{
		result.attackFrom = actor->getPosition();
		result.distance = 1;
		result.attackTime = 1 + firstActionOffset;
	}
	else
	{
		const ReachabilityInfo reachability = scenario.getReachability(actor);
		const auto [distance, landing] = closestAttack(reachability, actor, forced.target);
		if(distance == ReachabilityInfo::INFINITE_DIST)
			return {};
		result.attackFrom = landing;
		result.distance = static_cast<int32_t>(distance);
		const uint32_t movementRange = ClassicRulesAdapter::movementRangeAfterBindCleanup(scenario, actor);
		const int32_t firstAttackTime = distance == 0 ? 1 : attackTime(distance, movementRange);
		result.attackTime = firstAttackTime == std::numeric_limits<int32_t>::max()
			? firstAttackTime
			: firstAttackTime + firstActionOffset;
	}
	return result;
}

int64_t signedExchangeValue(
	const CBattleInfoCallback & scenario,
	const battle::Unit * actor,
	const CStack * originalActor,
	const AttackOpportunity & opportunity,
	BattleSide side,
	const ClassicCombatParameters & parameters,
	const ClassicAttackEvaluator & attackEvaluator)
{
	if(!opportunity.originalTarget || opportunity.attackTime == std::numeric_limits<int32_t>::max())
		return 0;
	const battle::Unit * target = scenario.battleGetUnitByID(opportunity.originalTarget->unitId());
	if(!target || !target->alive())
		return 0;
	const uint32_t movementRange = ClassicRulesAdapter::movementRangeAfterBindCleanup(scenario, actor);
	const int32_t effectDistance = !opportunity.ranged
		&& opportunity.distance > static_cast<int32_t>(movementRange)
		? 0
		: opportunity.distance;
	const ClassicProjectedExchange exchange = attackEvaluator.projectedExchangeLosses(
		scenario,
		actor,
		target,
		originalActor,
		opportunity.originalTarget,
		opportunity.attackFrom,
		opportunity.ranged,
		effectDistance,
		parameters);
	const int64_t attackerValue = originalActor->unitSide() == side
		? -exchange.attackerLoss
		: exchange.attackerLoss;
	const int64_t defenderValue = opportunity.originalTarget->unitSide() == side
		? -exchange.defenderLoss
		: exchange.defenderLoss;
	return attackerValue + defenderValue;
}

int64_t scaledExchangeValue(int64_t value, int32_t attackTime, int32_t roundsLeft)
{
	const int32_t horizon = std::max(1, roundsLeft);
	if(attackTime > horizon)
		return 0;
	return value * (horizon - attackTime + 1) / horizon;
}

int64_t removedBerserkValue(
	const std::shared_ptr<CBattleInfoCallback> & battle,
	const HypotheticBattle & after,
	BattleSide side,
	const ClassicCombatParameters & parameters,
	const ClassicAttackEvaluator & attackEvaluator)
{
	int64_t result = 0;
	for(const CStack * originalActor : battle->battleGetAllStacks(false))
	{
		if(!originalActor->alive()
		   || !originalActor->hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE))
			continue;
		const battle::Unit * changedActor = after.battleGetUnitByID(originalActor->unitId());
		if(!changedActor
		   || !changedActor->alive()
		   || changedActor->hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE))
		{
			continue;
		}

		const AttackOpportunity before = berserkOpportunity(
			*battle, originalActor, originalActor, battle, parameters.roundsLeft);
		const AttackOpportunity changed = bestOpportunity(
			after, changedActor, originalActor, battle, attackEvaluator, parameters, true);
		const int64_t beforeValue = scaledExchangeValue(
			signedExchangeValue(
				*battle,
				originalActor,
				originalActor,
				before,
				side,
				parameters,
				attackEvaluator),
			before.attackTime,
			parameters.roundsLeft);
		const int64_t changedValue = changed.value > 0
			? scaledExchangeValue(
				signedExchangeValue(
					after,
					changedActor,
					originalActor,
					changed,
					side,
					parameters,
					attackEvaluator),
				changed.attackTime,
				parameters.roundsLeft)
			: 0;
		result += changedValue - beforeValue;
	}
	return result;
}

int64_t berserkValue(
	const std::shared_ptr<CBattleInfoCallback> & battle,
	const HypotheticBattle & after,
	BattleSide side,
	const ClassicCombatParameters & parameters,
	const ClassicAttackEvaluator & attackEvaluator)
{
	int64_t result = 0;
	for(const CStack * originalActor : battle->battleGetAllStacks(false))
	{
		if(!originalActor->alive())
			continue;
		const battle::Unit * changedActor = after.battleGetUnitByID(originalActor->unitId());
		if(!changedActor
		   || !changedActor->alive()
		   || originalActor->hasBonusFrom(
			   BonusSource::SPELL_EFFECT, BonusSourceID(SpellID(SpellID::BERSERK)))
		   || !changedActor->hasBonusFrom(
			   BonusSource::SPELL_EFFECT, BonusSourceID(SpellID(SpellID::BERSERK))))
		{
			continue;
		}
		const AttackOpportunity before = bestOpportunity(
			*battle, originalActor, originalActor, battle, attackEvaluator, parameters);
		const AttackOpportunity afterOpportunity = berserkOpportunity(
			after, changedActor, originalActor, battle, parameters.roundsLeft);
		// Score losses by physical army instead of by attacker/defender role:
		// Berserk may force two stacks from the same army to damage one another.
		const int64_t beforeValue = before.value > 0
			? scaledExchangeValue(
				signedExchangeValue(
					*battle,
					originalActor,
					originalActor,
					before,
					side,
					parameters,
					attackEvaluator),
				before.attackTime,
				parameters.roundsLeft)
			: 0;
		const int64_t afterValue = scaledExchangeValue(
			signedExchangeValue(
				after,
				changedActor,
				originalActor,
				afterOpportunity,
				side,
				parameters,
				attackEvaluator),
			afterOpportunity.attackTime,
			parameters.roundsLeft);
		result += afterValue - beforeValue;
	}
	return result;
}

int64_t opportunityUtility(const AttackOpportunity & opportunity, int32_t roundsLeft)
{
	const int32_t horizon = std::max(1, roundsLeft);
	if(!opportunity.originalTarget || opportunity.value <= 0 || opportunity.attackTime > horizon)
		return 0;
	return opportunity.value * (horizon - opportunity.attackTime + 1) / horizon;
}

int64_t teleportValue(
	const std::shared_ptr<CBattleInfoCallback> & battle,
	const HypotheticBattle & after,
	BattleSide side,
	const spells::Target & target,
	const ClassicCombatParameters & parameters,
	const ClassicAttackEvaluator & attackEvaluator)
{
	if(target.empty() || !target.front().unitValue)
		return 0;
	const uint32_t sourceID = target.front().unitValue->unitId();
	const battle::Unit * moved = after.battleGetUnitByID(sourceID);
	if(!moved || after.playerToSide(after.battleGetOwner(moved)) != side)
		return 0;

	int64_t result = 0;
	for(const CStack * originalActor : battle->battleGetAllStacks(false))
	{
		if(!originalActor->alive())
			continue;
		const battle::Unit * changedActor = after.battleGetUnitByID(originalActor->unitId());
		if(!changedActor || !changedActor->alive())
			continue;
		const AttackOpportunity before = bestOpportunity(
			*battle, originalActor, originalActor, battle, attackEvaluator, parameters);
		const AttackOpportunity changed = bestOpportunity(
			after, changedActor, originalActor, battle, attackEvaluator, parameters);
		const int64_t difference = opportunityUtility(changed, parameters.roundsLeft)
			- opportunityUtility(before, parameters.roundsLeft);
		result += originalActor->unitSide() == moved->unitSide() ? difference : -difference;
	}
	return result;
}
}

ClassicSpellEvaluator::ClassicSpellEvaluator(
	const Environment * env,
	std::shared_ptr<CBattleInfoCallback> battle,
	std::shared_ptr<IClassicBattleAIRng> randomGenerator,
	std::shared_ptr<ClassicDecisionTrace> trace,
	const ClassicAttackEvaluator * attackEvaluator
)
	: env(env)
	, battle(std::move(battle))
	, randomGenerator(std::move(randomGenerator))
	, trace(std::move(trace))
	, attackEvaluator(attackEvaluator)
{
}

int64_t ClassicSpellEvaluator::evaluateStateChange(const HypotheticBattle & after, BattleSide side) const
{
	std::set<uint32_t> ids;
	for(const battle::Unit * unit : battle->battleGetAllUnits(false))
		ids.insert(unit->unitId());
	for(const battle::Unit * unit : after.battleGetAllUnits(false))
		ids.insert(unit->unitId());

	int64_t beforeValue = 0;
	int64_t afterValue = 0;
	for(uint32_t id : ids)
	{
		if(const battle::Unit * unit = battle->battleGetUnitByID(id))
		{
			const BattleSide owner = battle->playerToSide(battle->battleGetOwner(unit));
			beforeValue += signedUtility(unit, owner, side);
		}
		if(const battle::Unit * unit = after.battleGetUnitByID(id))
		{
			const BattleSide owner = after.playerToSide(after.battleGetOwner(unit));
			afterValue += signedUtility(unit, owner, side);
		}
	}
	return afterValue - beforeValue;
}

int64_t ClassicSpellEvaluator::applyManaConservation(int64_t value, int32_t castsAvailable)
{
	if(castsAvailable <= 0 || value <= 0)
		return 0;
	if(castsAvailable < 7)
		return static_cast<int64_t>(std::llround(value * std::sqrt(static_cast<double>(castsAvailable))));
	return 5 * value / 2;
}

int32_t ClassicSpellEvaluator::classicSlowSpeed(int32_t currentEffectiveSpeed, int32_t effectLevel)
{
	const int32_t slowPercent = effectLevel >= 2 ? 50 : 75;
	return std::max(1, currentEffectiveSpeed * slowPercent / 100);
}

int64_t ClassicSpellEvaluator::manaAdjustedValue(int64_t rawValue, int32_t currentMana, int32_t cost) const
{
	return cost > 0 ? applyManaConservation(rawValue, currentMana / cost) : applyManaConservation(rawValue, 7);
}

ClassicScoredSpell ClassicSpellEvaluator::chooseHeroSpell(
	BattleSide side,
	bool retreating,
	const ClassicCombatParameters & parameters) const
{
	ClassicScoredSpell best;
	const CGHeroInstance * hero = battle->battleGetFightingHero(side);
	if(!canChooseHeroSpell(side))
		return best;
	ClassicCombatValue combatValue(battle);
	const ClassicProjectedTargets hasteTargets = attackEvaluator
		? attackEvaluator->projectSpellTargets(side, parameters)
		: ClassicProjectedTargets{};
	const ClassicProjectedTargets slowTargets = attackEvaluator
		? attackEvaluator->projectSpellTargets(battle->otherSide(side), parameters)
		: ClassicProjectedTargets{};

	for(int32_t spellIndex = 0; spellIndex <= 69; ++spellIndex)
	{
		const CSpell * spell = SpellID(spellIndex).toSpell();
		if(!spell || !spell->isCombat() || (retreating && !spell->isDamage()))
			continue;
		if(!hero->canCastThisSpell(spell))
			continue;
		if(!spell->canBeCast(battle.get(), spells::Mode::HERO, hero))
			continue;
		const int32_t cost = battle->battleGetSpellCost(spell, hero);
		if(cost > hero->mana)
			continue;

		spells::BattleCast castInfo(battle.get(), hero, spells::Mode::HERO, spell);
		auto mechanics = spell->battleMechanics(&castInfo);
		ClassicScoredSpell spellBest;
		for(const spells::Target & target : enumerateTargets(*mechanics))
		{
			auto state = std::make_shared<HypotheticBattle>(env, battle);
			spells::BattleCast cast(state.get(), hero, spells::Mode::HERO, spell);
			cast.castEval(state->getServerCallback(), target);
			int64_t rawValue = evaluateStateChange(*state, side);
			if(attackEvaluator)
				rawValue += removedBerserkValue(
					battle, *state, side, parameters, *attackEvaluator);
			if(spellIndex == SpellID::BLESS
			   || spellIndex == SpellID::CURSE
			   || spellIndex == SpellID::SHIELD
			   || spellIndex == SpellID::AIR_SHIELD
			   || spellIndex == SpellID::FIRE_SHIELD
			   || spellIndex == SpellID::FORGETFULNESS)
			{
				rawValue = attackEvaluator
					? projectedEnchantmentValue(
						battle,
						*state,
						side,
						target,
						mechanics->getEffectDuration(),
						parameters,
						*attackEvaluator)
					: 0;
			}
			else if(spellIndex == SpellID::COUNTERSTRIKE)
			{
				rawValue = attackEvaluator
					? counterstrikeValue(
						battle,
						*state,
						side,
						target,
						mechanics->getEffectDuration(),
						parameters,
						*attackEvaluator)
					: 0;
			}
			else if(spellIndex == SpellID::BERSERK)
			{
				rawValue = attackEvaluator
					? berserkValue(battle, *state, side, parameters, *attackEvaluator)
					: 0;
			}
			else if(spellIndex == SpellID::TELEPORT)
			{
				rawValue = attackEvaluator
					? teleportValue(battle, *state, side, target, parameters, *attackEvaluator)
					: 0;
			}
			else if(spellIndex == SpellID::HASTE)
			{
				rawValue = 0;
				const int32_t horizon = std::max(1, parameters.roundsLeft);
				const int32_t duration = mechanics->getEffectDuration();
				for(const CStack * original : battle->battleGetAllStacks(false))
				{
					if(original->unitSide() != side
					   || vstd::contains(original->activeSpells(), SpellID(SpellID::HASTE)))
						continue;
					const auto planIt = hasteTargets.find(original->unitId());
					if(planIt == hasteTargets.end() || !planIt->second.target)
						continue;
					if(duration - (original->waited() ? 1 : 0) <= 0)
						continue;
					const battle::Unit * resulting = state->battleGetUnitByID(original->unitId());
					if(!resulting || !resulting->alive())
						continue;
					// Basic/advanced Haste has a single unit target. Expert Haste
					// is represented by an empty mass target.
					const bool affected = target.empty() || std::ranges::any_of(
						target,
						[&](const auto & destination)
						{
							return destination.unitValue
								&& destination.unitValue->unitId() == original->unitId();
						});
					if(!affected)
						continue;
					const int32_t oldSpeed = original->getInitiative(0);
					const int32_t newSpeed = resulting->getInitiative(0);
					if(oldSpeed <= 0 || newSpeed <= 0)
						continue;
					const int32_t distance = planIt->second.distance;
					// Bind prevents movement, but Haste can still change the order
					// of attacks against an adjacent target.
					if(distance > 0 && (!original->getMovementRange() || !resulting->getMovementRange()))
						continue;
					const int32_t oldTime = std::max(1, (distance + oldSpeed - 1) / oldSpeed);
					const int32_t newTime = std::max(1, (distance + newSpeed - 1) / newSpeed);
					if(newTime > horizon)
						continue;

					int64_t value = 0;
					if(newTime == 1)
					{
						const int32_t targetSpeed = planIt->second.target->getInitiative(0);
						if(targetSpeed >= oldSpeed && targetSpeed < newSpeed && attackEvaluator)
						{
							value = std::max<int64_t>(0, attackEvaluator->spellExchangeEffect(
								original, planIt->second.target, parameters));
						}
					}
					if(newTime < oldTime)
					{
						const int32_t cappedOldTime = std::min(oldTime, horizon + 1);
						const int64_t stackValue = combatValue.stackValue(original, parameters);
						value += (horizon - newTime + 1) * stackValue / horizon;
						value -= (horizon - cappedOldTime + 1) * stackValue / horizon;
					}
					rawValue += value;
					if(trace)
						trace->record("spell.haste.stack", std::to_string(original->unitId()), value);
				}
			}
			else if(spellIndex == SpellID::SLOW)
			{
				rawValue = 0;
				const int32_t horizon = std::max(1, parameters.roundsLeft);
				const int32_t duration = mechanics->getEffectDuration();
				for(const CStack * original : battle->battleGetAllStacks(false))
				{
					if(original->unitSide() == side
					   || vstd::contains(original->activeSpells(), SpellID(SpellID::SLOW)))
						continue;
					const battle::Unit * resulting = state->battleGetUnitByID(original->unitId());
					const bool affected = target.empty() || std::ranges::any_of(
						target,
						[&](const auto & destination)
						{
							return destination.unitValue
								&& destination.unitValue->unitId() == original->unitId();
						});
					if(!resulting || !resulting->alive() || !affected
					   || !resulting->hasBonusFrom(BonusSource::SPELL_EFFECT, BonusSourceID(SpellID(SpellID::SLOW))))
						continue;
					const auto planIt = slowTargets.find(original->unitId());
					if(planIt == slowTargets.end() || !planIt->second.target)
						continue;
					const int32_t usableDuration = duration - (original->waited() ? 1 : 0);
					if(usableDuration <= 0)
						continue;

					const int32_t oldSpeed = original->getInitiative(0);
					// The SoD callback does not ask the rules engine for the
					// post-cast speed. It applies Slow's mastery percentage to
					// the stack's current effective speed, even when that speed
					// already includes Haste.
					const int32_t newSpeed = classicSlowSpeed(oldSpeed, mechanics->getEffectLevel());
					if(oldSpeed <= 0 || newSpeed <= 0)
						continue;
					const int32_t distance = planIt->second.distance;
					if(distance > 0 && !original->getMovementRange())
						continue;
					const int32_t oldTime = std::max(1, (distance + oldSpeed - 1) / oldSpeed);
					if(oldTime > horizon)
						continue;

					int64_t value = 0;
					if(oldTime == 1 && attackEvaluator)
					{
						// If Slow moves the victim behind a friendly attacker that
						// plans to hit it this turn, preserve the best newly reversed
						// exchange. The executable scans physical army order and keeps
						// a strict maximum starting at zero.
						for(const CStack * friendly : battle->battleGetAllStacks(false))
						{
							if(friendly->unitSide() != side || !friendly->alive()
							   || friendly->isFirstAidTent() || friendly->isAmmoCart()
							   || vstd::contains(friendly->activeSpells(), SpellID(SpellID::BLIND))
							   || vstd::contains(friendly->activeSpells(), SpellID(SpellID::STONE_GAZE))
							   || vstd::contains(friendly->activeSpells(), SpellID(SpellID::PARALYZE)))
								continue;
							const auto friendlyPlan = hasteTargets.find(friendly->unitId());
							if(friendlyPlan == hasteTargets.end()
							   || friendlyPlan->second.target != original)
								continue;
							const int32_t friendlySpeed = friendly->getInitiative(0);
							if(friendlySpeed > oldSpeed || friendlySpeed <= newSpeed)
								continue;
							value = std::max<int64_t>(value, attackEvaluator->spellExchangeEffect(
								friendly, original, parameters));
						}
					}

					// Ranged stacks lose no modeled attack opportunities. For a
					// melee target, cap the delay by spell duration and the combat
					// horizon, then subtract the two integer action-share values.
					if(!battle->battleCanShoot(original))
					{
						const int32_t newTime = std::max(1, (distance + newSpeed - 1) / newSpeed);
						const int32_t delayedTime = std::min(
							std::min(newTime - oldTime, usableDuration) + oldTime,
							horizon + 1);
						if(delayedTime > oldTime)
						{
							const int64_t stackValue = combatValue.stackValue(original, parameters);
							value += (horizon - oldTime + 1) * stackValue / horizon;
							value -= (horizon - delayedTime + 1) * stackValue / horizon;
						}
					}
					rawValue += value;
					if(trace)
						trace->record("spell.slow.stack", std::to_string(original->unitId()), value);
				}
			}
			if(spell->isDamage())
			{
				// The executable values every damaged stack with the tactical
				// combat-loss function.  A generic before/after utility delta is
				// observably wrong for partial hit points and for the group-wide
				// friendly-fire tests below.
				int64_t enemyDamageValue = 0;
				int64_t friendlyDamageValue = 0;
				int64_t enemyTotalValue = 0;
				int64_t friendlyTotalValue = 0;
				for(const CStack * originalTarget : battle->battleGetAllStacks(false))
				{
					const BattleSide targetSide = battle->playerToSide(
						battle->battleGetOwner(originalTarget));
					const int64_t totalValue = combatValue.stackValue(originalTarget, parameters);
					if(targetSide == side)
						friendlyTotalValue += totalValue;
					else
						enemyTotalValue += totalValue;

					const battle::Unit * resultingTarget = state->battleGetUnitByID(
						originalTarget->unitId());
					const int64_t beforeHealth = originalTarget->getAvailableHealth();
					const int64_t afterHealth = resultingTarget && resultingTarget->alive()
						? resultingTarget->getAvailableHealth()
						: 0;
					if(afterHealth >= beforeHealth)
						continue;
					int64_t targetValue = combatValue.lossValue(
						originalTarget, beforeHealth, afterHealth, parameters);
					if(targetValue > 0
					   && (!originalTarget->canMove()
						   || originalTarget->isFirstAidTent()
						   || originalTarget->isAmmoCart()))
					{
						targetValue = 2 * targetValue - totalValue;
					}
					if(targetSide == side)
						friendlyDamageValue += targetValue;
					else
						enemyDamageValue += targetValue;
				}

				rawValue = enemyDamageValue - friendlyDamageValue;
				if(spellIndex >= 24 && spellIndex <= 26)
				{
					// Death Ripple, Destroy Undead, and Armageddon are accepted
					// only when they hurt the enemy both absolutely and by a
					// greater share of its army.  Use products to preserve the
					// original integer comparison without division rounding.
					const int64_t friendlyLoss = std::max<int64_t>(0, friendlyDamageValue);
					const bool safeMassDamage = enemyDamageValue > 0
						&& enemyDamageValue > friendlyLoss
						&& friendlyLoss < friendlyTotalValue
						&& enemyTotalValue > 0
						&& friendlyTotalValue > 0
						&& enemyDamageValue * friendlyTotalValue
							> friendlyLoss * enemyTotalValue;
					if(!safeMassDamage)
						rawValue = 0;
				}
			}
			if(rawValue <= 0)
				continue;
			if(trace)
			{
				const std::string key = std::to_string(spellIndex) + ":"
					+ (target.empty() ? "-1" : std::to_string(target.front().hexValue.toInt()));
				trace->record("spell.raw", key, rawValue);
			}
			if(!spellBest.valid || rawValue > spellBest.rawValue)
			{
				spellBest.valid = true;
				spellBest.rawValue = rawValue;
				spellBest.action.actionType = EActionType::HERO_SPELL;
				spellBest.action.spell = spell->id;
				spellBest.action.setTarget(target);
				spellBest.action.side = side;
				spellBest.action.stackNumber = -1;
			}
		}
		if(!spellBest.valid)
			continue;
		const int64_t adjusted = manaAdjustedValue(spellBest.rawValue, hero->mana, cost);
		const int32_t randomPercent = randomGenerator->nextIntInclusive(75, 100);
		spellBest.score = adjusted * randomPercent / 100;
		if(trace)
		{
			const std::string key = std::to_string(spellIndex) + ":"
				+ (spellBest.action.target.empty()
					? "-1"
					: std::to_string(spellBest.action.target.front().hexValue.toInt()));
			trace->record("spell.random", key, randomPercent);
			trace->record("spell.score", key, spellBest.score);
		}
		if(!best.valid || spellBest.score > best.score)
			best = spellBest;
	}
	return best;
}

bool ClassicSpellEvaluator::canChooseHeroSpell(BattleSide side) const
{
	const CGHeroInstance * hero = battle->battleGetFightingHero(side);
	return hero && !battle->battleTacticDist()
		&& battle->battleCanCastSpell(hero, spells::Mode::HERO) == ESpellCastProblem::OK;
}

ClassicScoredSpell ClassicSpellEvaluator::chooseRandomBeneficialSpell(
	const CStack * caster, int64_t competingValue) const
{
	ClassicScoredSpell best;
	ClassicBattleStateView view(battle);
	const BattleSide casterSide = view.controllingSide(caster);
	const auto randomCaster = caster->getBonusesOfType(BonusType::RANDOM_SPELLCASTER)->front();
	for(const CStack * subject : view.orderedFriendlies(caster))
	{
		const auto candidates = battle->getPossibleBeneficialSpells(caster, subject);
		if(candidates.empty())
			continue;
		spells::Target target;
		target.emplace_back(subject);
		int64_t totalValue = 0;
		for(SpellID spellID : candidates)
		{
			const CSpell * spell = spellID.toSpell();
			auto state = std::make_shared<HypotheticBattle>(env, battle);
			const battle::Unit * stateCaster = state->battleGetUnitByID(caster->unitId());
			spells::BattleCast cast(state.get(), stateCaster, spells::Mode::CREATURE_ACTIVE, spell);
			cast.setSpellLevel(std::max(randomCaster->val, caster->getSpellSchoolLevel(spell)));
			cast.castEval(state->getServerCallback(), target);
			totalValue += evaluateStateChange(*state, casterSide);
		}
		// The server selects a uniformly random eligible buff after receiving
		// the target. Compare expected values without drawing a client-side spell.
		const int64_t score = totalValue / static_cast<int64_t>(candidates.size());
		if(score <= 0 || score <= competingValue || (best.valid && score <= best.score))
			continue;
		best.valid = true;
		best.rawValue = score;
		best.score = score;
		best.action = BattleAction::makeCreatureSpellcast(caster, target, SpellID::NONE);
	}
	return best;
}

ClassicScoredSpell ClassicSpellEvaluator::chooseCreatureSpell(
	const CStack * caster,
	int64_t competingValue,
	const ClassicCombatParameters & parameters) const
{
	ClassicScoredSpell best;
	if(!caster->canCast())
		return best;
	const int32_t creatureID = caster->creatureId().getNum();
	if(competingValue != 0 && (creatureID == 37 || creatureID == 91))
	{
		const int32_t roll = randomGenerator->nextIntInclusive(1, 100);
		if(trace)
			trace->record("creatureSpell", "declineRoll", roll);
		if(roll <= 30)
			return best;
	}
	if(caster->hasBonusOfType(BonusType::RANDOM_SPELLCASTER))
		return chooseRandomBeneficialSpell(caster, competingValue);

	std::vector<SpellID> spells;
	for(const auto & bonus : *caster->getBonusesOfType(BonusType::SPELLCASTER))
	{
		if(!bonus->parameters && bonus->subtype.as<SpellID>().hasValue())
			spells.push_back(bonus->subtype.as<SpellID>());
	}
	ClassicVstdRngAdapter rng(*randomGenerator);
	const SpellID randomSpell = battle->getRandomCastedSpell(rng, caster);
	if(randomSpell.hasValue())
		spells.push_back(randomSpell);
	std::stable_sort(
		spells.begin(),
		spells.end(),
		[](SpellID lhs, SpellID rhs)
		{
			return lhs.getNum() < rhs.getNum();
		}
	);
	spells.erase(std::unique(spells.begin(), spells.end()), spells.end());

	for(SpellID spellID : spells)
	{
		const CSpell * spell = spellID.toSpell();
		if(!spell || !spell->canBeCast(battle.get(), spells::Mode::CREATURE_ACTIVE, caster))
			continue;
		spells::BattleCast castInfo(battle.get(), caster, spells::Mode::CREATURE_ACTIVE, spell);
		auto mechanics = spell->battleMechanics(&castInfo);
		for(const spells::Target & target : enumerateTargets(*mechanics))
		{
			auto state = std::make_shared<HypotheticBattle>(env, battle);
			const battle::Unit * stateCaster = state->battleGetUnitByID(caster->unitId());
			spells::BattleCast cast(state.get(), stateCaster, spells::Mode::CREATURE_ACTIVE, spell);
			cast.castEval(state->getServerCallback(), target);
			int64_t score = evaluateStateChange(
				*state, battle->playerToSide(battle->battleGetOwner(caster)));
			const bool faerieDragon = creatureID == 134;
			const int32_t spellIndex = spellID.getNum();
			const bool directDamageSpell = spellIndex >= 15 && spellIndex <= 18;
			if(faerieDragon && directDamageSpell
			   && target.size() == 1 && target.front().unitValue)
			{
				const uint32_t targetId = target.front().unitValue->unitId();
				const CStack * originalTarget = battle->battleGetStackByID(targetId, false);
				const battle::Unit * resultingTarget = state->battleGetUnitByID(targetId);
				if(originalTarget)
				{
					const int64_t afterHealth = resultingTarget && resultingTarget->alive()
						? resultingTarget->getAvailableHealth()
						: 0;
					ClassicCombatValue combatValue(battle);
					score = combatValue.lossValue(
						originalTarget,
						originalTarget->getAvailableHealth(),
						afterHealth,
						parameters);
					const BattleSide casterSide = battle->playerToSide(battle->battleGetOwner(caster));
					const BattleSide targetSide = battle->playerToSide(
						battle->battleGetOwner(originalTarget));
					if(targetSide == casterSide)
						score = -score;
					if(score > 0
					   && (!originalTarget->canMove()
						   || originalTarget->isFirstAidTent()
						   || originalTarget->isAmmoCart()))
					{
						score = 2 * score
							- combatValue.stackValue(originalTarget, parameters);
					}
				}
			}
			if(score <= 0
			   || (!faerieDragon && score <= competingValue)
			   || (best.valid && score <= best.score))
				continue;
			best.valid = true;
			best.rawValue = score;
			best.score = score;
			best.action = BattleAction::makeCreatureSpellcast(caster, target, spellID);
		}
	}
	return best;
}
