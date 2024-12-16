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
#include "../mapObjects/CGTownInstance.h"
#include "../spells/CSpellHandler.h"
#include "../IGameSettings.h"
#include "../VCMI_Lib.h"

#include <lua.hpp>

class LuaExpressionEvaluator : boost::noncopyable
{
	lua_State * luaState = nullptr;
	const std::string expression;
	int compiledReference = -1;

	void compile();
	void registerLibrary();
public:
	LuaExpressionEvaluator(const std::string & expression);
	~LuaExpressionEvaluator();

	void setVariable(const std::string & name, double value);
	double evaluate();
};

void LuaExpressionEvaluator::registerLibrary()
{
	const auto & luaMax = [](lua_State * state)
	{
		lua_Number a = luaL_checknumber(state, 1);
		lua_Number b = luaL_checknumber(state, 2);
		lua_pushnumber(state, std::max(a,b));
		return 1;
	};

	const auto & luaMin = [](lua_State * state)
	{
		lua_Number a = luaL_checknumber(state, 1);
		lua_Number b = luaL_checknumber(state, 2);
		lua_pushnumber(state, std::min(a,b));
		return 1;
	};

	const auto & luaClamp = [](lua_State * state)
	{
		lua_Number a = luaL_checknumber(state, 1);
		lua_Number b = luaL_checknumber(state, 2);
		lua_Number c = luaL_checknumber(state, 3);
		lua_pushnumber(state, std::clamp(a,b,c));
		return 1;
	};

	const luaL_Reg registry[] = {
		{ "max", luaMax },
		{ "min", luaMin },
		{ "clamp", luaClamp },
		{ nullptr,	nullptr }
	};

	lua_pushvalue(luaState, LUA_GLOBALSINDEX);
	luaL_setfuncs(luaState,registry,0);
}

LuaExpressionEvaluator::LuaExpressionEvaluator(const std::string & expression)
	: expression(expression)
{
	luaState = luaL_newstate();
	registerLibrary();
	compile();
}

LuaExpressionEvaluator::~LuaExpressionEvaluator()
{
	lua_close(luaState);
}

void LuaExpressionEvaluator::setVariable(const std::string & name, double value)
{
	lua_pushnumber(luaState, value);
	lua_setglobal(luaState, name.c_str());
}

void LuaExpressionEvaluator::compile()
{
	int result = luaL_loadstring(luaState, expression.c_str());

	if (result)
	{
		logGlobal->error("Lua compilation failure: %s", lua_tostring(luaState,-1));
		lua_pop(luaState,1);
	}

	assert(result == LUA_OK);
	compiledReference = luaL_ref(luaState, LUA_REGISTRYINDEX);
}

double LuaExpressionEvaluator::evaluate()
{
	lua_rawgeti(luaState, LUA_REGISTRYINDEX, compiledReference);
	int errorCode = lua_pcall(luaState,0,1,0);
	if (errorCode)
	{
		logGlobal->error("Lua evaluation failure: %s", lua_tostring(luaState,-1));
		lua_pop(luaState,1);
		return 0;
	}

	double result = lua_tonumber(luaState,-1);
	lua_pop(luaState,1);
	return result;
}

VCMI_LIB_NAMESPACE_BEGIN

DamageRange DamageCalculator::getBaseDamageSingle() const
{
	int64_t minDmg = 0.0;
	int64_t maxDmg = 0.0;

	minDmg = info.attacker->getMinDamage(info.shooting);
	maxDmg = info.attacker->getMaxDamage(info.shooting);

    if(minDmg > maxDmg)
    {
	const auto & creatureName = info.attacker->creatureId().toEntity(VLC)->getNamePluralTranslated();
	logGlobal->error("Creature %s: min damage %lld exceeds max damage %lld.", creatureName, minDmg, maxDmg);
        logGlobal->error("This may lead to unexpected results, please report it to the mod's creator.");
        // to avoid an RNG crash and make bless and curse spells work as expected
        std::swap(minDmg, maxDmg);
    }

	if(info.attacker->creatureIndex() == CreatureID::ARROW_TOWERS)
	{
		const auto * town = callback.battleGetDefendedTown();
		assert(town);

		switch(info.attacker->getPosition())
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
		auto retrieveHeroPrimSkill = [&](PrimarySkill skill) -> int
		{
			std::shared_ptr<const Bonus> b = info.attacker->getBonus(Selector::sourceTypeSel(BonusSource::HERO_BASE_SKILL).And(Selector::typeSubtype(BonusType::PRIMARY_SKILL, BonusSubtypeID(skill))));
			return b ? b->val : 0;
		};

		//minDmg and maxDmg are multiplied by hero attack + 1
		minDmg *= retrieveHeroPrimSkill(PrimarySkill::ATTACK) + 1;
		maxDmg *= retrieveHeroPrimSkill(PrimarySkill::ATTACK) + 1;
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
	auto slayerAffected = info.defender->unitType()->valOfBonuses(Selector::type()(BonusType::KING));

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
	static LuaExpressionEvaluator evaluator("return clamp((attack - defense) * 0.05, 0, 4)");

	evaluator.setVariable("defense", getTargetDefenseEffective());
	evaluator.setVariable("attack", getActorAttackEffective());
	double result = evaluator.evaluate();
	return result;
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
	const std::string cachingStrJousting = "type_JOUSTING";
	static const auto selectorJousting = Selector::type()(BonusType::JOUSTING);

	const std::string cachingStrChargeImmunity = "type_CHARGE_IMMUNITY";
	static const auto selectorChargeImmunity = Selector::type()(BonusType::CHARGE_IMMUNITY);

	//applying jousting bonus
	if(info.chargeDistance > 0 && info.attacker->hasBonus(selectorJousting, cachingStrJousting) && !info.defender->hasBonus(selectorChargeImmunity, cachingStrChargeImmunity))
		return info.chargeDistance * (info.attacker->valOfBonuses(selectorJousting))/100.0;
	return 0.0;
}

double DamageCalculator::getAttackHateFactor() const
{
	//assume that unit have only few HATE features and cache them all
	const std::string cachingStrHate = "type_HATE";
	static const auto selectorHate = Selector::type()(BonusType::HATE);

	auto allHateEffects = info.attacker->getBonuses(selectorHate, cachingStrHate);

	return allHateEffects->valOfBonuses(Selector::subtype()(BonusSubtypeID(info.defender->creatureId()))) / 100.0;
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
	static LuaExpressionEvaluator evaluator("return clamp((defense - attack) * 0.025, 0, 0.7)");

	evaluator.setVariable("defense", getTargetDefenseEffective());
	evaluator.setVariable("attack", getActorAttackEffective());
	double result = evaluator.evaluate();
	return result;
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
		TConstBonusListPtr forgetfulList = info.attacker->getBonuses(Selector::type()(BonusType::FORGETFULL),"type_FORGETFULL");

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
		getAttackDeathBlowFactor(),
		getAttackDoubleDamageFactor(),
		getAttackHateFactor(),
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
	return bearer->getBonuses(selector, noLimit.Or(limitMatches))->totalValue();
};

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

	DamageRange damageDealt {
		std::max<int64_t>( 1.0, std::floor(damageBase.min * resultingFactor)),
		std::max<int64_t>( 1.0, std::floor(damageBase.max * resultingFactor))
	};

	DamageRange killsDealt = getCasualties(damageDealt);

	return DamageEstimation{damageDealt, killsDealt};
}

VCMI_LIB_NAMESPACE_END
