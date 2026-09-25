/*
 * ClassicCombatValue.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "../StdInc.h"
#include "ClassicCombatValue.h"

#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/CStack.h"
#include "../../../lib/battle/CBattleInfoCallback.h"
#include "../../../lib/bonuses/BonusCustomTypes.h"
#include "../../../lib/mapObjects/CGHeroInstance.h"
#include "ClassicBattleStateView.h"
#include "ClassicRulesAdapter.h"

namespace
{
bool excludedFromAwakeValue(const CStack * stack)
{
	return !stack->canMove()
		|| stack->hasBonusOfType(BonusType::NOT_ACTIVE)
		|| stack->isFirstAidTent()
		|| stack->isAmmoCart();
}

const CGHeroInstance * ownerHero(
	const std::shared_ptr<CBattleInfoCallback> & battle,
	const battle::Unit * unit)
{
	const battle::Unit * original = battle->battleGetUnitByID(unit->unitId());
	return original ? battle->battleGetOwnerHero(original) : nullptr;
}
}

ClassicCombatValue::ClassicCombatValue(std::shared_ptr<CBattleInfoCallback> battle) : battle(std::move(battle)) {}

int64_t ClassicCombatValue::truncateTowardZero(double value)
{
	return static_cast<int64_t>(value);
}

ClassicCombatParameters ClassicCombatValue::buildParameters(BattleSide side, int32_t difficulty) const
{
	ClassicCombatParameters result;
	result.ourSide = side;
	result.enemySide = battle->otherSide(side);

	bool first = true;
	for(const CStack * stack : battle->battleGetAllStacks(false))
	{
		if(!stack->alive() || stack->isTurret())
			continue;
		// army::get_unit_combat_value receives the lowest *bonus above the
		// creature base*, not the lowest absolute primary stat.  Its formula
		// subtracts both the creature base and this common bonus.  Feeding it
		// an absolute stat double-subtracts the base value and materially
		// distorts exchanges between creatures of different tiers.
		const auto * creature = stack->unitType();
		const int32_t attack = ClassicRulesAdapter::attack(stack, stack->isShooter()) - creature->getBaseAttack();
		const int32_t defense = ClassicRulesAdapter::defense(stack) - creature->getBaseDefense();
		if(first)
		{
			result.lowestAttack = attack;
			result.lowestDefense = defense;
			first = false;
		}
		else
		{
			result.lowestAttack = std::min(result.lowestAttack, attack);
			result.lowestDefense = std::min(result.lowestDefense, defense);
		}
	}

	result.friendlyCombatValue = sideValue(side, result, true);
	result.enemyCombatValue = sideValue(battle->otherSide(side), result, true);
	result.awakeFriendlyValue = sideValue(side, result, false);
	result.awakeEnemyValue = sideValue(battle->otherSide(side), result, false);
	result.killsOnly = 2 * result.friendlyCombatValue < result.enemyCombatValue;
	if(difficulty == 0)
		result.killsOnly = false;

	const int64_t high = std::max(result.awakeFriendlyValue, result.awakeEnemyValue);
	const int64_t low = std::min(result.awakeFriendlyValue, result.awakeEnemyValue);
	if(low == 0 || high >= 5 * low)
		result.roundsLeft = 1;
	else
	{
		const double ratio = static_cast<double>(high) / low;
		if(ratio >= 2.60)
			result.roundsLeft = 1;
		else if(ratio >= 1.90)
			result.roundsLeft = 2;
		else if(ratio >= 1.50)
			result.roundsLeft = 3;
		else if(ratio >= 1.31)
			result.roundsLeft = 4;
		else if(ratio >= 1.20)
			result.roundsLeft = 5;
		else if(ratio >= 1.13)
			result.roundsLeft = 6;
		else
			result.roundsLeft = 7;
	}
	return result;
}

double ClassicCombatValue::unitValueExact(
	const CStack * stack,
	const ClassicCombatParameters & parameters,
	bool ranged) const
{
	const auto * creature = stack->unitType();
	// The executable accepts a requested mode and then validates that request
	// against the stack's current shooting state. Ballista and arrow towers are
	// the two hard-coded exceptions to the ordinary ammunition/blocking checks.
	const int32_t creatureID = creature->getId().getNum();
	const bool rangedAttack = ranged && (creatureID == 146 || creatureID == 149
		|| (stack->isShooter() && battle->battleCanShoot(stack)));
	const int32_t attack = ClassicRulesAdapter::attack(stack, rangedAttack);
	const int32_t defense = ClassicRulesAdapter::defense(stack);
	double defenseModifier = 1.0;
	const BonusSubtypeID shieldType = rangedAttack
		? BonusSubtypeID(BonusCustomSubtype::damageTypeRanged)
		: BonusSubtypeID(BonusCustomSubtype::damageTypeMelee);
	const int32_t shieldReduction = stack->valOfBonuses(BonusType::GENERAL_DAMAGE_REDUCTION, shieldType);
	if(shieldReduction != 0)
		defenseModifier = 1.0 - shieldReduction / 100.0;
	if(stack->isFrozen())
		defenseModifier *= 0.5;

	ClassicBattleStateView view(battle);
	if(const CGHeroInstance * owner = ownerHero(battle, stack))
	{
		const int32_t armorerReduction = owner->valOfBonuses(
			BonusType::GENERAL_DAMAGE_REDUCTION,
			BonusSubtypeID(BonusCustomSubtype::damageTypeAll));
		defenseModifier *= 1.0 - std::min(100, armorerReduction) / 100.0;
	}

	const double attackFactor = 1.0
		+ 0.05 * (attack - creature->getBaseAttack() - parameters.lowestAttack);
	const double defenseFactor = (1.0
		+ 0.05 * (defense - creature->getBaseDefense() - parameters.lowestDefense))
		* defenseModifier;
	double damageFactor = 1.0;
	const std::vector<SpellID> activeSpells = stack->activeSpells();
	if(vstd::contains(activeSpells, SpellID(SpellID::BLESS))
	   || vstd::contains(activeSpells, SpellID(SpellID::CURSE)))
	{
		const double baseAverage = (creature->getBaseDamageMin() + creature->getBaseDamageMax()) / 2.0;
		const bool cursed = stack->hasBonusOfType(BonusType::ALWAYS_MINIMUM_DAMAGE);
		const bool blessed = stack->hasBonusOfType(BonusType::ALWAYS_MAXIMUM_DAMAGE);
		const int32_t shift = stack->valOfBonuses(BonusType::ALWAYS_MAXIMUM_DAMAGE)
			- stack->valOfBonuses(BonusType::ALWAYS_MINIMUM_DAMAGE);
		const int32_t minimum = std::max(1, ClassicRulesAdapter::minDamage(stack, rangedAttack) + shift);
		const int32_t maximum = std::max(1, ClassicRulesAdapter::maxDamage(stack, rangedAttack) + shift);
		// Unit damage accessors expose the uncollapsed range; Bless and Curse
		// are applied by the combat script, not by getMinDamage/getMaxDamage.
		const double adjustedAverage = cursed != blessed
			? cursed ? minimum : maximum
			: (minimum + maximum) / 2.0;
		if(baseAverage > 0.0)
			damageFactor = adjustedAverage / baseAverage;
	}
	if(rangedAttack && stack->getTotalAttacks(true) > 1)
		damageFactor *= 2.0;
	if(creature->getId() == CreatureID(146))
	{
		static constexpr std::array<double, 4> artilleryFactors = {1.0, 1.5, 3.0, 4.0};
		int32_t mastery = 0;
		if(const CGHeroInstance * owner = ownerHero(battle, stack))
			mastery = owner->getSecSkillLevel(SecondarySkill::ARTILLERY);
		damageFactor *= artilleryFactors.at(std::clamp(mastery, 0, 3));
	}
	if(stack->isShooter() && !rangedAttack)
		damageFactor *= 0.5;

	double value = creature->getFightValue()
		* std::sqrt(attackFactor * defenseFactor * damageFactor);
	if(stack->hasBonusOfType(BonusType::SIEGE_WEAPON) || stack->summoned)
	{
		const int64_t specialHealth = stack->getAvailableHealth();
		if(specialHealth == 0)
			return 0.1;
		int64_t ordinaryHealth = 0;
		for(const CStack * candidate : view.orderedStacks())
		{
			if(candidate->unitSide() != stack->unitSide()
			   || candidate->hasBonusOfType(BonusType::SIEGE_WEAPON)
			   || candidate->summoned
			   || candidate->isClone())
				continue;
			ordinaryHealth += candidate->getAvailableHealth();
		}
		value *= static_cast<double>(ordinaryHealth) / (specialHealth + ordinaryHealth);
	}
	return value;
}

int64_t ClassicCombatValue::unitValue(const CStack * stack, const ClassicCombatParameters & parameters) const
{
	return truncateTowardZero(unitValueExact(stack, parameters, stack->isShooter()));
}

int64_t ClassicCombatValue::stackValue(const CStack * stack, const ClassicCombatParameters & parameters) const
{
	return stackValueAtHealth(stack, stack->getAvailableHealth(), parameters);
}

int64_t ClassicCombatValue::stackValue(
	const CStack * stack,
	const ClassicCombatParameters & parameters,
	bool ranged) const
{
	return stackValueAtHealth(stack, stack->getAvailableHealth(), parameters, ranged);
}

int64_t ClassicCombatValue::stackValueAtHealth(
	const CStack * stack,
	int64_t health,
	const ClassicCombatParameters & parameters) const

{
	return stackValueAtHealth(stack, health, parameters, stack->isShooter());
}

int64_t ClassicCombatValue::stackValueAtHealth(
	const CStack * stack,
	int64_t health,
	const ClassicCombatParameters & parameters,
	bool ranged) const
{
	if(health <= 0 || stack->isTurret())
		return 0;
	if(stack->isClone())
		return truncateTowardZero(unitValueExact(stack, parameters, ranged) * stack->getCount() / 5.0);
	return truncateTowardZero(
		unitValueExact(stack, parameters, ranged) * health / std::max<int64_t>(1, stack->getMaxHealth()));
}

int64_t ClassicCombatValue::lossValue(
	const CStack * stack,
	int64_t healthBefore,
	int64_t healthAfter,
	const ClassicCombatParameters & parameters) const

{
	return lossValue(stack, healthBefore, healthAfter, parameters, stack->isShooter(), parameters.killsOnly);
}

int64_t ClassicCombatValue::lossValue(
	const CStack * stack,
	int64_t healthBefore,
	int64_t healthAfter,
	const ClassicCombatParameters & parameters,
	bool ranged,
	bool killsOnly) const
{
	healthBefore = std::max<int64_t>(0, healthBefore);
	healthAfter = std::clamp<int64_t>(healthAfter, 0, healthBefore);
	const int64_t lostHealth = healthBefore - healthAfter;
	if(lostHealth <= 0)
		return 0;
	if(stack->isClone())
		return truncateTowardZero(unitValueExact(stack, parameters, ranged) * stack->getCount() / 5.0);

	// army::get_loss_combat_value values the hit points lost directly and
	// converts only the final product.  If the loss finishes the currently
	// wounded top creature, its already-missing hit points are added so that
	// the transition accounts for one complete creature.  This is observably
	// different from subtracting two converted total-stack values.
	const int64_t hitPoints = std::max<int64_t>(1, stack->getMaxHealth());
	const int64_t residualDamage = std::clamp<int64_t>(
		hitPoints - stack->getFirstHPleft(), 0, hitPoints - 1);
	int64_t valuedHealth = lostHealth;
	if(lostHealth % hitPoints + residualDamage >= hitPoints)
		valuedHealth += residualDamage;
	const double valuePerCreature = killsOnly ? 1000.0 : unitValueExact(stack, parameters, ranged);
	return truncateTowardZero(valuePerCreature * valuedHealth / hitPoints);
}

int64_t ClassicCombatValue::sideValue(
	BattleSide side,
	const ClassicCombatParameters & parameters,
	bool includeCripples) const
{
	int64_t result = 0;
	ClassicBattleStateView view(battle);
	for(const CStack * stack : view.orderedStacks())
	{
		if(view.controllingSide(stack) != side || stack->isTurret())
			continue;
		if(!includeCripples && excludedFromAwakeValue(stack))
			continue;
		result += stackValue(stack, parameters);
	}
	return result;
}
