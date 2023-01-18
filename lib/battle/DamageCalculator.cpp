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

#include "../HeroBonus.h"
#include "../mapObjects/CGTownInstance.h"
#include "../spells/CSpellHandler.h"
#include "../CModHandler.h"

namespace SiegeStuffThatShouldBeMovedToHandlers // <=== TODO
{

static void retrieveTurretDamageRange(const CGTownInstance * town, const battle::Unit * turret, double & outMinDmg, double & outMaxDmg)
{
	// http://heroes.thelazy.net/wiki/Arrow_tower
	assert(turret->creatureIndex() == CreatureID::ARROW_TOWERS);
	assert(town);
	assert(turret->getPosition() >= -4 && turret->getPosition() <= -2);

	// base damage, irregardless of town level
	static const int baseDamageKeep = 10;
	static const int baseDamageTower = 6;

	// extra damage, for each building in town
	static const int extraDamage = 2;

	const int townLevel = town->getTownLevel();

	int minDamage;

	if (turret->getPosition() == BattleHex::CASTLE_CENTRAL_TOWER)
		minDamage = baseDamageKeep + townLevel * extraDamage;
	else
		minDamage = baseDamageTower + townLevel / 2 * extraDamage;

	outMinDmg = minDamage;
	outMaxDmg = minDamage * 2;
}

}

TDmgRange DamageCalculator::getBaseDamageSingle() const
{
	double minDmg = 0.0;
	double maxDmg = 0.0;

	minDmg = info.attacker->getMinDamage(info.shooting);
	maxDmg = info.attacker->getMaxDamage(info.shooting);

	if(info.attacker->creatureIndex() == CreatureID::ARROW_TOWERS)
		SiegeStuffThatShouldBeMovedToHandlers::retrieveTurretDamageRange(callback.battleGetDefendedTown(), info.attacker, minDmg, maxDmg);

	const std::string cachingStrSiedgeWeapon = "type_SIEGE_WEAPON";
	static const auto selectorSiedgeWeapon = Selector::type()(Bonus::SIEGE_WEAPON);

	if(info.attacker->hasBonus(selectorSiedgeWeapon, cachingStrSiedgeWeapon) && info.attacker->creatureIndex() != CreatureID::ARROW_TOWERS) //any siege weapon, but only ballista can attack (second condition - not arrow turret)
	{ //minDmg and maxDmg are multiplied by hero attack + 1
		auto retrieveHeroPrimSkill = [&](int skill) -> int
		{
			std::shared_ptr<const Bonus> b = info.attacker->getBonus(Selector::sourceTypeSel(Bonus::HERO_BASE_SKILL).And(Selector::typeSubtype(Bonus::PRIMARY_SKILL, skill)));
			return b ? b->val : 0; //if there is no hero or no info on his primary skill, return 0
		};

		minDmg *= retrieveHeroPrimSkill(PrimarySkill::ATTACK) + 1;
		maxDmg *= retrieveHeroPrimSkill(PrimarySkill::ATTACK) + 1;
	}
	return { minDmg, maxDmg };
}

TDmgRange DamageCalculator::getBaseDamageBlessCurse() const
{
	const std::string cachingStrForcedMinDamage = "type_ALWAYS_MINIMUM_DAMAGE";
	static const auto selectorForcedMinDamage = Selector::type()(Bonus::ALWAYS_MINIMUM_DAMAGE);

	const std::string cachingStrForcedMaxDamage = "type_ALWAYS_MAXIMUM_DAMAGE";
	static const auto selectorForcedMaxDamage = Selector::type()(Bonus::ALWAYS_MAXIMUM_DAMAGE);

	TConstBonusListPtr curseEffects = info.attacker->getBonuses(selectorForcedMinDamage, cachingStrForcedMinDamage);
	TConstBonusListPtr blessEffects = info.attacker->getBonuses(selectorForcedMaxDamage, cachingStrForcedMaxDamage);

	int curseBlessAdditiveModifier = blessEffects->totalValue() - curseEffects->totalValue();

	TDmgRange baseDamage = getBaseDamageSingle();
	TDmgRange modifiedDamage = {
		std::max( int64_t(1), baseDamage.first  + curseBlessAdditiveModifier),
		std::max( int64_t(1), baseDamage.second + curseBlessAdditiveModifier)
	};

	if ( curseEffects->size() && blessEffects->size() )
	{
		logGlobal->warn("Stack has both curse and bless! Effects will negate each other!");
		return modifiedDamage;
	}

	if(curseEffects->size())
	{
		return {
			modifiedDamage.first,
			modifiedDamage.first
		};
	}

	if(blessEffects->size())
	{
		return {
			modifiedDamage.second,
			modifiedDamage.second
		};
	}

	return modifiedDamage;
}

TDmgRange DamageCalculator::getBaseDamageStack() const
{
	auto stackSize = info.attacker->getCount();
	auto baseDamage = getBaseDamageBlessCurse();
	return {
		baseDamage.first * stackSize,
		baseDamage.second * stackSize
	};
}

int DamageCalculator::getActorAttackBase() const
{
	return info.attacker->getAttack(info.shooting);
}

int DamageCalculator::getActorAttackEffective() const
{
	return getActorAttackBase() + getActorAttackSlayer();
}

int DamageCalculator::getActorAttackSlayer() const
{
	const std::string cachingStrSlayer = "type_SLAYER";
	static const auto selectorSlayer = Selector::type()(Bonus::SLAYER);

	auto slayerEffects = info.attacker->getBonuses(selectorSlayer, cachingStrSlayer);

	if(std::shared_ptr<const Bonus> slayerEffect = slayerEffects->getFirst(Selector::all))
	{
		std::vector<int32_t> affectedIds;
		const auto spLevel = slayerEffect->val;
		const CCreature * defenderType = info.defender->unitType();
		bool isAffected = false;

		for(const auto & b : defenderType->getBonusList())
		{
			if((b->type == Bonus::KING3 && spLevel >= 3) || //expert
				(b->type == Bonus::KING2 && spLevel >= 2) || //adv +
				(b->type == Bonus::KING1 && spLevel >= 0)) //none or basic +
			{
				isAffected = true;
				break;
			}
		}

		if(isAffected)
		{
			int attackBonus = SpellID(SpellID::SLAYER).toSpell()->getLevelPower(spLevel);
			if(info.attacker->hasBonusOfType(Bonus::SPECIAL_PECULIAR_ENCHANT, SpellID::SLAYER))
			{
				ui8 attackerTier = info.attacker->unitType()->level;
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
	double multDefenceReduction = battleBonusValue(info.attacker, Selector::type()(Bonus::ENEMY_DEFENCE_REDUCTION)) / 100.0;

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

	if (attackAdvantage > 0)
	{
		const double attackMultiplier = VLC->modh->settings.ATTACK_POINT_DMG_MULTIPLIER;
		const double attackMultiplierCap = VLC->modh->settings.ATTACK_POINTS_DMG_MULTIPLIER_CAP;
		const double attackFactor = std::min(attackMultiplier * attackAdvantage, attackMultiplierCap);

		return attackFactor;
	}
	return 0.f;
}

double DamageCalculator::getAttackBlessFactor() const
{
	const std::string cachingStrDamage = "type_GENERAL_DAMAGE_PREMY";
	static const auto selectorDamage = Selector::type()(Bonus::GENERAL_DAMAGE_PREMY);
	return info.attacker->valOfBonuses(selectorDamage, cachingStrDamage) / 100.0;
}

double DamageCalculator::getAttackOffenseArcheryFactor() const
{
	if(info.shooting)
	{
		const std::string cachingStrArchery = "type_SECONDARY_SKILL_PREMYs_ARCHERY";
		static const auto selectorArchery = Selector::typeSubtype(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::ARCHERY);
		return info.attacker->valOfBonuses(selectorArchery, cachingStrArchery) / 100.0;
	}
	else
	{
		const std::string cachingStrOffence = "type_SECONDARY_SKILL_PREMYs_OFFENCE";
		static const auto selectorOffence = Selector::typeSubtype(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::OFFENCE);
		return info.attacker->valOfBonuses(selectorOffence, cachingStrOffence) / 100.0;
	}
}

double DamageCalculator::getAttackLuckFactor() const
{
	if (info.luckyStrike)
		return 1.0;
	return 0.0;
}

double DamageCalculator::getAttackDeathBlowFactor() const
{
	if (info.deathBlow)
		return 1.0;
	return 0.0;
}

double DamageCalculator::getAttackDoubleDamageFactor() const
{
	if (info.doubleDamage)
		return 1.0;
	return 0.0;
}

double DamageCalculator::getAttackJoustingFactor() const
{
	const std::string cachingStrJousting = "type_JOUSTING";
	static const auto selectorJousting = Selector::type()(Bonus::JOUSTING);

	const std::string cachingStrChargeImmunity = "type_CHARGE_IMMUNITY";
	static const auto selectorChargeImmunity = Selector::type()(Bonus::CHARGE_IMMUNITY);

	//applying jousting bonus
	if(info.chargeDistance > 0 && info.attacker->hasBonus(selectorJousting, cachingStrJousting) && !info.defender->hasBonus(selectorChargeImmunity, cachingStrChargeImmunity))
		return info.chargeDistance * 0.05;
	return 0.0;
}

double DamageCalculator::getAttackHateFactor() const
{
	//assume that unit have only few HATE features and cache them all
	const std::string cachingStrHate = "type_HATE";
	static const auto selectorHate = Selector::type()(Bonus::HATE);

	auto allHateEffects = info.attacker->getBonuses(selectorHate, cachingStrHate);

	return allHateEffects->valOfBonuses(Selector::subtype()(info.defender->creatureIndex())) / 100.0;
}

double DamageCalculator::getDefenseSkillFactor() const
{
	int defenseAdvantage = getTargetDefenseEffective() - getActorAttackEffective();

	//bonus from attack/defense skills
	if(defenseAdvantage > 0) //decreasing dmg
	{
		const double defenseMultiplier = VLC->modh->settings.DEFENSE_POINT_DMG_MULTIPLIER;
		const double defenseMultiplierCap = VLC->modh->settings.DEFENSE_POINTS_DMG_MULTIPLIER_CAP;

		const double dec = std::min(defenseMultiplier * defenseAdvantage, defenseMultiplierCap);
		return dec;
	}
	return 0.0;
}

double DamageCalculator::getDefenseArmorerFactor() const
{
	const std::string cachingStrArmorer = "type_SECONDARY_SKILL_PREMYs_ARMORER";
	static const auto selectorArmorer = Selector::typeSubtype(Bonus::SECONDARY_SKILL_PREMY, SecondarySkill::ARMORER);
	return info.defender->valOfBonuses(selectorArmorer, cachingStrArmorer) / 100.0;

}

double DamageCalculator::getDefenseMagicShieldFactor() const
{
	const std::string cachingStrMeleeReduction = "type_GENERAL_DAMAGE_REDUCTIONs_0";
	static const auto selectorMeleeReduction = Selector::typeSubtype(Bonus::GENERAL_DAMAGE_REDUCTION, 0);

	const std::string cachingStrRangedReduction = "type_GENERAL_DAMAGE_REDUCTIONs_1";
	static const auto selectorRangedReduction = Selector::typeSubtype(Bonus::GENERAL_DAMAGE_REDUCTION, 1);

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
			return bonus->source == Bonus::SPELL_EFFECT
					&& bonus->sid == SpellID::AIR_SHIELD
					&& bonus->val >= SecSkillLevel::ADVANCED;
		};

		const bool distPenalty = callback.battleHasDistancePenalty(info.attacker, attackerPos, defenderPos);

		if(distPenalty || info.defender->hasBonus(isAdvancedAirShield, cachingStrAdvAirShield))
			return 0.5;

	}
	else
	{
		const std::string cachingStrNoMeleePenalty = "type_NO_MELEE_PENALTY";
		static const auto selectorNoMeleePenalty = Selector::type()(Bonus::NO_MELEE_PENALTY);

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
	if (info.unluckyStrike)
		return 0.5;
	return 0.0;
}

double DamageCalculator::getDefenseBlindParalysisFactor() const
{

	double multAttackReduction = battleBonusValue(info.attacker, Selector::type()(Bonus::GENERAL_ATTACK_REDUCTION)) / 100.0;

	return multAttackReduction;
}

double DamageCalculator::getDefenseForgetfulnessFactor() const
{
	if(info.shooting)
	{
		//todo: set actual percentage in spell bonus configuration instead of just level; requires non trivial backward compatibility handling
		//get list first, total value of 0 also counts
		TConstBonusListPtr forgetfulList = info.attacker->getBonuses(Selector::type()(Bonus::FORGETFULL),"type_FORGETFULL");

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
	const std::string cachingStrAllReduction = "type_GENERAL_DAMAGE_REDUCTIONs_N1";
	static const auto selectorAllReduction = Selector::typeSubtype(Bonus::GENERAL_DAMAGE_REDUCTION, -1);

	return info.defender->valOfBonuses(selectorAllReduction, cachingStrAllReduction) / 100.0;
}

double DamageCalculator::getDefenseMagicFactor() const
{
	// Magic Elementals deal half damage (R8 = 0.50) against Magic Elementals and Black Dragons. This is not affected by the Orb of Vulnerability, Anti-Magic, or Magic Resistance.
	if(info.attacker->creatureIndex() == CreatureID::MAGIC_ELEMENTAL)
	{
		const std::string cachingStrMagicImmunity = "type_LEVEL_SPELL_IMMUNITY";
		static const auto selectorMagicImmunity = Selector::type()(Bonus::LEVEL_SPELL_IMMUNITY);

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
		static const auto selectorMindImmunity = Selector::type()(Bonus::MIND_IMMUNITY);

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
		getAttackDeathBlowFactor(),
		getAttackDoubleDamageFactor(),
		getAttackHateFactor()
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

int DamageCalculator::battleBonusValue(const IBonusBearer * bearer, const CSelector & selector) const
{
	auto noLimit = Selector::effectRange()(Bonus::NO_LIMIT);
	auto limitMatches = info.shooting
						? Selector::effectRange()(Bonus::ONLY_DISTANCE_FIGHT)
						: Selector::effectRange()(Bonus::ONLY_MELEE_FIGHT);

	//any regular bonuses or just ones for melee/ranged
	return bearer->getBonuses(selector, noLimit.Or(limitMatches))->totalValue();
};

TDmgRange DamageCalculator::calculateDmgRange() const
{
	TDmgRange result = getBaseDamageStack();

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
		defenseFactorTotal *= ( 1 - std::min(1.0, factor));
	}

	double resultingFactor = std::min(8.0, attackFactorTotal) * std::max( 0.01, defenseFactorTotal);

	return {
		std::max( 1.0, std::floor(result.first * resultingFactor)),
		std::max( 1.0, std::floor(result.second * resultingFactor))
	};
}
