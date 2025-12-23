/*
 * DamageCalculator.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "DamageCalculator.h"
#include "CBattleInfoCallback.h"
#include "Unit.h"

#include "../bonuses/Bonus.h"
#include "../CCreatureHandler.h"
#include "../mapObjects/CGTownInstance.h"
#include "../spells/CSpellHandler.h"
#include "../IGameSettings.h"
#include "../GameLibrary.h"


VCMI_LIB_NAMESPACE_BEGIN

DamageRange DamageCalculator::getBaseDamageSingle() const
{
	int64_t minDmg = 0.0;
	int64_t maxDmg = 0.0;

	minDmg = info.attacker->getMinDamage(info.shooting);
	maxDmg = info.attacker->getMaxDamage(info.shooting);

	if(minDmg > maxDmg)
	{
	const auto & creatureName = info.attacker->creatureId().toEntity(LIBRARY)->getNamePluralTranslated();
	logGlobal->error("Creature %s: min damage %lld exceeds max damage %lld.", creatureName, minDmg, maxDmg);
		logGlobal->error("This may lead to unexpected results, please report it to the mod's creator.");
		// to avoid an RNG crash and make bless and curse spells work as expected
		std::swap(minDmg, maxDmg);
	}

	if(info.attacker->creatureIndex() == CreatureID::ARROW_TOWERS)
	{
		const auto * town = callback.battleGetDefendedTown();
		assert(town);

		switch(info.attacker->getPosition().toInt())
		{
		case BattleHex::CASTLE_CENTRAL_TOWER:
			return town->getKeepDamageRange();
		case BattleHex::CASTLE_BOTTOM_TOWER:
		case BattleHex::CASTLE_UPPER_TOWER:
			return town->getTowerDamageRange();
		default:
			assert(0);
		}
	}

	const std::string cachingStrSiedgeWeapon = "type_SIEGE_WEAPON";
	static const auto selectorSiedgeWeapon = Selector::type()(BonusType::SIEGE_WEAPON);

	if(info.attacker->hasBonus(selectorSiedgeWeapon, cachingStrSiedgeWeapon) && info.attacker->creatureIndex() != CreatureID::ARROW_TOWERS)
	{
		static const auto bonusSelector =
			Selector::sourceTypeSel(BonusSource::ARTIFACT).Or(
			Selector::sourceTypeSel(BonusSource::HERO_BASE_SKILL)).And(
			Selector::typeSubtype(BonusType::PRIMARY_SKILL, BonusSubtypeID(PrimarySkill::ATTACK)));

		//minDmg and maxDmg of a Ballista are multiplied by hero attack + 1
		int heroAttackSkill = info.attacker->valOfBonuses(bonusSelector);
		minDmg *= heroAttackSkill + 1;
		maxDmg *= heroAttackSkill + 1;
	}
	return { minDmg, maxDmg };
}

DamageRange DamageCalculator::getBaseDamageBlessCurse() const
{
	const std::string cachingStrForcedMinDamage = "type_ALWAYS_MINIMUM_DAMAGE";
	static const auto selectorForcedMinDamage = Selector::type()(BonusType::ALWAYS_MINIMUM_DAMAGE);

	const std::string cachingStrForcedMaxDamage = "type_ALWAYS_MAXIMUM_DAMAGE";
	static const auto selectorForcedMaxDamage = Selector::type()(BonusType::ALWAYS_MAXIMUM_DAMAGE);

	TConstBonusListPtr curseEffects = info.attacker->getBonuses(selectorForcedMinDamage, cachingStrForcedMinDamage);
	TConstBonusListPtr blessEffects = info.attacker->getBonuses(selectorForcedMaxDamage, cachingStrForcedMaxDamage);

	int curseBlessAdditiveModifier = blessEffects->totalValue() - curseEffects->totalValue();

	DamageRange baseDamage = getBaseDamageSingle();
	DamageRange modifiedDamage = {
		std::max(static_cast<int64_t>(1), baseDamage.min + curseBlessAdditiveModifier),
		std::max(static_cast<int64_t>(1), baseDamage.max + curseBlessAdditiveModifier)
	};

	if(curseEffects->size() && blessEffects->size() )
	{
		logGlobal->warn("Stack has both curse and bless! Effects will negate each other!");
		return modifiedDamage;
	}

	if(curseEffects->size())
	{
		return {
			modifiedDamage.min,
			modifiedDamage.min
		};
	}

	if(blessEffects->size())
	{
		return {
			modifiedDamage.max,
			modifiedDamage.max
		};
	}

	return modifiedDamage;
}

DamageRange DamageCalculator::getBaseDamageStack() const
{
	auto stackSize = info.attacker->getCount();
	auto baseDamage = getBaseDamageBlessCurse();
	return {
		baseDamage.min * stackSize,
		baseDamage.max * stackSize
	};
}

int DamageCalculator::getActorAttackBase() const
{
	return info.attacker->getAttack(info.shooting);
}

int DamageCalculator::getActorAttackEffective() const
{
	return getActorAttackBase() + getActorAttackSlayer() + getActorAttackIgnored();
}

int DamageCalculator::getActorAttackIgnored() const
{
	int multAttackReductionPercent = battleBonusValue(info.defender, Selector::type()(BonusType::ENEMY_ATTACK_REDUCTION));

	if(multAttackReductionPercent > 0)
	{
		//using ints so 1.5 for 5 attack is rounded down as in HotA / h3assist etc. (keep in mind h3assist 1.2 shows wrong value for 15 attack points and unupg. nix)
		int reduction = vstd::divideAndRound( getActorAttackBase() * multAttackReductionPercent, 100);
		return -std::min(reduction, getActorAttackBase());
	}
	return 0;
}

int DamageCalculator::getActorAttackSlayer() const
{
	const std::string cachingStrSlayer = "type_SLAYER";
	static const auto selectorSlayer = Selector::type()(BonusType::SLAYER);

	if (!info.defender->hasBonusOfType(BonusType::KING))
		return 0;

	auto slayerEffects = info.attacker->getBonuses(selectorSlayer, cachingStrSlayer);
	auto slayerAffected = info.defender->unitType()->valOfBonuses(BonusType::KING);

	if(std::shared_ptr<const Bonus> slayerEffect = slayerEffects->getFirst(Selector::all))
	{
		const auto spLevel = slayerEffect->val;
		bool isAffected = spLevel >= slayerAffected;

		if(isAffected)
		{
			SpellID spell(SpellID::SLAYER);
			int attackBonus = spell.toSpell()->getLevelPower(spLevel);
			if(info.attacker->hasBonusOfType(BonusType::SPECIAL_PECULIAR_ENCHANT, BonusSubtypeID(spell)))
			{
				ui8 attackerTier = info.attacker->unitType()->getLevel();
				ui8 specialtyBonus = std::max(5 - attackerTier, 0);
				attackBonus += specialtyBonus;
			}
			return attackBonus;
		}
	}
	return 0;
}

int DamageCalculator::getTargetDefenseBase() const
{
	return info.defender->getDefense(info.shooting);
}

int DamageCalculator::getTargetDefenseEffective() const
{
	return getTargetDefenseBase() + getTargetDefenseIgnored();
}

int DamageCalculator::getTargetDefenseIgnored() const
{
	double multDefenceReduction = battleBonusValue(info.attacker, Selector::type()(BonusType::ENEMY_DEFENCE_REDUCTION)) / 100.0;

	if(multDefenceReduction > 0)
	{
		int reduction = std::floor(multDefenceReduction * getTargetDefenseBase()) + 1;
		return -std::min(reduction,getTargetDefenseBase());
	}
	return 0;
}

double DamageCalculator::getAttackSkillFactor() const
{
	int attackAdvantage = getActorAttackEffective() - getTargetDefenseEffective();

	if(attackAdvantage > 0)
	{
		// FIXME: use cb to acquire these settings
		const double attackMultiplier = LIBRARY->engineSettings()->getDouble(EGameSettings::COMBAT_ATTACK_POINT_DAMAGE_FACTOR);
		const double attackMultiplierCap = LIBRARY->engineSettings()->getDouble(EGameSettings::COMBAT_ATTACK_POINT_DAMAGE_FACTOR_CAP);
		const double attackFactor = std::min(attackMultiplier * attackAdvantage, attackMultiplierCap);

		return attackFactor;
	}
	return 0.f;
}

double DamageCalculator::getAttackBlessFactor() const
{
	const std::string cachingStrDamage = "type_GENERAL_DAMAGE_PREMY";
	static const auto selectorDamage = Selector::type()(BonusType::GENERAL_DAMAGE_PREMY);
	return info.attacker->valOfBonuses(selectorDamage, cachingStrDamage) / 100.0;
}

double DamageCalculator::getAttackOffenseArcheryFactor() const
{
	
	if(info.shooting)
	{
		const std::string cachingStrArchery = "type_PERCENTAGE_DAMAGE_BOOSTs_1";
		static const auto selectorArchery = Selector::typeSubtype(BonusType::PERCENTAGE_DAMAGE_BOOST, BonusCustomSubtype::damageTypeRanged);
		return info.attacker->valOfBonuses(selectorArchery, cachingStrArchery) / 100.0;
	}
	const std::string cachingStrOffence = "type_PERCENTAGE_DAMAGE_BOOSTs_0";
	static const auto selectorOffence = Selector::typeSubtype(BonusType::PERCENTAGE_DAMAGE_BOOST, BonusCustomSubtype::damageTypeMelee);
	return info.attacker->valOfBonuses(selectorOffence, cachingStrOffence) / 100.0;
}

double DamageCalculator::getAttackLuckFactor() const
{
	if(info.luckyStrike)
		return 1.0;
	return 0.0;
}

double DamageCalculator::getAttackDeathBlowFactor() const
{
	if(info.deathBlow)
		return 1.0;
	return 0.0;
}

double DamageCalculator::getAttackDoubleDamageFactor() const
{
	if(info.doubleDamage) {
		const auto cachingStr = "type_BONUS_DAMAGE_PERCENTAGEs_" + std::to_string(info.attacker->creatureIndex());
		const auto selector = Selector::typeSubtype(BonusType::BONUS_DAMAGE_PERCENTAGE, BonusSubtypeID(info.attacker->creatureId()));
		return info.attacker->valOfBonuses(selector, cachingStr) / 100.0;
	}
	return 0.0;
}

double DamageCalculator::getAttackJoustingFactor() const
{
	//applying jousting bonus
	if(info.chargeDistance > 0 && info.attacker->hasBonusOfType(BonusType::JOUSTING) && !info.defender->hasBonusOfType(BonusType::CHARGE_IMMUNITY))
		return info.chargeDistance * (info.attacker->valOfBonuses(BonusType::JOUSTING))/100.0;
	return 0.0;
}

double DamageCalculator::getAttackFromBackFactor() const
{
	int value = info.defender->valOfBonuses(BonusType::VULNERABLE_FROM_BACK);

	if (value != 0 && callback.isToReverse(info.attacker, info.defender, info.attackerPos, info.defenderPos))
		return value / 100.0;
	return 0;
}

double DamageCalculator::getAttackHateCreatureFactor() const
{
	//assume that unit have only few HATE features and cache them all
	auto allHateEffects = info.attacker->getBonusesOfType(BonusType::HATE);
	return allHateEffects->valOfBonuses(Selector::subtype()(BonusSubtypeID(info.defender->creatureId()))) / 100.0;
}

double DamageCalculator::getAttackHateTraitFactor() const
{
	//assume that unit have only few HATE features and cache them all
	auto allHateEffects = info.attacker->getBonusesOfType(BonusType::HATES_TRAIT);

	auto selector = [this](const Bonus* hateBonus) -> bool
	{
		return info.defender->hasBonusOfType(hateBonus->subtype.as<BonusTypeID>().toEnum());
	};

	return allHateEffects->valOfBonuses(selector) / 100.0;
}

double DamageCalculator::getAttackRevengeFactor() const
{
	if(info.attacker->hasBonusOfType(BonusType::REVENGE)) //HotA Haspid ability
	{
		int totalStackCount = info.attacker->unitBaseAmount();
		int currentStackHealth = info.attacker->getAvailableHealth();
		int creatureHealth = info.attacker->getMaxHealth();

		return sqrt(static_cast<double>((totalStackCount + 1) * creatureHealth) / (currentStackHealth + creatureHealth) - 1);
	}

	return 0.0;
}

double DamageCalculator::getDefenseSkillFactor() const
{
	int defenseAdvantage = getTargetDefenseEffective() - getActorAttackEffective();

	//bonus from attack/defense skills
	if(defenseAdvantage > 0) //decreasing dmg
	{
		// FIXME: use cb to acquire these settings
		const double defenseMultiplier = LIBRARY->engineSettings()->getDouble(EGameSettings::COMBAT_DEFENSE_POINT_DAMAGE_FACTOR);
		const double defenseMultiplierCap = LIBRARY->engineSettings()->getDouble(EGameSettings::COMBAT_DEFENSE_POINT_DAMAGE_FACTOR_CAP);

		const double dec = std::min(defenseMultiplier * defenseAdvantage, defenseMultiplierCap);
		return dec;
	}
	return 0.0;
}

double DamageCalculator::getDefenseArmorerFactor() const
{
	const std::string cachingStrArmorer = "type_GENERAL_DAMAGE_REDUCTIONs_N1_NsrcSPELL_EFFECT";
	static const auto selectorArmorer = Selector::typeSubtype(BonusType::GENERAL_DAMAGE_REDUCTION, BonusCustomSubtype::damageTypeAll).And(Selector::sourceTypeSel(BonusSource::SPELL_EFFECT).Not());
	return info.defender->valOfBonuses(selectorArmorer, cachingStrArmorer) / 100.0;

}

double DamageCalculator::getDefenseMagicShieldFactor() const
{
	const std::string cachingStrMeleeReduction = "type_GENERAL_DAMAGE_REDUCTIONs_0";
	static const auto selectorMeleeReduction = Selector::typeSubtype(BonusType::GENERAL_DAMAGE_REDUCTION, BonusCustomSubtype::damageTypeMelee);

	const std::string cachingStrRangedReduction = "type_GENERAL_DAMAGE_REDUCTIONs_1";
	static const auto selectorRangedReduction = Selector::typeSubtype(BonusType::GENERAL_DAMAGE_REDUCTION, BonusCustomSubtype::damageTypeRanged);

	//handling spell effects - shield and air shield
	if(info.shooting)
		return info.defender->valOfBonuses(selectorRangedReduction, cachingStrRangedReduction) / 100.0;
	else
		return info.defender->valOfBonuses(selectorMeleeReduction, cachingStrMeleeReduction) / 100.0;
}

double DamageCalculator::getDefenseRangePenaltiesFactor() const
{
	if(info.shooting)
	{
		BattleHex attackerPos = info.attackerPos.isValid() ? info.attackerPos : info.attacker->getPosition();
		BattleHex defenderPos = info.defenderPos.isValid() ? info.defenderPos : info.defender->getPosition();

		const std::string cachingStrAdvAirShield = "isAdvancedAirShield";
		auto isAdvancedAirShield = [](const Bonus* bonus)
		{
			return bonus->source == BonusSource::SPELL_EFFECT
					&& bonus->sid == BonusSourceID(SpellID(SpellID::AIR_SHIELD))
					&& bonus->val >= MasteryLevel::ADVANCED;
		};

		const bool distPenalty = callback.battleHasDistancePenalty(info.attacker, attackerPos, defenderPos);

		if(distPenalty || info.defender->hasBonus(isAdvancedAirShield, cachingStrAdvAirShield))
			return 0.5;

	}
	else
	{
		const std::string cachingStrNoMeleePenalty = "type_NO_MELEE_PENALTY";
		static const auto selectorNoMeleePenalty = Selector::type()(BonusType::NO_MELEE_PENALTY);

		if(info.attacker->isShooter() && !info.attacker->hasBonus(selectorNoMeleePenalty, cachingStrNoMeleePenalty))
			return 0.5;
	}
	return 0.0;
}

double DamageCalculator::getDefenseObstacleFactor() const
{
	if(info.shooting)
	{
		BattleHex attackerPos = info.attackerPos.isValid() ? info.attackerPos : info.attacker->getPosition();
		BattleHex defenderPos = info.defenderPos.isValid() ? info.defenderPos : info.defender->getPosition();

		const bool obstaclePenalty = callback.battleHasWallPenalty(info.attacker, attackerPos, defenderPos);
		if(obstaclePenalty)
			return 0.5;
	}
	return 0.0;
}

double DamageCalculator::getDefenseUnluckyFactor() const
{
	if(info.unluckyStrike)
		return 0.5;
	return 0.0;
}

double DamageCalculator::getDefenseBlindParalysisFactor() const
{
	double multAttackReduction = battleBonusValue(info.attacker, Selector::type()(BonusType::GENERAL_ATTACK_REDUCTION)) / 100.0;
	return multAttackReduction;
}

double DamageCalculator::getDefenseForgetfulnessFactor() const
{
	if(info.shooting)
	{
		//todo: set actual percentage in spell bonus configuration instead of just level; requires non trivial backward compatibility handling
		//get list first, total value of 0 also counts
		TConstBonusListPtr forgetfulList = info.attacker->getBonusesOfType(BonusType::FORGETFULL);

		if(!forgetfulList->empty())
		{
			int forgetful = forgetfulList->valOfBonuses(Selector::all);

			//none of basic level
			if(forgetful == 0 || forgetful == 1)
				return 0.5;
			else
				logGlobal->warn("Attempt to calculate shooting damage with adv+ FORGETFULL effect");
		}
	}
	return 0.0;
}

double DamageCalculator::getDefensePetrificationFactor() const
{
	// Creatures that are petrified by a Basilisk's Petrifying attack or a Medusa's Stone gaze take 50% damage (R8 = 0.50) from ranged and melee attacks. Taking damage also deactivates the effect.
	const std::string cachingStrAllReduction = "type_GENERAL_DAMAGE_REDUCTIONs_N1_srcSPELL_EFFECT";
	static const auto selectorAllReduction = Selector::typeSubtype(BonusType::GENERAL_DAMAGE_REDUCTION, BonusCustomSubtype::damageTypeAll).And(Selector::sourceTypeSel(BonusSource::SPELL_EFFECT));

	return info.defender->valOfBonuses(selectorAllReduction, cachingStrAllReduction) / 100.0;
}

double DamageCalculator::getDefenseMagicFactor() const
{
	// Magic Elementals deal half damage (R8 = 0.50) against Magic Elementals and Black Dragons. This is not affected by the Orb of Vulnerability, Anti-Magic, or Magic Resistance.
	if(info.attacker->creatureIndex() == CreatureID::MAGIC_ELEMENTAL)
	{
		const std::string cachingStrMagicImmunity = "type_LEVEL_SPELL_IMMUNITY";
		static const auto selectorMagicImmunity = Selector::type()(BonusType::LEVEL_SPELL_IMMUNITY);

		if(info.defender->valOfBonuses(selectorMagicImmunity, cachingStrMagicImmunity) >= 5)
			return 0.5;
	}
	return 0.0;
}

double DamageCalculator::getDefenseMindFactor() const
{
	// Psychic Elementals deal half damage (R8 = 0.50) against creatures that are immune to Mind spells, such as Giants and Undead. This is not affected by the Orb of Vulnerability.
	if(info.attacker->creatureIndex() == CreatureID::PSYCHIC_ELEMENTAL)
	{
		const std::string cachingStrMindImmunity = "type_MIND_IMMUNITY";
		static const auto selectorMindImmunity = Selector::type()(BonusType::MIND_IMMUNITY);

		if(info.defender->hasBonus(selectorMindImmunity, cachingStrMindImmunity))
			return 0.5;
	}
	return 0.0;
}

std::vector<double> DamageCalculator::getAttackFactors() const
{
	return {
		getAttackSkillFactor(),
		getAttackOffenseArcheryFactor(),
		getAttackBlessFactor(),
		getAttackLuckFactor(),
		getAttackJoustingFactor(),
		getAttackFromBackFactor(),
		getAttackDeathBlowFactor(),
		getAttackDoubleDamageFactor(),
		getAttackHateCreatureFactor(),
		getAttackHateTraitFactor(),
		getAttackRevengeFactor()
	};
}

std::vector<double> DamageCalculator::getDefenseFactors() const
{
	return {
		getDefenseSkillFactor(),
		getDefenseArmorerFactor(),
		getDefenseMagicShieldFactor(),
		getDefenseRangePenaltiesFactor(),
		getDefenseObstacleFactor(),
		getDefenseBlindParalysisFactor(),
		getDefenseUnluckyFactor(),
		getDefenseForgetfulnessFactor(),
		getDefensePetrificationFactor(),
		getDefenseMagicFactor(),
		getDefenseMindFactor()
	};
}

DamageRange DamageCalculator::getCasualties(const DamageRange & damageDealt) const
{
	return {
		getCasualties(damageDealt.min),
		getCasualties(damageDealt.max),
	};
}

int64_t DamageCalculator::getCasualties(int64_t damageDealt) const
{
	if (damageDealt < info.defender->getFirstHPleft())
		return 0;

	int64_t damageLeft = damageDealt - info.defender->getFirstHPleft();
	int64_t killsLeft = damageLeft / info.defender->getMaxHealth();

	return std::min<int32_t>(1 + killsLeft, info.defender->getCount());
}

int DamageCalculator::battleBonusValue(const IBonusBearer * bearer, const CSelector & selector) const
{
	auto noLimit = Selector::effectRange()(BonusLimitEffect::NO_LIMIT);
	auto limitMatches = info.shooting
						? Selector::effectRange()(BonusLimitEffect::ONLY_DISTANCE_FIGHT)
						: Selector::effectRange()(BonusLimitEffect::ONLY_MELEE_FIGHT);

	//any regular bonuses or just ones for melee/ranged
	return bearer->getBonuses(selector)->valOfBonuses(noLimit.Or(limitMatches));
};

int64_t DamageCalculator::getDamageCap() const
{
	const std::string cachingStrDamageCap = "type_DAMAGE_RECEIVED_CAP";
	static const auto selectorDamageCap = Selector::type()(BonusType::DAMAGE_RECEIVED_CAP);

	int damageCapPercentage = info.defender->valOfBonuses(selectorDamageCap, cachingStrDamageCap);
	if (damageCapPercentage <= 0)
		return std::numeric_limits<int64_t>::max();

	return info.defender->getMaxHealth() * damageCapPercentage / 100;
}

DamageEstimation DamageCalculator::calculateDmgRange() const
{
	DamageRange damageBase = getBaseDamageStack();

	auto attackFactors = getAttackFactors();
	auto defenseFactors = getDefenseFactors();

	double attackFactorTotal = 1.0;
	double defenseFactorTotal = 1.0;

	for (auto & factor : attackFactors)
	{
		assert(factor >= 0.0);
		attackFactorTotal += factor;
	}

	for (auto & factor : defenseFactors)
	{
		assert(factor >= 0.0);
		defenseFactorTotal *= (1 - std::min(1.0, factor));
	}

	double resultingFactor = attackFactorTotal * defenseFactorTotal;

	int64_t avail = info.defender->getAvailableHealth();
	int64_t cap = getDamageCap();

	auto dmin = std::max<int64_t>(1.0, std::floor(damageBase.min * resultingFactor));
	auto dmax = std::max<int64_t>(1.0, std::floor(damageBase.max * resultingFactor));

	dmin = std::min({dmin, avail, cap});
	dmax = std::min({dmax, avail, cap});

	DamageRange damageDealt{ dmin, dmax };

	DamageRange killsDealt = getCasualties(damageDealt);

	return DamageEstimation{damageDealt, killsDealt};
}

VCMI_LIB_NAMESPACE_END
