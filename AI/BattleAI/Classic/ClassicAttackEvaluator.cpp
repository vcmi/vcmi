/*
 * ClassicAttackEvaluator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "../StdInc.h"
#include "ClassicAttackEvaluator.h"

#include "../../../lib/CStack.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/battle/CBattleInfoCallback.h"
#include "../../../lib/battle/CObstacleInstance.h"
#include "../../../lib/battle/CUnitState.h"
#include "../../../lib/battle/ReachabilityInfo.h"
#include "../../../lib/bonuses/BonusSelector.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "../../../lib/mapObjects/CGTownInstance.h"
#include "../../../lib/spells/CSpell.h"
#include "../../../lib/spells/ISpellMechanics.h"
#include "ClassicBattleRng.h"
#include "ClassicBattleStateView.h"
#include "ClassicDecisionTrace.h"
#include "ClassicRulesAdapter.h"

namespace
{
constexpr std::array<EWallPart, 7> WALL_TARGET_ORDER = {
	EWallPart::KEEP,
	EWallPart::BOTTOM_TOWER,
	EWallPart::UPPER_TOWER,
	EWallPart::BELOW_GATE,
	EWallPart::OVER_GATE,
	EWallPart::BOTTOM_WALL,
	EWallPart::UPPER_WALL
};

int32_t turnsToReach(uint32_t distance, uint32_t speed)
{
	if(distance == 0)
		return 0;
	if(distance == ReachabilityInfo::INFINITE_DIST || speed == 0)
		return std::numeric_limits<int32_t>::max();
	return static_cast<int32_t>((distance + speed - 1) / speed);
}

int32_t spellDuration(const CStack * stack, SpellID spell)
{
	const auto bonuses = stack->getBonuses(
		Selector::source(BonusSource::SPELL_EFFECT, BonusSourceID(spell)));
	int32_t result = 0;
	for(const auto & bonus : *bonuses)
		result = std::max(result, static_cast<int32_t>(bonus->turnsRemain));
	return result;
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

std::vector<BattleHex> originalAttackHexOrder(
	const CStack * attacker,
	const CStack * defender)
{
	const BattleHexArray legal = defender->getAttackableHexes(attacker);
	std::vector<BattleHex> result;
	// find_attack_hex scans the six direction records starting at direction 1,
	// wrapping after direction 5. VCMI's BattleHexArray starts at direction 0;
	// preserving membership while restoring this rotation is observable whenever
	// path cost and danger tie (notably a target on the right battlefield edge).
	static constexpr std::array<BattleHex::EDir, 6> directions = {
		BattleHex::EDir::TOP_RIGHT,
		BattleHex::EDir::RIGHT,
		BattleHex::EDir::BOTTOM_RIGHT,
		BattleHex::EDir::BOTTOM_LEFT,
		BattleHex::EDir::LEFT,
		BattleHex::EDir::TOP_LEFT
	};
	for(const BattleHex & occupied : defender->getHexes())
	{
		for(const BattleHex::EDir direction : directions)
		{
			const BattleHex candidate = occupied.cloneInDirection(direction, false);
			if(legal.contains(candidate) && !vstd::contains(result, candidate))
				result.push_back(candidate);
		}
	}
	for(const BattleHex & candidate : legal)
		if(!vstd::contains(result, candidate))
			result.push_back(candidate);
	return result;
}

std::shared_ptr<battle::CUnitState> stateAtHealth(
	const battle::Unit * stack,
	int64_t health,
	const BattleHex & position = BattleHex::INVALID)
{
	auto state = stack->acquireState();
	health = std::max<int64_t>(0, health);
	JsonNode serialized = state->save();
	const int64_t hitPoints = std::max<int64_t>(1, state->getMaxHealth());
	serialized["state"]["health"]["firstHPleft"].Integer() = health == 0 ? 0 : (health - 1) % hitPoints + 1;
	serialized["state"]["health"]["fullUnits"].Integer() = health == 0 ? 0 : (health - 1) / hitPoints;
	serialized["state"]["health"]["resurrected"].Integer() = 0;
	state->load(serialized);
	if(position.isValid())
		state->setPosition(position);
	return state;
}

std::optional<double> exactSecondarySkillDamageFactor(
	const battle::Unit * attacker,
	bool ranged)
{
	const BonusSubtypeID subtype(ranged
		? BonusCustomSubtype::damageTypeRanged
		: BonusCustomSubtype::damageTypeMelee);
	const auto bonuses = attacker->getBonuses(
		Selector::typeSubtype(BonusType::PERCENTAGE_DAMAGE_BOOST, subtype));

	int32_t percentToSecondarySkill = 0;
	for(const auto & bonus : *bonuses)
	{
		if(bonus->valType == BonusValueType::PERCENT_TO_TARGET_TYPE
			&& bonus->targetSourceType == BonusSource::SECONDARY_SKILL)
		{
			percentToSecondarySkill += bonus->val;
		}
	}
	if(percentToSecondarySkill == 0)
		return std::nullopt;

	// BonusList deliberately exposes integral values. The original damage
	// pipeline instead retains the fraction introduced by a secondary-skill
	// specialty: e.g. Orrin level 1 turns Basic Archery's 10% into 10.5%.
	double fractionalRemainder = 0.0;
	for(const auto & bonus : *bonuses)
	{
		if(bonus->source != BonusSource::SECONDARY_SKILL)
			continue;
		if(bonus->valType != BonusValueType::BASE_NUMBER)
			continue;
		const int64_t numerator = static_cast<int64_t>(bonus->val)
			* (100 + percentToSecondarySkill);
		fractionalRemainder += static_cast<double>(numerator) / 100.0
			- static_cast<double>(numerator / 100);
	}
	return (static_cast<double>(bonuses->totalValue()) + fractionalRemainder) / 100.0;
}
}

ClassicAttackEvaluator::ClassicAttackEvaluator(
	std::shared_ptr<CBattleInfoCallback> battle,
	std::shared_ptr<IClassicBattleAIRng> randomGenerator,
	std::shared_ptr<ClassicDecisionTrace> trace,
	bool preserveLongMoveWait,
	bool secondPhase
)
	: battle(std::move(battle)),
	randomGenerator(std::move(randomGenerator)),
	trace(std::move(trace)),
	combatValue(this->battle),
	preserveLongMoveWait(preserveLongMoveWait),
	secondPhase(secondPhase)
{
}

bool ClassicAttackEvaluator::shouldReplaceShooterTarget(
	int64_t candidateScore,
	bool candidateDisabled,
	int64_t bestScore,
	bool bestDisabled)
{
	if(candidateDisabled && bestDisabled)
		return false;
	if(!candidateDisabled && bestDisabled)
		return true;
	return candidateScore >= bestScore;
}

int64_t ClassicAttackEvaluator::averageDamage(
	const CBattleInfoCallback & scenario,
	const battle::Unit * attacker,
	int64_t attackerHealth,
	const battle::Unit * defender,
	int64_t defenderHealth,
	const BattleHex & attackerPosition,
	bool ranged,
	int32_t distance) const
{
	auto attackerState = stateAtHealth(attacker, attackerHealth, attackerPosition);
	auto defenderState = stateAtHealth(defender, defenderHealth);
	if(!attackerState->alive() || !defenderState->alive())
		return 0;

	BattleAttackInfo attackInfo(attackerState.get(), defenderState.get(), distance, ranged);
	attackInfo.attackerPos = attackerState->getPosition();
	attackInfo.defenderPos = defenderState->getPosition();
	const bool cursed = attackerState->hasBonusOfType(BonusType::ALWAYS_MINIMUM_DAMAGE);
	const bool blessed = attackerState->hasBonusOfType(BonusType::ALWAYS_MAXIMUM_DAMAGE);
	const int32_t shift = attackerState->valOfBonuses(BonusType::ALWAYS_MAXIMUM_DAMAGE)
		- attackerState->valOfBonuses(BonusType::ALWAYS_MINIMUM_DAMAGE);
	const int64_t minimum = std::max<int64_t>(1, attackerState->getMinDamage(ranged) + shift);
	const int64_t maximum = std::max<int64_t>(1, attackerState->getMaxDamage(ranged) + shift);
	const int64_t perCreatureTwice = cursed != blessed
		? 2 * (cursed ? minimum : maximum)
		: minimum + maximum;
	attackInfo.baseDamageOverride = perCreatureTwice * attackerState->getCount() / 2;
	if(const auto exactFactor = exactSecondarySkillDamageFactor(attackerState.get(), ranged))
		attackInfo.offenseArcheryFactorOverride = *exactFactor;
	const DamageEstimation estimate = scenario.battleEstimateDamage(attackInfo);
	return estimate.damage.min;
}

int64_t ClassicAttackEvaluator::averageDamage(
	const CStack * attacker,
	int64_t attackerHealth,
	const CStack * defender,
	int64_t defenderHealth,
	const BattleHex & attackerPosition,
	bool ranged,
	int32_t distance) const
{
	return averageDamage(
		*battle, attacker, attackerHealth, defender, defenderHealth,
		attackerPosition, ranged, distance);
}

int64_t ClassicAttackEvaluator::fireShieldDamage(
	const CBattleInfoCallback & scenario,
	const battle::Unit * shieldBearer,
	int64_t shieldBearerHealth,
	const battle::Unit * recipient,
	int64_t incomingDamage) const
{
	if(shieldBearerHealth <= 0 || incomingDamage <= 0)
		return 0;

	// get_fire_shield_damage first rejects natural fire immunity, before it
	// enters the generic spell-damage pipeline.
	if(recipient->hasBonusOfType(
		   BonusType::SPELL_SCHOOL_IMMUNITY,
		   BonusSubtypeID(SpellSchool::FIRE)))
	{
		return 0;
	}

	int32_t strength = shieldBearer->creatureId() == CreatureID(53) ? 20 : 0;
	const CSelector spellShield = Selector::source(
		BonusSource::SPELL_EFFECT,
		BonusSourceID(SpellID(SpellID::FIRE_SHIELD)));
	strength = std::max(strength, shieldBearer->valOfBonuses(spellShield));
	if(strength <= 0)
		return 0;

	const int64_t reflectable = std::min(incomingDamage, shieldBearerHealth);
	const int64_t rawDamage = ClassicCombatValue::truncateTowardZero(
		static_cast<double>(reflectable) * strength / 100.0);
	if(rawDamage <= 0)
		return 0;

	const CSpell * fireShield = SpellID(SpellID::FIRE_SHIELD).toSpell();
	if(!fireShield)
		return rawDamage;
	const battle::Unit * originalShieldBearer = battle->battleGetUnitByID(shieldBearer->unitId());
	const CGHeroInstance * hero = originalShieldBearer
		? battle->battleGetOwnerHero(originalShieldBearer)
		: nullptr;
	const spells::Caster * caster = hero
		? static_cast<const spells::Caster *>(hero)
		: static_cast<const spells::Caster *>(shieldBearer);
	return std::max<int64_t>(0, fireShield->adjustRawDamage(caster, recipient, rawDamage));
}

ClassicAttackEvaluator::SimulatedAttack ClassicAttackEvaluator::simulateAttack(
	const CBattleInfoCallback & scenario,
	const battle::Unit * attacker,
	const battle::Unit * defender,
	const BattleHex & attackFrom,
	bool ranged,
	int32_t distance,
	int64_t attackerHealth,
	int64_t defenderHealth,
	RetaliationMode retaliationMode) const
{
	SimulatedAttack result;
	result.attackerBefore = attackerHealth >= 0
		? attackerHealth
		: attacker->isClone() ? 1 : attacker->getAvailableHealth();
	result.defenderBefore = defenderHealth >= 0
		? defenderHealth
		: defender->isClone() ? 1 : defender->getAvailableHealth();
	result.attackerAfter = result.attackerBefore;
	result.defenderAfter = result.defenderBefore;

	// The executable accepts a requested ranged mode, then downgrades it if
	// the active stack cannot currently shoot.
	if(ranged && !scenario.battleCanShoot(attacker, defender->getPosition()))
	{
		// A spell-like area shot can affect a friendly secondary stack even
		// though that stack is not itself a legal primary shooting target. The
		// primary shot has already established that the attacker can fire; retain
		// ranged damage for physical-side friendly splash victims.
		const bool friendlyAreaSecondary =
			attacker->unitSide() == defender->unitSide()
			&& scenario.battleCanShoot(attacker);
		if(!friendlyAreaSecondary)
			ranged = false;
	}

	result.firstStrike = std::min(result.defenderAfter, averageDamage(
		scenario,
		attacker,
		result.attackerAfter,
		defender,
		result.defenderAfter,
		attackFrom,
		ranged,
		distance));
	if(!ranged)
	{
		result.firstFireShield = fireShieldDamage(
			scenario, defender, result.defenderAfter, attacker, result.firstStrike);
		result.attackerAfter = std::max<int64_t>(0, result.attackerAfter - result.firstFireShield);
	}
	result.defenderAfter = std::max<int64_t>(0, result.defenderAfter - result.firstStrike);

	const bool blocksRetaliation = attacker->hasBonusOfType(BonusType::BLOCKS_RETALIATION);
	if(result.attackerAfter > 0
	   && result.defenderAfter > 0
	   && !ranged
	   && !blocksRetaliation)
	{
		auto defenderState = stateAtHealth(defender, result.defenderAfter);
		const bool canRetaliate = retaliationMode == RetaliationMode::ALWAYS
			|| (retaliationMode == RetaliationMode::NORMAL && defenderState->ableToRetaliate());
		if(canRetaliate)
		{
			result.retaliation = std::min(result.attackerAfter, averageDamage(
				scenario,
				defender,
				result.defenderAfter,
				attacker,
				result.attackerAfter,
				defender->getPosition(),
				false,
				0));
			result.retaliationFireShield = fireShieldDamage(
				scenario, attacker, result.attackerAfter, defender, result.retaliation);
			result.defenderAfter = std::max<int64_t>(0, result.defenderAfter - result.retaliationFireShield);
			result.attackerAfter = std::max<int64_t>(0, result.attackerAfter - result.retaliation);
		}
	}

	if(result.attackerAfter > 0
	   && result.defenderAfter > 0
	   && attacker->getTotalAttacks(ranged) > 1)
	{
		// Jousting distance applies only to the first blow. The second strike is
		// recalculated from both stacks' remaining modeled hit points.
		result.secondStrike = std::min(result.defenderAfter, averageDamage(
			scenario,
			attacker,
			result.attackerAfter,
			defender,
			result.defenderAfter,
			attackFrom,
			ranged,
			0));
		if(!ranged)
		{
			result.secondFireShield = fireShieldDamage(
				scenario, defender, result.defenderAfter, attacker, result.secondStrike);
			result.attackerAfter = std::max<int64_t>(0, result.attackerAfter - result.secondFireShield);
		}
		result.defenderAfter = std::max<int64_t>(0, result.defenderAfter - result.secondStrike);
	}
	return result;
}

ClassicAttackEvaluator::SimulatedAttack ClassicAttackEvaluator::simulateAttack(
	const CStack * attacker,
	const CStack * defender,
	const BattleHex & attackFrom,
	bool ranged,
	int32_t distance,
	int64_t attackerHealth,
	int64_t defenderHealth) const
{
	return simulateAttack(
		*battle, attacker, defender, attackFrom, ranged, distance, attackerHealth, defenderHealth);
}

ClassicProjectedTargets ClassicAttackEvaluator::findProjectedTargets(
	BattleSide actorPhysicalSide,
	const ClassicCombatParameters & parameters,
	const CStack * excluded,
	bool onlyPriorTargets) const
{
	ClassicProjectedTargets result;
	ClassicBattleStateView view(battle);
	std::map<uint32_t, uint32_t> moveRanks;
	if(onlyPriorTargets)
	{
		for(const ClassicMoveOrderEntry & entry : view.moveOrder(actorPhysicalSide, secondPhase))
			moveRanks[entry.stack->unitId()] = entry.order;
	}

	const auto stacks = view.orderedStacks(false, true);
	for(const CStack * actor : stacks)
	{
		if(actor->unitSide() != actorPhysicalSide)
			continue;

		// find_AI_targets clears these fields before applying its actor filters.
		// The compatibility implementation keeps the same zero record locally
		// instead of mutating the authoritative VCMI battle state.
		ClassicProjectedTarget & record = result[actor->unitId()];
		if(actor == excluded
		   || modeledHealth(actor, parameters) <= 0
		   || (actor->hasBonusOfType(BonusType::SIEGE_WEAPON) && !actor->isBallista())
		   || spellDuration(actor, SpellID(SpellID::BLIND)) > 1
		   || spellDuration(actor, SpellID(SpellID::STONE_GAZE)) > 1
		   || actor->isHypnotized()
		   || actor->hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE))
		{
			continue;
		}

		// find_AI_targets tests the creature's shooter flag, not whether the
		// stack can fire from its current cell. A blocked shooter is therefore
		// still a ranged actor here and is excluded from the prior-melee-target
		// projection used by get_attack_change.
		const bool ranged = actor->isShooter();
		if(ranged && onlyPriorTargets)
			continue;
		const uint32_t speed = actor->getMovementRange();
		const ReachabilityInfo reachability = battle->getReachability(actor);

		for(const CStack * candidate : stacks)
		{
			if(candidate->unitSide() == actorPhysicalSide
			   || !candidate->alive()
			   || modeledHealth(candidate, parameters) <= 0
			   || candidate->isTurret())
			{
				continue;
			}
			if(onlyPriorTargets
			   && moveRanks[candidate->unitId()] >= moveRanks[actor->unitId()])
			{
				continue;
			}

			uint32_t distance = ranged ? 1 : ReachabilityInfo::INFINITE_DIST;
			BattleHex landing = BattleHex::INVALID;
			if(!ranged)
			{
				for(const BattleHex & candidateLanding : candidate->getAttackableHexes(actor))
				{
					if(!candidateLanding.isAvailable() || !reachability.isReachable(candidateLanding))
						continue;
					const uint32_t candidateDistance = reachability.distances[candidateLanding.toInt()];
					const uint32_t limit = onlyPriorTargets ? speed : 127;
					if(candidateDistance > limit || candidateDistance >= distance)
						continue;
					distance = candidateDistance;
					landing = candidateLanding;
				}
				if(distance == ReachabilityInfo::INFINITE_DIST)
					continue;
			}

			const int32_t effectDistance = !ranged && distance > speed
				? 0
				: static_cast<int32_t>(distance);
			const int64_t value = attackValue(
				actor,
				candidate,
				landing,
				ranged,
				parameters,
				effectDistance);
			const int32_t attackTime = actor->hasBonusOfType(BonusType::SIEGE_WEAPON) || speed == 0
				? 1
				: std::max(1, turnsToReach(distance, speed));
			if(attackTime == 1)
			{
				const int32_t slot = candidate->unitSlot().validSlot()
					? candidate->unitSlot().getNum()
					: static_cast<int32_t>(candidate->unitId());
				if(slot >= 0 && slot < 32)
					record.possibleTargets |= uint32_t(1) << slot;
			}
			if(!record.target
			   || attackTime < record.attackTime
			   || (attackTime == record.attackTime && value > record.value))
			{
				record.target = candidate;
				record.value = value;
				record.distance = static_cast<int32_t>(distance);
				record.attackTime = attackTime;
			}
		}
		if(trace)
		{
			trace->record("projection.target", std::to_string(actor->unitId()),
				record.target ? static_cast<int64_t>(record.target->unitId()) : -1);
			trace->record("projection.target_time", std::to_string(actor->unitId()), record.attackTime);
		}
	}
	return result;
}

ClassicProjectedTargets ClassicAttackEvaluator::projectSpellTargets(
	BattleSide physicalSide,
	const ClassicCombatParameters & parameters) const
{
	return findProjectedTargets(physicalSide, parameters);
}

int64_t ClassicAttackEvaluator::rangedTargetValue(
	const CStack * attacker,
	const CStack * target,
	const ClassicCombatParameters & parameters,
	const ClassicProjectedTargets & projection,
	const std::string * areaTraceKey) const
{
	int64_t score = attackValue(
		attacker, target, BattleHex::INVALID, true, parameters, -1, areaTraceKey);
	// Blind, Stone Gaze, and Paralyze share the executable's immediate
	// one-tenth urgency path. Other targets are divided by the time until
	// their projected attack, capped at five; no projection is equivalent
	// to the five-turn case.
	const int32_t blindDuration = spellDuration(target, SpellID(SpellID::BLIND));
	const int32_t stoneDuration = spellDuration(target, SpellID(SpellID::STONE_GAZE));
	const int32_t paralyzeDuration = spellDuration(target, SpellID(SpellID::PARALYZE));
	const bool disabledByScoredSpell = blindDuration > 0 || stoneDuration > 0 || paralyzeDuration > 0;
	if(trace)
	{
		trace->record("shoot.blind_duration", std::to_string(target->unitId()), blindDuration);
		trace->record("shoot.stone_duration", std::to_string(target->unitId()), stoneDuration);
		trace->record("shoot.paralyze_duration", std::to_string(target->unitId()), paralyzeDuration);
	}
	if(disabledByScoredSpell)
		score /= 10;
	else
	{
		const auto found = projection.find(target->unitId());
		const int32_t targetTime = found == projection.end() ? 0 : found->second.attackTime;
		score /= targetTime > 0 && targetTime <= 5 ? targetTime : 5;
		if(trace)
			trace->record("shoot.target_time", std::to_string(target->unitId()), targetTime);
	}
	return score;
}

int64_t ClassicAttackEvaluator::areaShotValue(
	const CStack * attacker,
	const BattleHex & center,
	const ClassicCombatParameters & parameters,
	const ClassicProjectedTargets & projection) const
{
	int64_t result = 0;
	std::set<uint32_t> affected;
	auto addTarget = [&](const CStack * target, bool eligible = true)
	{
		if(!target || !affected.insert(target->unitId()).second)
			return;
		const std::string key = std::to_string(target->getPosition().toInt());
		if(trace)
			trace->record("area_shot.consider", key, target->creatureId().getNum());
		if(!eligible)
		{
			if(trace)
				trace->record("area_shot.excluded", key, 0);
			return;
		}
		const bool friendly = target->unitSide() == attacker->unitSide();
		if(trace)
			trace->record("area_shot.eligible", key, friendly ? -1 : 1);
		const int64_t unsignedValue = friendly
			? attackValue(attacker, target, BattleHex::INVALID, true, parameters, -1, &key)
			: rangedTargetValue(attacker, target, parameters, projection, &key);
		const int64_t signedValue = friendly ? -unsignedValue : unsignedValue;
		result += signedValue;
		if(trace)
		{
			trace->record("area_shot.effect", key, signedValue);
			trace->record("area_shot.total", key, result);
		}
	};

	// SoD's area-shot vector includes the impact occupant. VCMI's generic
	// battleGetAttackedHexes callback follows melee geometry and therefore does
	// not expose FIREBALL/DEATH_CLOUD secondary targets at shooting range. Ask
	// the stack's spell-like attack mechanics for its receptive affected stacks;
	// this also preserves Death Cloud's living/undead eligibility. Keep the
	// ordinary callback as a defensive fallback for a modded area-shooter rule.
	const auto spellLike = attacker->getBonus(Selector::type()(BonusType::SPELL_LIKE_ATTACK));
	bool usedSpellLikeGeometry = false;
	if(spellLike)
	{
		const CSpell * spell = spellLike->subtype.as<SpellID>().toSpell();
		if(spell)
		{
			spells::BattleCast cast(battle.get(), attacker, spells::Mode::SPELL_LIKE_ATTACK, spell);
			auto mechanics = spell->battleMechanics(&cast);
			usedSpellLikeGeometry = true;
			const BattleHexArray area = mechanics->rangeInHexes(center);
			for(auto hex = area.rbegin(); hex != area.rend(); ++hex)
			{
				const CStack * affectedStack = battle->battleGetStackByPos(*hex, true);
				addTarget(affectedStack, !affectedStack || mechanics->isReceptive(affectedStack));
			}
		}
	}
	if(!usedSpellLikeGeometry)
	{
		addTarget(battle->battleGetStackByPos(center, true));
		for(const BattleHex & hex : battle->battleGetAttackedHexes(attacker, center))
			addTarget(battle->battleGetStackByPos(hex, true));
	}
	if(trace)
		trace->record("area_shot.final", std::to_string(center.toInt()), result);
	return result;
}

int64_t ClassicAttackEvaluator::attackValue(
	const CStack * attacker,
	const CStack * defender,
	const BattleHex & attackFrom,
	bool ranged,
	const ClassicCombatParameters & parameters,
	int32_t distanceOverride,
	const std::string * areaTraceKey
) const
{
	const int distance = distanceOverride >= 0
		? distanceOverride
		: ranged || !attackFrom.isValid() ? 0 : BattleHex::getDistance(attacker->getPosition(), attackFrom);
	const SimulatedAttack simulation = simulateAttack(
		attacker, defender, attackFrom, ranged, distance,
		modeledHealth(attacker, parameters), modeledHealth(defender, parameters));
	const int64_t defenderLoss = combatValue.lossValue(
		defender,
		simulation.defenderBefore,
		simulation.defenderAfter,
		parameters,
		ranged,
		parameters.killsOnly);
	const int64_t attackerLoss = combatValue.lossValue(
		attacker,
		simulation.attackerBefore,
		simulation.attackerAfter,
		parameters,
		false,
		false);
	const int64_t result = defenderLoss - attackerLoss;
	if(trace && areaTraceKey)
	{
		trace->record("area_shot.damage", *areaTraceKey, simulation.firstStrike);
		trace->record("area_shot.defender_after", *areaTraceKey, simulation.defenderAfter);
		trace->record("area_shot.loss", *areaTraceKey, defenderLoss);
	}
	if(trace)
	{
		const std::string key = std::to_string(attacker->unitId()) + ":" + std::to_string(defender->unitId());
		trace->record("attack_sim.first", key, simulation.firstStrike);
		trace->record("attack_sim.fire_first", key, simulation.firstFireShield);
		trace->record("attack_sim.retaliation", key, simulation.retaliation);
		trace->record("attack_sim.fire_retaliation", key, simulation.retaliationFireShield);
		trace->record("attack_sim.second", key, simulation.secondStrike);
		trace->record("attack_sim.fire_second", key, simulation.secondFireShield);
		trace->record("attack_sim.attacker_after", key, simulation.attackerAfter);
		trace->record("attack_sim.defender_after", key, simulation.defenderAfter);
		trace->record("attack_sim.value", key, result);
	}

	return result;
}

int64_t ClassicAttackEvaluator::projectedExchangeValue(
	const CBattleInfoCallback & scenario,
	const battle::Unit * attacker,
	const battle::Unit * defender,
	const CStack * originalAttacker,
	const CStack * originalDefender,
	const BattleHex & attackFrom,
	bool ranged,
	int32_t distance,
	const ClassicCombatParameters & parameters) const
{
	const ClassicProjectedExchange exchange = projectedExchangeLosses(
		scenario,
		attacker,
		defender,
		originalAttacker,
		originalDefender,
		attackFrom,
		ranged,
		distance,
		parameters);
	return exchange.defenderLoss - exchange.attackerLoss;
}

ClassicProjectedExchange ClassicAttackEvaluator::projectedExchangeLosses(
	const CBattleInfoCallback & scenario,
	const battle::Unit * attacker,
	const battle::Unit * defender,
	const CStack * originalAttacker,
	const CStack * originalDefender,
	const BattleHex & attackFrom,
	bool ranged,
	int32_t distance,
	const ClassicCombatParameters & parameters) const
{
	ClassicProjectedExchange result;
	if(!attacker || !defender || !originalAttacker || !originalDefender)
		return result;
	const SimulatedAttack simulation = simulateAttack(
		scenario,
		attacker,
		defender,
		attackFrom,
		ranged,
		distance,
		modeledHealth(originalAttacker, parameters),
		modeledHealth(originalDefender, parameters));
	result.defenderLoss = combatValue.lossValue(
		originalDefender,
		simulation.defenderBefore,
		simulation.defenderAfter,
		parameters,
		ranged,
		parameters.killsOnly);
	result.attackerLoss = combatValue.lossValue(
		originalAttacker,
		simulation.attackerBefore,
		simulation.attackerAfter,
		parameters,
		false,
		false);
	return result;
}

int64_t ClassicAttackEvaluator::projectedRetaliationValue(
	const CBattleInfoCallback & scenario,
	const battle::Unit * attacker,
	const battle::Unit * defender,
	const CStack * originalAttacker,
	const CStack * originalDefender,
	const BattleHex & attackFrom,
	int32_t distance,
	const ClassicCombatParameters & parameters) const
{
	if(!attacker || !defender || !originalAttacker || !originalDefender)
		return 0;
	auto exchange = [&](RetaliationMode mode)
	{
		const SimulatedAttack simulation = simulateAttack(
			scenario,
			attacker,
			defender,
			attackFrom,
			false,
			distance,
			modeledHealth(originalAttacker, parameters),
			modeledHealth(originalDefender, parameters),
			mode);
		ClassicProjectedExchange result;
		result.defenderLoss = combatValue.lossValue(
			originalDefender,
			simulation.defenderBefore,
			simulation.defenderAfter,
			parameters,
			false,
			parameters.killsOnly);
		result.attackerLoss = combatValue.lossValue(
			originalAttacker,
			simulation.attackerBefore,
			simulation.attackerAfter,
			parameters,
			false,
			false);
		return result;
	};
	const ClassicProjectedExchange without = exchange(RetaliationMode::NEVER);
	const ClassicProjectedExchange with = exchange(RetaliationMode::ALWAYS);
	return std::max<int64_t>(
		0,
		with.attackerLoss - without.attackerLoss
			- (with.defenderLoss - without.defenderLoss));
}

int64_t ClassicAttackEvaluator::attackChange(
	const CStack * attacker,
	const CStack * target,
	const ClassicCombatParameters & parameters,
	const ClassicProjectedTargets & projection) const
{
	// The projected attack change is only meaningful while the target has
	// exactly one retaliation and is not petrified. Its purpose is to value how
	// this hit changes the already projected attacks of stacks moving earlier.
	if(spellDuration(target, SpellID(SpellID::STONE_GAZE)) > 0
	   || target->counterAttacks.available() != 1)
	{
		return 0;
	}

	const int64_t targetHealth = modeledHealth(target, parameters);
	const int64_t firstDamage = averageDamage(
		attacker,
		modeledHealth(attacker, parameters),
		target,
		targetHealth,
		attacker->getPosition(),
		false,
		0);
	const int64_t remainingHealth = targetHealth - firstDamage;
	if(remainingHealth <= 0)
		return 0;

	const int32_t targetSlot = target->unitSlot().validSlot()
		? target->unitSlot().getNum()
		: static_cast<int32_t>(target->unitId());
	if(targetSlot < 0 || targetSlot >= 32)
		return 0;

	const int64_t hitPoints = std::max<int64_t>(1, target->getMaxHealth());
	const double targetUnitValue = combatValue.unitValueExact(target, parameters, false);
	int64_t directChanges = 0;
	int64_t bestRedirectedChange = 0;
	ClassicBattleStateView view(battle);
	for(const CStack * projectedAttacker : view.orderedStacks(false, true))
	{
		if(projectedAttacker->unitSide() != attacker->unitSide()
		   || modeledHealth(projectedAttacker, parameters) <= 0
		   || projectedAttacker->isTurret())
		{
			continue;
		}
		const auto found = projection.find(projectedAttacker->unitId());
		if(found == projection.end()
		   || (found->second.possibleTargets & (uint32_t(1) << targetSlot)) == 0)
		{
			continue;
		}

		const int64_t projectedDamage = std::min<int64_t>(
			remainingHealth,
			averageDamage(
				projectedAttacker,
				modeledHealth(projectedAttacker, parameters),
				target,
				targetHealth,
				projectedAttacker->getPosition(),
				false,
				0));
		int64_t changedValue = 0;
		if(parameters.killsOnly)
		{
			const int64_t killed = (remainingHealth % hitPoints + projectedDamage) / hitPoints;
			changedValue = ClassicCombatValue::truncateTowardZero(killed * targetUnitValue);
		}
		else
		{
			changedValue = ClassicCombatValue::truncateTowardZero(
				projectedDamage * targetUnitValue / hitPoints);
		}
		changedValue -= found->second.value;
		if(found->second.target == target && changedValue > 0)
			directChanges += changedValue;
		else
			bestRedirectedChange = std::max(bestRedirectedChange, changedValue);
	}
	return directChanges + bestRedirectedChange;
}

int64_t ClassicAttackEvaluator::spellExchangeEffect(
	const CStack * first,
	const CStack * second,
	const ClassicCombatParameters & parameters) const
{
	if(!first || !second)
		return 0;
	auto exchangeValue = [&](const CStack * attacker, const CStack * defender)
	{
		const int64_t attackerBefore = attacker->getAvailableHealth();
		const int64_t defenderBefore = defender->getAvailableHealth();
		const bool ranged = battle->battleCanShoot(attacker);
		const SimulatedAttack firstExchange = simulateAttack(
			attacker,
			defender,
			attacker->getPosition(),
			ranged,
			0,
			attackerBefore,
			defenderBefore);
		int64_t attackerAfter = firstExchange.attackerAfter;
		int64_t defenderAfter = firstExchange.defenderAfter;
		if(defenderAfter > 0 && attackerAfter > 0)
		{
			const SimulatedAttack answer = simulateAttack(
				defender,
				attacker,
				defender->getPosition(),
				battle->battleCanShoot(defender),
				0,
				defenderAfter,
				attackerAfter);
			defenderAfter = answer.attackerAfter;
			attackerAfter = answer.defenderAfter;
		}
		const int64_t value = combatValue.lossValue(
			defender,
			defenderBefore,
			defenderAfter,
			parameters,
			ranged,
			parameters.killsOnly)
			- combatValue.lossValue(
				attacker,
				attackerBefore,
				attackerAfter,
				parameters,
				ranged,
				parameters.killsOnly);
		if(trace)
		{
			const std::string key = std::to_string(attacker->unitId()) + ":"
				+ std::to_string(defender->unitId());
			trace->record("spell.exchange.attacker_before", key, attackerBefore);
			trace->record("spell.exchange.attacker_after", key, attackerAfter);
			trace->record("spell.exchange.defender_before", key, defenderBefore);
			trace->record("spell.exchange.defender_after", key, defenderAfter);
			trace->record("spell.exchange.value", key, value);
		}
		return value;
	};
	return exchangeValue(first, second) + exchangeValue(second, first);
}

int64_t ClassicAttackEvaluator::landingContextValue(
	const CStack * attacker,
	const BattleHex & landing,
	const ClassicCombatParameters & parameters) const
{
	int64_t result = 0;
	ClassicBattleStateView view(battle);
	const BattleHexArray occupied = battle::Unit::getHexes(
		landing, attacker->doubleWide(), attacker->unitSide());
	for(const CStack * enemy : view.orderedEnemies(attacker))
	{
		if(!enemy->canMove() || modeledHealth(enemy, parameters) <= 0
		   || !enemy->isShooter() || !battle->battleCanShoot(enemy))
			continue;
		bool blockedByLanding = false;
		for(const BattleHex & attackerHex : occupied)
		{
			for(const BattleHex & enemyHex : enemy->getHexes())
			{
				if(attackerHex.getNeighbouringTiles().contains(enemyHex))
				{
					blockedByLanding = true;
					break;
				}
			}
			if(blockedByLanding)
				break;
		}
		if(!blockedByLanding)
			continue;
		result += combatValue.stackValue(enemy, parameters, true)
			- combatValue.stackValue(enemy, parameters, false);
	}
	return result;
}

int64_t ClassicAttackEvaluator::meleeCollateralValue(
	const CStack * attacker,
	const CStack * primaryTarget,
	const BattleHex & landing,
	const ClassicCombatParameters & parameters) const
{
	const auto attacked = battle->getAttackedCreatures(
		attacker, primaryTarget->getPosition(), false, landing).first;
	// VCMI returns secondary victims only; the directly attacked stack is omitted.
	if(attacked.empty())
		return 0;

	const SimulatedAttack primaryExchange = simulateAttack(
		attacker, primaryTarget, landing, false, 0,
		modeledHealth(attacker, parameters), modeledHealth(primaryTarget, parameters));
	if(primaryExchange.attackerAfter <= 0)
		return 0;

	// The original adjacent-cell evaluator scales collateral by the attacker's
	// modeled post-exchange strength. Hydra-style attacks value partial damage;
	// line and breath attacks retain the current kills-only policy.
	const bool multiheaded = attacker->hasBonusOfType(BonusType::ATTACKS_ALL_ADJACENT)
		|| attacker->hasBonusOfType(BonusType::THREE_HEADED_ATTACK);
	int64_t result = 0;
	for(const battle::Unit * attackedUnit : attacked)
	{
		const auto * secondary = dynamic_cast<const CStack *>(attackedUnit);
		if(!secondary || secondary == primaryTarget || secondary == attacker)
			continue;
		const int64_t before = modeledHealth(secondary, parameters);
		if(before <= 0)
			continue;
		const int64_t damage = std::min(before, averageDamage(
			attacker, primaryExchange.attackerAfter, secondary, before, landing, false, 0));
		const int64_t loss = combatValue.lossValue(
			secondary, before, before - damage, parameters, false,
			multiheaded ? false : parameters.killsOnly);
		result += secondary->unitSide() == attacker->unitSide() ? -loss : loss;
	}
	return result;
}

int32_t ClassicAttackEvaluator::dangerAt(
	const CStack * stack,
	const BattleHex & head,
	const DangerMap & dangerMap)
{
	if(!head.isValid())
		return 0;
	int32_t result = dangerMap[head.toInt()];
	if(stack->doubleWide())
	{
		const BattleHex rear = stack->occupiedHex(head);
		if(rear.isValid())
			result = std::min(result, dangerMap[rear.toInt()]);
	}
	return result;
}

ClassicAttackEvaluator::DangerMap ClassicAttackEvaluator::buildDangerMap(
	const CStack * stack,
	const ClassicCombatParameters & parameters) const
{
	DangerMap result{};
	const int64_t activeValue = combatValue.stackValue(stack, parameters, false);
	const int32_t floor = static_cast<int32_t>(-
		std::min<int64_t>(activeValue, std::numeric_limits<int32_t>::max()));
	auto addDanger = [&](const BattleHex & hex, int64_t value)
	{
		if(!hex.isValid() || !hex.isAvailable() || value >= 0)
			return;
		const int64_t combined = static_cast<int64_t>(result[hex.toInt()]) + value;
		result[hex.toInt()] = static_cast<int32_t>(std::max<int64_t>(floor, combined));
	};

	// mark_firewalls runs before stack exposure. Only hostile, damaging Fire
	// Walls contribute; friendly Fire Walls are favorable terrain to this side
	// and therefore do not lower its movement baseline. Moats are different:
	// the executable charges their damage to either physical side, including a
	// defending stack that crosses its own moat.
	for(const auto & obstacle : battle->battleGetAllObstacles(stack->unitSide()))
	{
		const auto * spellObstacle = dynamic_cast<const SpellCreatedObstacle *>(obstacle.get());
		if(!spellObstacle || spellObstacle->minimalDamage <= 0)
			continue;
		const bool hostileFireWall = spellObstacle->getTrigger() == SpellID(SpellID::FIRE_WALL)
			&& spellObstacle->casterSide != stack->unitSide();
		const bool moat = spellObstacle->obstacleType == CObstacleInstance::MOAT;
		if(!hostileFireWall && !moat)
		{
			continue;
		}
		const int64_t health = stack->getAvailableHealth();
		const int64_t loss = combatValue.lossValue(
			stack,
			health,
			std::max<int64_t>(0, health - spellObstacle->minimalDamage),
			parameters,
			false,
			parameters.killsOnly);
		for(const BattleHex & hex : obstacle->getAffectedTiles())
			addDanger(hex, -loss);
	}
	// The projection itself is difficulty-independent. The top-level
	// Easy/Normal gate controls long-move WAIT/fallback behavior, but the
	// resulting danger values still constrain which prefix of a path is used.

	ClassicBattleStateView view(battle);
	for(const CStack * enemy : view.orderedEnemies(stack))
	{
		if(!enemy->alive()
		   || modeledHealth(enemy, parameters) <= 0
		   || !enemy->canMove()
		   || enemy->isTurret()
		   || enemy->isCatapult()
		   || enemy->isFirstAidTent()
		   || enemy->isAmmoCart())
		{
			continue;
		}
		// The executable records an unblocked shooter in a separate bit mask;
		// it does not paint that stack's ranged value onto the melee danger map.
		if(enemy->isShooter() && battle->battleCanShoot(enemy))
			continue;

		const ReachabilityInfo reachability = battle->getReachability(enemy);
		const uint32_t speed = enemy->getMovementRange();
		const uint32_t projectionRange = speed < std::numeric_limits<uint32_t>::max()
			? speed + 1
			: speed;
		bool hasImmediateTarget = false;
		for(const CStack * friendly : view.orderedFriendlies(stack))
		{
			if(!friendly->alive() || friendly->isTurret())
				continue;
			for(const BattleHex & landing : friendly->getAttackableHexes(enemy))
			{
				if(landing.isAvailable()
				   && reachability.isReachable(landing)
				   && reachability.distances[landing.toInt()] <= speed)
				{
					hasImmediateTarget = true;
					break;
				}
			}
			if(hasImmediateTarget)
				break;
		}
		// The original projects each enemy with a speed-limited combat search.
		// If that stack can already attack one of our current stacks, it does
		// not add its ordinary melee value to this map.
		if(hasImmediateTarget)
			continue;

		const int64_t exchange = attackValue(
			enemy, stack, enemy->getPosition(), false, parameters, 0);
		if(exchange <= 0)
			continue;

		// The original search uses speed + 1 as its limit. Each reachable
		// position paints its occupied cells directly, including the second
		// cell of a double-wide stack. Do not expand to neighbouring attack
		// cells or count overlapping positions twice for the same enemy.
		std::array<bool, GameConstants::BFIELD_SIZE> painted{};
		for(int32_t index = 0; index < GameConstants::BFIELD_SIZE; ++index)
		{
			const BattleHex landing(index);
			if(!reachability.isReachable(landing)
			   || reachability.distances[index] > projectionRange)
			{
				continue;
			}
			for(const BattleHex & occupied : battle::Unit::getHexes(
				landing, enemy->doubleWide(), enemy->unitSide()))
			{
				if(!painted[occupied.toInt()])
				{
					painted[occupied.toInt()] = true;
					addDanger(occupied, -exchange);
				}
			}
		}
	}
	return result;
}

ClassicScoredAction ClassicAttackEvaluator::chooseShooterAction(
	const CStack * stack,
	const ClassicCombatParameters & parameters) const
{
	ClassicScoredAction best;
	ClassicBattleStateView view(battle);
	const BattleSide enemyPhysicalSide = stack->unitSide() == BattleSide::ATTACKER
		? BattleSide::DEFENDER
		: BattleSide::ATTACKER;
	const ClassicProjectedTargets projection = findProjectedTargets(enemyPhysicalSide, parameters);
	const CStack * bestTarget = nullptr;
	BattleHex bestCenter = BattleHex::INVALID;
	const int32_t creatureID = stack->creatureId().getNum();
	const bool areaShooter = creatureID == 45 || creatureID == 64 || creatureID == 65;
	for(const CStack * enemy : view.orderedEnemies(stack))
	{
		if(!enemy->alive() || modeledHealth(enemy, parameters) <= 0
		   || !battle->battleCanShoot(stack, enemy->getPosition()))
			continue;
		BattleHex center = enemy->getPosition();
		int64_t score = 0;
		if(areaShooter)
		{
			score = areaShotValue(stack, center, parameters, projection);
			if(enemy->doubleWide())
			{
				const BattleHex rear = enemy->occupiedHex();
				const int64_t rearValue = areaShotValue(stack, rear, parameters, projection);
				if(rearValue > score)
				{
					score = rearValue;
					center = rear;
				}
			}
			if(score < 0)
				continue;
		}
		else
			score = rangedTargetValue(stack, enemy, parameters, projection);
		if(trace)
			trace->record("shoot", std::to_string(enemy->unitId()), score);
		const bool candidateDisabled = spellDuration(enemy, SpellID(SpellID::BLIND)) > 0
			|| spellDuration(enemy, SpellID(SpellID::STONE_GAZE)) > 0
			|| spellDuration(enemy, SpellID(SpellID::PARALYZE)) > 0;
		bool select = !best.valid;
		if(best.valid)
		{
			const bool bestDisabled = bestTarget
				&& (spellDuration(bestTarget, SpellID(SpellID::BLIND)) > 0
					|| spellDuration(bestTarget, SpellID(SpellID::STONE_GAZE)) > 0
					|| spellDuration(bestTarget, SpellID(SpellID::PARALYZE)) > 0);
			select = shouldReplaceShooterTarget(
				score, candidateDisabled, best.score, bestDisabled);
		}
		if(select)
		{
			best.valid = true;
			best.score = score;
			best.attackTime = 0;
			best.action = BattleAction::makeShotAttack(stack, enemy);
			best.action.target.front().hexValue = center;
			bestTarget = enemy;
			bestCenter = center;
		}
	}
	if(trace && bestTarget)
		trace->record("shoot.center", std::to_string(bestTarget->unitId()), bestCenter.toInt());
	return best;
}

ClassicScoredAction ClassicAttackEvaluator::chooseBerserkAction(const CStack * stack) const
{
	ClassicScoredAction result;
	const ForcedAction forced = battle->getBerserkForcedAction(stack);
	if(forced.type == EActionType::SHOOT && forced.target)
		result.action = BattleAction::makeShotAttack(stack, forced.target);
	else if(forced.type == EActionType::WALK_AND_ATTACK && forced.target)
		result.action = BattleAction::makeMeleeAttack(stack, forced.target, forced.position);
	else if(forced.type == EActionType::WALK && forced.position.isValid())
		result.action = BattleAction::makeMove(stack, forced.position);
	else
		result.action = BattleAction::makeDefend(stack);
	result.valid = true;
	result.score = 0;
	result.attackTime = 0;
	return result;
}

ClassicScoredAction ClassicAttackEvaluator::chooseMeleeAction(
	const CStack * stack,
	const ClassicCombatParameters & parameters) const
{
	ClassicScoredAction best;
	const CStack * bestEnemy = nullptr;
	uint32_t bestPathDistance = ReachabilityInfo::INFINITE_DIST;
	ClassicBattleStateView view(battle);
	const ClassicProjectedTargets projection = findProjectedTargets(
		stack->unitSide(), parameters, stack, true);
	const ReachabilityInfo reachability = battle->getReachability(stack);
	const uint32_t speed = stack->getMovementRange();
	const DangerMap dangerMap = buildDangerMap(stack, parameters);
	BattleHex bestLanding = BattleHex::INVALID;

	for(const CStack * enemy : view.orderedEnemies(stack))
	{
		if(modeledHealth(enemy, parameters) <= 0)
			continue;
		BattleHex enemyBestHex = BattleHex::INVALID;
		int64_t enemyBestContext = std::numeric_limits<int64_t>::min();
		int32_t enemyBestTime = std::numeric_limits<int32_t>::max();
		uint32_t enemyBestDistance = ReachabilityInfo::INFINITE_DIST;
		const std::vector<BattleHex> attackable = originalAttackHexOrder(stack, enemy);
		auto traceBest = [&](const BattleHex & candidate)
		{
			if(!trace)
				return;
			trace->record("attack_hex.best_hex", std::to_string(candidate.toInt()), enemyBestHex.toInt());
			trace->record("attack_hex.best_value", std::to_string(candidate.toInt()),
				enemyBestHex.isValid() ? enemyBestContext : 0);
			trace->record("attack_hex.best_time", std::to_string(candidate.toInt()),
				enemyBestHex.isValid() ? enemyBestTime : 0);
		};

		for(const BattleHex & landing : attackable)
		{
			if(trace)
				trace->record("attack_hex.candidate", std::to_string(landing.toInt()), landing.toInt());
			if(!landing.isAvailable() || !reachability.isReachable(landing))
			{
				traceBest(landing);
				continue;
			}
			if(pathCrossesClosedGate(stack, landing, reachability))
			{
				// SoD's find_attack_hex rejects a defender route that exits an
				// intact castle through its closed gate. VCMI deliberately lets
				// the defending side path through its own gate, so this must be
				// filtered before evaluation and, critically, before melee RNG.
				if(trace)
					trace->record("attack_hex.closed_gate", std::to_string(landing.toInt()), 1);
				traceBest(landing);
				continue;
			}
			// Attack time is one even when the current position is already a
			// legal landing. The executable compares actions, not movement-only
			// turns, so an adjacent strike never receives a special zero band.
			const int32_t attackTime = std::max(
				1,
				turnsToReach(reachability.distances[landing.toInt()], speed));
			const int64_t adjacent = landingContextValue(stack, landing, parameters);
			const int64_t adjacentFinal = adjacent
				+ meleeCollateralValue(stack, enemy, landing, parameters);
			const int64_t danger = dangerAt(stack, landing, dangerMap);
			const int64_t context = adjacentFinal + danger;
			if(trace)
			{
				trace->record("attack_hex.time", std::to_string(landing.toInt()), attackTime);
				trace->record("attack_hex.adjacent", std::to_string(landing.toInt()), adjacent);
				trace->record("attack_hex.adjacent_final", std::to_string(landing.toInt()), adjacentFinal);
				trace->record("attack_hex.danger", std::to_string(landing.toInt()), danger);
				trace->record("attack_hex.total", std::to_string(landing.toInt()), context);
			}
			const uint32_t pathDistance = reachability.distances[landing.toInt()];
			if(attackTime < enemyBestTime
				|| (attackTime == enemyBestTime && context > enemyBestContext)
				|| (attackTime == enemyBestTime && context == enemyBestContext && pathDistance < enemyBestDistance))
			{
				enemyBestTime = attackTime;
				enemyBestContext = context;
				enemyBestDistance = pathDistance;
				enemyBestHex = landing;
				if(trace)
					trace->record("attack_hex.select", std::to_string(landing.toInt()), context);
			}
			traceBest(landing);
		}
		if(!enemyBestHex.isValid())
			continue;

		const bool attackerIncapacitated = spellDuration(stack, SpellID(SpellID::BLIND)) > 0
			|| spellDuration(stack, SpellID(SpellID::STONE_GAZE)) > 0
			|| spellDuration(stack, SpellID(SpellID::PARALYZE)) > 0;
		// choose_melee_target calls get_attack_change only for an attack that
		// can occur this activation, and only for an ordinary active attacker
		// against a target without the Hydra-style all-adjacent retaliation.
		// These target-selection guards do not apply to other callers of
		// the projected attack-change calculation.
		const bool mayChangeProjectedAttack = enemyBestDistance <= speed
			&& !attackerIncapacitated
			&& !stack->hasBonusOfType(BonusType::SIEGE_WEAPON)
			&& !stack->isFirstAidTent()
			&& !stack->isAmmoCart()
			&& !enemy->hasBonusOfType(BonusType::ATTACKS_ALL_ADJACENT);
		const int64_t targetAttackChange = mayChangeProjectedAttack
			? attackChange(stack, enemy, parameters, projection)
			: 0;
		// An immediate charge follows the pathfinder's route, including detours
		// around obstacles. Its length can exceed the straight hex distance.
		const int32_t chargeDistance = enemyBestDistance <= speed
			? static_cast<int32_t>(enemyBestDistance)
			: -1;
		const int64_t simpleAttackValue = attackValue(
			stack, enemy, enemyBestHex, false, parameters, chargeDistance);
		const int64_t rawScore = targetAttackChange + simpleAttackValue + enemyBestContext;
		const int32_t randomPercent = randomGenerator->nextIntInclusive(75, 100);
		const int64_t score = rawScore * randomPercent / 100;
		if(trace)
		{
			const std::string targetKey = std::to_string(enemy->getPosition().toInt());
			trace->record("melee.attack_change", targetKey, targetAttackChange);
			trace->record("melee.attack_value", targetKey, simpleAttackValue);
			trace->record("melee.after_attack", targetKey, targetAttackChange + simpleAttackValue);
			trace->record("melee.raw", targetKey, rawScore);
			trace->record("melee.random", targetKey, randomPercent);
			trace->record("melee.randomized", targetKey, score);
			trace->record("melee.time", std::to_string(enemy->unitId()), enemyBestTime);
		}

		const bool candidateDisabled = spellDuration(enemy, SpellID(SpellID::BLIND)) > 0
			|| spellDuration(enemy, SpellID(SpellID::STONE_GAZE)) > 0
			|| spellDuration(enemy, SpellID(SpellID::PARALYZE)) > 0;
		const bool bestDisabled = bestEnemy
			&& (spellDuration(bestEnemy, SpellID(SpellID::BLIND)) > 0
				|| spellDuration(bestEnemy, SpellID(SpellID::STONE_GAZE)) > 0
				|| spellDuration(bestEnemy, SpellID(SpellID::PARALYZE)) > 0);
		const int64_t bestNormalized = best.attackTime > 0
			&& best.attackTime != std::numeric_limits<int32_t>::max()
			? best.score / best.attackTime
			: best.score;
		bool select = !best.valid;
		if(best.valid && candidateDisabled != bestDisabled)
		{
			// Blind, Stone Gaze, and Paralyze form one target class.
			// An enabled target always replaces a disabled one, while a disabled
			// target can never replace an enabled one, regardless of distance or score.
			select = bestDisabled;
		}
		else if(best.valid && enemyBestTime < best.attackTime)
		{
			select = true;
		}
		else if(best.valid && enemyBestTime == best.attackTime)
		{
			// The executable has an asymmetric comparison here: the retained
			// candidate is divided by its attack time, while the new candidate
			// is still its raw randomized score.  For targets several turns away
			// this strongly favors later equal-time candidates.  Preserve the
			// original arithmetic, including its observable asymmetry.
			if(score > bestNormalized)
				select = true;
			else if(score == bestNormalized)
			{
				const uint32_t candidateSpeed = enemy->getMovementRange();
				const uint32_t retainedSpeed = bestEnemy ? bestEnemy->getMovementRange() : 0;
				if(candidateSpeed > retainedSpeed)
					select = true;
				else if(candidateSpeed == retainedSpeed && enemyBestDistance >= bestPathDistance)
					select = true;
			}
		}
		if(select)
		{
			best.valid = true;
			best.score = score;
			best.attackTime = enemyBestTime;
			bestEnemy = enemy;
			bestPathDistance = enemyBestDistance;
			bestLanding = enemyBestHex;
		}
	}
	if(best.valid && bestEnemy && bestLanding.isValid())
	{
		if(best.attackTime <= 1 && battle->isMeleeAttackPossible(stack, bestEnemy, bestLanding))
			best.action = BattleAction::makeMeleeAttack(stack, bestEnemy, bestLanding);
		else
			best.action = moveToward(
				stack,
				bestLanding,
				best.attackTime > 1 && !parameters.simulated && !secondPhase,
				&dangerMap);
	}
	else
	{
		// With an intact siege line find_attack_hex legitimately rejects every
		// defender-adjacent cell.  The original does not stop there: attackers
		// advance to the first ordinary cell immediately outside the wall/moat
		// contour on their current battlefield row.  This fallback precedes the
		// shooter-defense/run-away paths and consumes no target-selection RNG.
		const BattleHex siegeTarget = siegeAdvanceTarget(stack);
		if(siegeTarget.isValid())
		{
			best.valid = true;
			best.score = 0;
			best.attackTime = 0;
			best.action = moveToward(stack, siegeTarget, false, &dangerMap);
			if(trace)
				trace->record("siege.advance", std::to_string(stack->getPosition().toInt()), siegeTarget.toInt());
		}
	}
	return best;
}

bool ClassicAttackEvaluator::pathCrossesClosedGate(
	const CStack * stack,
	const BattleHex & destination,
	const ReachabilityInfo & reachability) const
{
	if(stack->unitSide() != BattleSide::DEFENDER
	   || stack->hasBonusOfType(BonusType::FLYING)
	   || battle->battleIsGatePassable()
	   || !battle->battleIsInsideWalls(stack->getPosition())
	   || battle->battleIsInsideWalls(destination))
	{
		return false;
	}

	BattleHex cursor = destination;
	while(cursor.isValid() && cursor != stack->getPosition())
	{
		if(cursor == BattleHex::GATE_OUTER || cursor == BattleHex::GATE_INNER)
			return true;
		cursor = reachability.predecessors[cursor.toInt()];
	}
	return false;
}

BattleHex ClassicAttackEvaluator::siegeAdvanceTarget(const CStack * stack) const
{
	const CGTownInstance * town = battle->battleGetDefendedTown();
	if(!town
	   || battle->battleGetFortifications().wallsHealth <= 0
	   || stack->unitSide() != BattleSide::ATTACKER
	   || !stack->getPosition().isAvailable())
	{
		return BattleHex::INVALID;
	}

	// The original wall-contour cell for each battlefield row.
	// Scan left from that contour until a normally occupiable, non-moat cell
	// is found, then use it as the movement destination.
	static constexpr std::array<int16_t, GameConstants::BFIELD_HEIGHT> wallContour = {
		12, 29, 45, 62, 78, 96, 112, 130, 147, 165, 182
	};
	static constexpr std::array<int16_t, GameConstants::BFIELD_HEIGHT> ordinaryMoat = {
		11, 28, 44, 61, 77, 95, 111, 129, 146, 164, 181
	};
	// Fortress uses a second, wider moat contour in addition to the ordinary one.
	static constexpr std::array<int16_t, GameConstants::BFIELD_HEIGHT> fortressMoat = {
		10, 27, 43, 60, 76, 94, 110, 128, 145, 163, 180
	};

	const BattleHex start = stack->getPosition();
	const int32_t row = start.getY();
	const AccessibilityInfo accessibility = battle->getAccessibility(stack);
	const bool closedGate = battle->battleGetGateState() == EGateState::CLOSED
		|| battle->battleGetGateState() == EGateState::BLOCKED;
	const bool fortress = town->getFactionID() == FactionID::FORTRESS;
	auto isMoatCell = [&](int32_t index)
	{
		if(!battle->hasMoat())
			return false;
		const bool ordinary = ordinaryMoat[row] == index
			&& (index != BattleHex::GATE_OUTER || closedGate);
		const bool wide = fortress
			&& fortressMoat[row] == index
			&& (index != BattleHex::GATE_BRIDGE || closedGate);
		return ordinary || wide;
	};

	int32_t target = wallContour[row];
	while(target > start.toInt())
	{
		const BattleHex candidate(target);
		if(accessibility.accessible(candidate, stack) && !isMoatCell(target))
			return candidate;
		--target;
	}
	return BattleHex::INVALID;
}

ClassicScoredAction ClassicAttackEvaluator::chooseShooterDefense(
	const CStack * stack,
	const ClassicCombatParameters & parameters) const
{
	ClassicScoredAction result;
	const ReachabilityInfo reachability = battle->getReachability(stack);
	ClassicBattleStateView view(battle);
	BattleHex bestHex = BattleHex::INVALID;
	uint32_t bestDistance = ReachabilityInfo::INFINITE_DIST;
	int64_t bestProtectedValue = std::numeric_limits<int64_t>::min();

	for(const CStack * friendly : view.orderedFriendlies(stack))
	{
		if(friendly == stack || !friendly->alive() || !friendly->isShooter()
			|| friendly->isAmmoCart() || !battle->battleCanShoot(friendly))
		{
			continue;
		}

		BattleHex candidateHex = BattleHex::INVALID;
		uint32_t candidateDistance = ReachabilityInfo::INFINITE_DIST;
		int32_t openCells = 0;
		const BattleHex friendlyPosition = friendly->getPosition();
		for(const BattleHex & candidate : friendlyPosition.getNeighbouringTiles())
		{
			const CStack * occupant = battle->battleGetStackByPos(candidate, true);
			if(occupant && occupant != stack)
				continue;
			if(!reachability.isReachable(candidate))
				continue;
			++openCells;
			const uint32_t distance = reachability.distances[candidate.toInt()];
			bool select = !candidateHex.isValid();
			if(candidateHex.isValid())
			{
				// The original path-cell ordering is side-oriented before its stored
				// path cost: defenders prefer the western/forward neighbor and
				// attackers the mirrored eastern neighbor.
				if(candidate.getX() != candidateHex.getX())
					select = stack->unitSide() == BattleSide::DEFENDER
						? candidate.getX() < candidateHex.getX()
						: candidate.getX() > candidateHex.getX();
				else if(distance != candidateDistance)
					select = distance < candidateDistance;
				else
					select = candidate.toInt() < candidateHex.toInt();
			}
			if(select)
			{
				candidateHex = candidate;
				candidateDistance = distance;
			}
		}
		if(!candidateHex.isValid())
			continue;

		const int64_t protectedValue = combatValue.stackValue(friendly, parameters)
			/ std::max(1, openCells);
		if(candidateDistance < bestDistance
			|| (candidateDistance == bestDistance && protectedValue > bestProtectedValue))
		{
			bestHex = candidateHex;
			bestDistance = candidateDistance;
			bestProtectedValue = protectedValue;
		}
	}

	if(!bestHex.isValid())
		return result;
	result.valid = true;
	result.score = bestProtectedValue;
	result.attackTime = static_cast<int32_t>(bestDistance);
	result.action = moveToward(stack, bestHex, false);
	if(trace)
	{
		trace->record("shooter_defense", "target", bestHex.toInt());
		trace->record("shooter_defense", "distance", bestDistance);
		trace->record("shooter_defense", "value", bestProtectedValue);
	}
	return result;
}

ClassicScoredAction ClassicAttackEvaluator::chooseRunAction(const CStack * stack, const DangerMap & dangerMap) const
{
	ClassicScoredAction result;
	if(!preserveLongMoveWait)
		return result;

	const int32_t currentDanger = dangerAt(stack, stack->getPosition(), dangerMap);
	if(trace)
		trace->record("run.current", std::to_string(stack->getPosition().toInt()), currentDanger);
	if(currentDanger >= 0 || spellDuration(stack, SpellID(SpellID::BLIND)) > 0 || spellDuration(stack, SpellID(SpellID::STONE_GAZE)) > 0
	   || stack->hasBonusOfType(BonusType::BIND_EFFECT) || spellDuration(stack, SpellID(SpellID::PARALYZE)) > 0)
	{
		return result;
	}

	const ReachabilityInfo reachability = battle->getReachability(stack);
	const BattleHexArray available = battle->battleGetAvailableHexes(reachability, stack, false);
	const uint32_t speed = stack->getMovementRange();
	BattleHex bestHex = BattleHex::INVALID;
	int32_t bestDanger = currentDanger;
	uint32_t bestDistance = 0;
	for(int32_t index = 0; index < GameConstants::BFIELD_SIZE; ++index)
	{
		const BattleHex candidate(index);
		if(!available.contains(candidate))
			continue;
		const uint32_t distance = reachability.distances[index];
		if(distance > speed)
			continue;
		const int32_t candidateDanger = dangerAt(stack, candidate, dangerMap);
		if(trace)
		{
			trace->record("run.candidate_danger", std::to_string(index), candidateDanger);
			trace->record("run.candidate_distance", std::to_string(index), distance);
		}
		if(candidateDanger < bestDanger || (candidateDanger == bestDanger && bestDistance < distance))
		{
			continue;
		}
		bestHex = candidate;
		bestDanger = candidateDanger;
		bestDistance = distance;
		if(trace)
			trace->record("run.select", std::to_string(index), candidateDanger);
	}
	if(trace)
	{
		trace->record("run.final_hex", std::to_string(stack->getPosition().toInt()), bestHex.toInt());
		trace->record("run.final_danger", std::to_string(bestHex.toInt()), bestDanger);
		trace->record("run.final_distance", std::to_string(bestHex.toInt()), bestDistance);
	}
	if(!bestHex.isValid() || bestHex == stack->getPosition())
		return result;

	result.valid = true;
	result.score = bestDanger;
	result.attackTime = static_cast<int32_t>(bestDistance);
	result.action = moveToward(stack, bestHex, false, &dangerMap);
	return result;
}

ClassicScoredAction ClassicAttackEvaluator::chooseRunAction(const CStack * stack, const ClassicCombatParameters & parameters) const
{
	return chooseRunAction(stack, buildDangerMap(stack, parameters));
}

BattleAction ClassicAttackEvaluator::moveToward(
	const CStack * stack,
	const BattleHex & target,
	bool mayWait,
	const DangerMap * dangerMap) const
{
	if(trace)
	{
		trace->record("move_toward.call", std::to_string(stack->getPosition().toInt()), target.toInt());
		trace->record(
			"move_toward.speed",
			std::to_string(stack->getPosition().toInt()),
			stack->getMovementRange());
	}
	if(!stack->canMove() || stack->getMovementRange() == 0)
		return mayWait && !stack->waited()
			? BattleAction::makeWait(stack)
			: BattleAction::makeDefend(stack);

	const ReachabilityInfo reachability = battle->getReachability(stack);
	const BattleHexArray available = battle->battleGetAvailableHexes(reachability, stack, false);
	BattleHex destination = target;
	BattleHex lastScanned = stack->getPosition();
	const bool effectiveMayWait = mayWait
		&& preserveLongMoveWait
		&& dangerMap
		&& !stack->waited();
	const int32_t initialDanger = dangerMap
		? dangerAt(stack, stack->getPosition(), *dangerMap)
		: 0;
	int32_t acceptedDanger = initialDanger;
	if(trace)
	{
		const BattleHex initialRear = stack->doubleWide()
			? stack->occupiedHex(stack->getPosition())
			: stack->getPosition();
		trace->record(
			"move_toward.initial_head",
			std::to_string(stack->getPosition().toInt()),
			dangerMap ? (*dangerMap)[stack->getPosition().toInt()] : 0);
		trace->record(
			"move_toward.initial_rear",
			std::to_string(stack->getPosition().toInt()),
			dangerMap && initialRear.isValid() ? (*dangerMap)[initialRear.toInt()] : 0);
	}
	if(target.isValid() && reachability.isReachable(target))
	{
		// The original combat pathfinder retains a direction-ordered shortest
		// path. VCMI's BFS retains the first predecessor discovered instead,
		// which can select a different (but equally short) route. Recover the
		// complete shortest-path DAG from VCMI's distance labels, then walk it
		// with the original horizontal-first tie policy.
		std::array<bool, GameConstants::BFIELD_SIZE> reachesTarget{};
		reachesTarget[target.toInt()] = true;
		BattleHexArray stoppingTiles;
		for(const auto & obstacle : battle->battleGetAllObstacles(reachability.params.perspective))
			stoppingTiles.insert(obstacle->getStoppingTile());
		auto stopsPathExpansion = [&](const BattleHex & hex)
		{
			if(stack->hasBonusOfType(BonusType::FLYING))
				return false;
			for(const BattleHex & occupied : battle::Unit::getHexes(
				hex, reachability.params.doubleWide, reachability.params.side))
			{
				if(stack->getHexes().contains(occupied))
					continue;
				if(stoppingTiles.contains(occupied))
					return true;
			}
			return false;
		};
		const uint32_t targetDistance = reachability.distances[target.toInt()];
		for(uint32_t level = targetDistance; level > 0; --level)
		{
			for(int32_t index = 0; index < GameConstants::BFIELD_SIZE; ++index)
			{
				if(reachability.distances[index] != level - 1)
					continue;
				const BattleHex candidate(index);
				if(stopsPathExpansion(candidate))
					continue;
				for(const BattleHex & neighbour : candidate.getNeighbouringTiles())
				{
					if(reachability.distances[neighbour.toInt()] == level
						&& reachesTarget[neighbour.toInt()])
					{
						reachesTarget[index] = true;
						break;
					}
				}
			}
		}

		BattleHex current = stack->getPosition();
		BattleHex farthestAvailable = current;
		uint32_t scanned = 0;
		const bool flying = stack->hasBonusOfType(BonusType::FLYING);
		const uint32_t routeLength = flying
			? BattleHex::getDistance(stack->getPosition(), target)
			: targetDistance;
		const uint32_t speed = stack->getMovementRange();
		const uint32_t wholeSpeedBoundary = routeLength == 0 || speed == 0
			? 0
			: ((routeLength - 1) / speed) * speed;
		bool acceptedAny = false;
		while(current != target && scanned < stack->getMovementRange())
		{
			BattleHexArray continuations;
			const uint32_t nextDistance = reachability.distances[current.toInt()] + 1;
			const int32_t remainingDistance = BattleHex::getDistance(current, target);
			for(const BattleHex & neighbour : current.getNeighbouringTiles())
			{
				if((flying
						&& BattleHex::getDistance(neighbour, target) == remainingDistance - 1)
				   || (!flying
						&& reachability.distances[neighbour.toInt()] == nextDistance
						&& reachesTarget[neighbour.toInt()]))
				{
					continuations.insert(neighbour);
				}
			}
			if(continuations.empty())
				break;
			const int32_t currentAxialX = current.getX() + current.getY() / 2;
			const int32_t targetAxialX = target.getX() + target.getY() / 2;
			const int32_t axialX = targetAxialX - currentAxialX;
			const int32_t axialY = target.getY() - current.getY();
			const bool verticalFirst = axialX * axialY < 0;
			current = *std::ranges::min_element(
				continuations,
				[&](const BattleHex & lhs, const BattleHex & rhs)
				{
					const int32_t lhsDistance = BattleHex::getDistance(lhs, target);
					const int32_t rhsDistance = BattleHex::getDistance(rhs, target);
					if(lhsDistance != rhsDistance)
						return lhsDistance < rhsDistance;
					if(!verticalFirst)
					{
						auto axialImbalance = [&](const BattleHex & candidate)
						{
							const int32_t candidateX = candidate.getX() + candidate.getY() / 2;
							return std::abs(
								std::abs(targetAxialX - candidateX)
								- std::abs(target.getY() - candidate.getY()));
						};
						const int32_t lhsImbalance = axialImbalance(lhs);
						const int32_t rhsImbalance = axialImbalance(rhs);
						if(lhsImbalance != rhsImbalance)
							return lhsImbalance < rhsImbalance;
					}
					const int32_t lhsVertical = std::abs(lhs.getY() - target.getY());
					const int32_t rhsVertical = std::abs(rhs.getY() - target.getY());
					const int32_t lhsHorizontal = std::abs(lhs.getX() - target.getX());
					const int32_t rhsHorizontal = std::abs(rhs.getX() - target.getX());
					if(verticalFirst && lhsVertical != rhsVertical)
						return lhsVertical < rhsVertical;
					if(lhsHorizontal != rhsHorizontal)
						return lhsHorizontal < rhsHorizontal;
					return lhsVertical < rhsVertical;
				});
			++scanned;
			lastScanned = current;
			if(trace)
				trace->record(
					"move_toward.path",
					std::to_string(BattleHex::getDistance(current, target)),
					current.toInt());
			if(!available.contains(current))
				continue;
			const uint32_t remainingPathNodes = routeLength - scanned + 1;
			const bool checkDanger = dangerMap
				&& (effectiveMayWait
					|| (acceptedAny && remainingPathNodes <= wholeSpeedBoundary));
			if(checkDanger)
			{
				const BattleHexArray occupied = battle::Unit::getHexes(
					current, stack->doubleWide(), stack->unitSide());
				const BattleHex candidateRear = stack->doubleWide()
					? stack->occupiedHex(current)
					: current;
				if(trace)
				{
					trace->record(
						"move_toward.candidate_head",
						std::to_string(current.toInt()),
						(*dangerMap)[current.toInt()]);
					trace->record(
						"move_toward.candidate_rear",
						std::to_string(current.toInt()),
						candidateRear.isValid() ? (*dangerMap)[candidateRear.toInt()] : 0);
					trace->record(
						"move_toward.accepted_baseline",
						std::to_string(current.toInt()),
						acceptedDanger);
				}
				bool worsensBaseline = false;
				for(const BattleHex & occupiedHex : occupied)
				{
					if((*dangerMap)[occupiedHex.toInt()] < acceptedDanger)
					{
						worsensBaseline = true;
						break;
					}
				}
				if(worsensBaseline)
					continue;
			}
			farthestAvailable = current;
			acceptedAny = true;
			if(dangerMap)
				acceptedDanger = dangerAt(stack, current, *dangerMap);
			if(trace)
				trace->record("move_toward.accept", std::to_string(current.toInt()), current.toInt());
		}
		destination = farthestAvailable;
	}
	else
	{
		while(destination.isValid() && !available.contains(destination))
			destination = reachability.predecessors[destination.toInt()];
	}
	if(trace)
	{
		trace->record("move_toward.final_selected", std::to_string(destination.toInt()), destination.toInt());
		trace->record("move_toward.final_scanned", std::to_string(lastScanned.toInt()), lastScanned.toInt());
		trace->record("move_toward.final_initial", std::to_string(destination.toInt()), initialDanger);
		trace->record("move_toward.final_baseline", std::to_string(destination.toInt()), acceptedDanger);
		trace->record("move_toward.final_may_wait", std::to_string(destination.toInt()), effectiveMayWait ? 1 : 0);
	}
	if(!destination.isValid() || destination == stack->getPosition())
		return effectiveMayWait
			? BattleAction::makeWait(stack)
			: BattleAction::makeDefend(stack);
	if(effectiveMayWait
	   && destination != lastScanned
	   && acceptedDanger >= initialDanger)
	{
		if(trace)
			trace->record("move_toward.result", "8", destination.toInt());
		return BattleAction::makeWait(stack);
	}
	if(trace)
		trace->record("move_toward.result", "2", destination.toInt());
	return BattleAction::makeMove(stack, destination);
}

BattleAction ClassicAttackEvaluator::chooseTacticsShooterPlacement(
	const CStack * stack,
	bool enabled) const
{
	if(!enabled || !stack->isShooter())
		return BattleAction::makeWait(stack);

	const ReachabilityInfo reachability = battle->getReachability(stack);
	const BattleHexArray available = battle->battleGetAvailableHexes(reachability, stack, false);
	BattleHex bestHex = stack->getPosition();
	int32_t bestScore = std::numeric_limits<int32_t>::max();
	for(int32_t index = 0; index < GameConstants::BFIELD_SIZE; ++index)
	{
		const BattleHex candidate(index);
		if(!available.contains(candidate))
			continue;

		int32_t score = 0;
		BattleHexArray adjacent;
		for(const BattleHex & occupied : battle::Unit::getHexes(
			candidate, stack->doubleWide(), stack->unitSide()))
		{
			for(const BattleHex & neighbour : occupied.getNeighbouringTiles())
			{
				if(!battle::Unit::getHexes(candidate, stack->doubleWide(), stack->unitSide()).contains(neighbour))
					adjacent.insert(neighbour);
			}
		}
		for(const BattleHex & neighbour : adjacent)
		{
			const CStack * occupant = battle->battleGetStackByPos(neighbour, true);
			if(!occupant || occupant == stack)
				++score;
			else if(battle->battleMatchOwner(stack, occupant, false) && occupant->isShooter())
				score = 1000;
		}

		if(score < bestScore || (score == bestScore && candidate == stack->getPosition()))
		{
			bestScore = score;
			bestHex = candidate;
		}
	}
	return bestHex == stack->getPosition()
		? BattleAction::makeWait(stack)
		: BattleAction::makeMove(stack, bestHex);
}

ClassicScoredAction ClassicAttackEvaluator::chooseAction(
	const CStack * stack,
	const ClassicCombatParameters & parameters) const
{
	if(stack->hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE))
		return chooseBerserkAction(stack);
	if(battle->battleCanShoot(stack))
	{
		auto result = chooseShooterAction(stack, parameters);
		if(stack->hasBonusOfType(BonusType::CATAPULT)
			&& stack->unitSide() == BattleSide::ATTACKER
			&& battle->battleGetDefendedTown())
		{
			const int64_t stranded = strandedFriendlyValue(stack, parameters);
			const bool preferWall = !result.valid
								 || static_cast<long double>(stranded) * parameters.enemyCombatValue
										> 3.0L * std::max<int64_t>(0, result.score) * parameters.friendlyCombatValue;
			if(trace)
			{
				trace->record("cyclops", "stranded", stranded);
				trace->record("cyclops", "preferWall", preferWall);
			}
			if(preferWall)
			{
				result.valid = true;
				result.action = chooseCyclopsAction(stack);
				return result;
			}
		}
		if(result.valid)
			return result;
		ClassicScoredAction defend;
		defend.valid = true;
		defend.score = 0;
		defend.attackTime = 0;
		defend.action = BattleAction::makeDefend(stack);
		return defend;
	}

	auto result = chooseMeleeAction(stack, parameters);
	if(result.valid && result.score < 0 && stack->isShooter())
	{
		const ClassicScoredAction defense = chooseShooterDefense(stack, parameters);
		if(defense.valid)
			return defense;
	}
	if(result.valid)
		return result;
	result = chooseRunAction(stack, parameters);
	if(result.valid)
		return result;
	result.valid = true;
	result.score = 0;
	result.attackTime = 0;
	result.action = stack->waited()
		|| ClassicRulesAdapter::failedSiege(battle, stack->unitSide())
		|| !preserveLongMoveWait
		? BattleAction::makeDefend(stack)
		: BattleAction::makeWait(stack);
	return result;
}

ClassicExpectedDamage ClassicAttackEvaluator::projectExpectedDamage(
	BattleSide side,
	bool includeOtherSide,
	const ClassicCombatParameters & parameters) const
{
	ClassicBattleStateView view(battle);
	const auto order = view.moveOrder(side, secondPhase);
	ClassicExpectedDamage expected;
	for(const ClassicMoveOrderEntry & entry : order)
		expected[entry.stack->unitId()] = 0;

	auto remainingHealth = [&](const CStack * stack)
	{
		const int64_t initial = stack->isClone() ? 1 : stack->getAvailableHealth();
		return std::max<int64_t>(0, initial - expected[stack->unitId()]);
	};
	auto addDamage = [&](const CStack * stack, int64_t damage)
	{
		if(!stack || damage <= 0)
			return;
		const int64_t initial = stack->isClone() ? 1 : stack->getAvailableHealth();
		expected[stack->unitId()] = std::min<int64_t>(
			initial, expected[stack->unitId()] + damage);
	};

	size_t cursor = 0;
	auto simulateSide = [&](BattleSide requestedSide)
	{
		while(cursor < order.size())
		{
			const CStack * actor = order[cursor].stack;
			const bool incapacitated = spellDuration(actor, SpellID(SpellID::BLIND)) > 0
				|| spellDuration(actor, SpellID(SpellID::STONE_GAZE)) > 0
				|| spellDuration(actor, SpellID(SpellID::PARALYZE)) > 0;
			if(incapacitated
			   || actor->isFirstAidTent()
			   || actor->isAmmoCart()
			   || actor->hasBonusOfType(BonusType::ATTACKS_NEAREST_CREATURE)
			   || actor->isCatapult()
			   || remainingHealth(actor) <= 0)
			{
				++cursor;
				continue;
			}
			if(view.controllingSide(actor) != requestedSide)
				return;

			ClassicCombatParameters simulatedParameters = parameters;
			simulatedParameters.simulated = true;
			simulatedParameters.expectedDamage = expected;
			const bool ranged = battle->battleCanShoot(actor);
			const ClassicScoredAction choice = chooseAction(actor, simulatedParameters);
			const EActionType requiredType = ranged
				? EActionType::SHOOT
				: EActionType::WALK_AND_ATTACK;
			if(choice.valid && choice.action.actionType == requiredType)
			{
				const size_t targetIndex = ranged ? 0 : 1;
				if(choice.action.target.size() > targetIndex)
				{
					const auto & destination = choice.action.target[targetIndex];
					const CStack * target = destination.unitValue >= 0
						? battle->battleGetStackByID(destination.unitValue, false)
						: battle->battleGetStackByPos(destination.hexValue, true);
					if(target && remainingHealth(target) > 0)
					{
						if(ranged)
						{
							const int64_t damage = averageDamage(
								actor,
								remainingHealth(actor),
								target,
								remainingHealth(target),
								actor->getPosition(),
								true,
								0);
							addDamage(target, damage);
						}
						else
						{
							const BattleHex attackFrom = choice.action.target.front().hexValue;
							const auto attacked = battle->getAttackedCreatures(
								actor, target->getPosition(), false, attackFrom).first;
							const SimulatedAttack exchange = simulateAttack(
								actor,
								target,
								attackFrom,
								false,
								0,
								remainingHealth(actor),
								remainingHealth(target));
							addDamage(actor, exchange.attackerBefore - exchange.attackerAfter);
							addDamage(target, exchange.defenderBefore - exchange.defenderAfter);
							for(const battle::Unit * attackedUnit : attacked)
							{
								const auto * secondary = dynamic_cast<const CStack *>(attackedUnit);
								if(!secondary || secondary == actor || secondary == target
								   || remainingHealth(secondary) <= 0)
								{
									continue;
								}
								const int64_t secondaryBefore = remainingHealth(secondary);
								int64_t secondaryDamage = averageDamage(
									actor, exchange.attackerBefore, secondary, secondaryBefore,
									attackFrom, false, 0);
								if(exchange.secondStrike > 0 && secondaryDamage < secondaryBefore)
								{
									const int64_t attackerBeforeSecond = std::max<int64_t>(
										0, exchange.attackerBefore - exchange.firstFireShield - exchange.retaliation);
									secondaryDamage += averageDamage(
										actor, attackerBeforeSecond, secondary,
										secondaryBefore - secondaryDamage, attackFrom, false, 0);
								}
								addDamage(secondary, secondaryDamage);
								if(trace)
								{
									trace->record("projection.expected_damage",
										std::to_string(secondary->unitId()), expected[secondary->unitId()]);
								}
							}
						}
						if(trace)
						{
							trace->record("projection.expected_damage", std::to_string(actor->unitId()),
								expected[actor->unitId()]);
							trace->record("projection.expected_damage", std::to_string(target->unitId()),
								expected[target->unitId()]);
						}
					}
				}
			}
			++cursor;
		}
	};

	simulateSide(side);
	if(includeOtherSide)
		simulateSide(side == BattleSide::ATTACKER ? BattleSide::DEFENDER : BattleSide::ATTACKER);
	return expected;
}

int64_t ClassicAttackEvaluator::strandedFriendlyValue(
	const CStack * stack,
	const ClassicCombatParameters & parameters) const
{
	int64_t result = 0;
	ClassicBattleStateView view(battle);
	for(const CStack * friendly : view.orderedFriendlies(stack))
	{
		if(!friendly->alive() || friendly->isTurret())
			continue;
		bool hasTarget = false;
		const ReachabilityInfo reachability = battle->getReachability(friendly);
		for(const CStack * enemy : view.orderedEnemies(friendly))
		{
			if(battle->battleCanShoot(friendly, enemy->getPosition()))
			{
				hasTarget = true;
				break;
			}
			for(const BattleHex & landing : enemy->getAttackableHexes(friendly))
			{
				if(landing.isAvailable() && reachability.isReachable(landing))
				{
					hasTarget = true;
					break;
				}
			}
			if(hasTarget)
				break;
		}
		if(!hasTarget)
			result += combatValue.stackValue(friendly, parameters);
	}
	return result;
}

BattleAction ClassicAttackEvaluator::chooseCyclopsAction(const CStack * stack) const
{
	constexpr std::array<EWallPart, 4> WALL_PARTS = {
		EWallPart::BELOW_GATE,
		EWallPart::OVER_GATE,
		EWallPart::BOTTOM_WALL,
		EWallPart::UPPER_WALL
	};
	std::vector<EWallPart> candidates;
	EWallState minimum = EWallState::REINFORCED;
	for(EWallPart part : WALL_PARTS)
	{
		const EWallState state = battle->battleGetWallState(part);
		if(state <= EWallState::DESTROYED || !battle->isWallPartPotentiallyAttackable(part))
			continue;
		if(candidates.empty() || state < minimum)
		{
			minimum = state;
			candidates = {part};
		}
		else if(state == minimum)
			candidates.push_back(part);
	}
	if(candidates.empty())
		return BattleAction::makeDefend(stack);
	// The executable always asks for an inclusive 1..tieCount roll, including
	// the degenerate [1, 1] case. Preserving that draw is observable by every
	// later randomized choice in the battle.
	const size_t selected = static_cast<size_t>(
		randomGenerator->nextIntInclusive(1, static_cast<int32_t>(candidates.size())) - 1);
	BattleAction result;
	result.actionType = EActionType::CATAPULT;
	result.side = stack->unitSide();
	result.stackNumber = stack->unitId();
	result.aimToHex(battle->wallPartToBattleHex(candidates[selected]));
	return result;
}

BattleAction ClassicAttackEvaluator::chooseCatapultAction(const CStack * stack) const
{
	BattleHex target = BattleHex::INVALID;
	if(battle->battleGetGateState() == EGateState::CLOSED)
		target = battle->wallPartToBattleHex(EWallPart::GATE);
	else
	{
		for(EWallPart part : WALL_TARGET_ORDER)
		{
			const EWallState state = battle->battleGetWallState(part);
			if(state != EWallState::NONE && state != EWallState::DESTROYED && battle->isWallPartPotentiallyAttackable(part))
			{
				target = battle->wallPartToBattleHex(part);
				break;
			}
		}
	}
	if(!target.isValid())
		return BattleAction::makeDefend(stack);
	BattleAction result;
	result.actionType = EActionType::CATAPULT;
	result.side = stack->unitSide();
	result.stackNumber = stack->unitId();
	result.aimToHex(target);
	return result;
}

BattleAction ClassicAttackEvaluator::chooseHealingTentAction(const CStack * stack) const
{
	ClassicBattleStateView view(battle);
	const CStack * best = nullptr;
	int64_t bestWound = 0;
	for(const CStack * friendly : view.orderedFriendlies(stack))
	{
		if(!friendly->canBeHealed())
			continue;
		const int64_t wound = friendly->getMaxHealth() - friendly->getFirstHPleft();
		if(wound > bestWound)
		{
			bestWound = wound;
			best = friendly;
		}
	}
	return best ? BattleAction::makeHeal(stack, best) : BattleAction::makeDefend(stack);
}
