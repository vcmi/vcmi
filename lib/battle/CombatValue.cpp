/*
 * CombatValue.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CombatValue.h"

#include "CBattleInfoCallback.h"
#include "Unit.h"

#include "../CCreatureHandler.h"
#include "../GameLibrary.h"
#include "../IGameSettings.h"
#include "../bonuses/Bonus.h"
#include "../bonuses/BonusParameters.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../modding/IdentifierStorage.h"
#include "../modding/ModScope.h"
#include "../spells/CSpellHandler.h"

/// Estimated length of an average battle, in rounds
static constexpr int battleRounds = 8;

/// Magic strength of a hero with a full spellbook and a few points of knowledge and spell power
static constexpr double referenceMagicStrength = 1.25;

/// Upper limit on the value of a hero as a caster, however high its magic strength is
static constexpr double magicCap = 2.0;

/// Value of an army consisting entirely of hostile casters, relative to such a hero
static constexpr double creatureMagicWeight = 0.5;

/// Whether spells of this creature can target enemy units
static bool castsAtEnemies(const IBonusBearer * unit)
{
	// spells attached to an attack always target an enemy
	if(unit->hasBonusOfType(BonusType::SPELL_AFTER_ATTACK)
		|| unit->hasBonusOfType(BonusType::SPELL_BEFORE_ATTACK)
		|| unit->hasBonusOfType(BonusType::SPELL_LIKE_ATTACK))
		return true;

	// most creature casters only buff their own side, so enemy magic defenses do not apply
	for(const auto & bonus : *unit->getBonusesOfType(BonusType::SPELLCASTER))
	{
		const auto spell = bonus->subtype.as<SpellID>();

		if(spell.hasValue() && spell.toSpell()->isNegative())
			return true;
	}

	return false;
}

static double magicOf(const CGHeroInstance * hero)
{
	if(!hero)
		return 0;

	// saturating curve: each reference hero of magic strength halves the remaining headroom, giving 1.0 for the first
	const double strength = std::max(0.0, hero->getMagicStrength() - 1.0);

	return magicCap * (1.0 - std::exp2(-strength / (referenceMagicStrength - 1.0)));
}

int32_t CombatValueContext::id() const
{
	size_t hash = 0;

	vstd::hash_combine(hash, meleeShare);
	vstd::hash_combine(hash, magicPower);
	vstd::hash_combine(hash, allyCrowding);

	for(double share : kingShare)
		vstd::hash_combine(hash, share);

	return static_cast<int32_t>(hash);
}

/// Adds value of a creature to every slayer mastery level that can target it
static void countKing(const IBonusBearer * creature, double value, CombatValueContext::MasteryShares & kings)
{
	if(!creature->hasBonusOfType(BonusType::KING))
		return;

	const auto reachedBy = std::clamp<size_t>(creature->valOfBonuses(BonusType::KING), 0, kings.size() - 1);

	for(size_t mastery = reachedBy; mastery < kings.size(); ++mastery)
		kings[mastery] += value;
}

CombatValueContext CombatValueContext::against(const CBattleInfoCallback & battle, BattleSide side)
{
	double total = 0;
	double melee = 0;
	double casters = 0;
	size_t allies = 0;
	MasteryShares kings = {};

	for(const auto * unit : battle.battleGetUnitsIf([](const battle::Unit * candidate)
		{ return candidate->alive(); }))
	{
		// berserk is evaluated against own army, so count allies of the side this context describes
		if(unit->unitSide() == side)
		{
			++allies;
			continue;
		}

		const double value = unit->estimateCombatValue();
		const auto * bearer = unit->getBonusBearer();

		total += value;

		// blocked shooter, or one without ammo, attacks in melee like any other creature
		if(!battle.battleCanShoot(unit))
			melee += value;

		if(castsAtEnemies(bearer))
			casters += value;

		countKing(bearer, value, kings);
	}

	const double crowding = allies > 0 ? 1.0 - 1.0 / allies : 0.0;

	// no enemies left to describe, but the number of allies is still valid
	if(total <= 0)
	{
		CombatValueContext result = LIBRARY->combatValues->averageBattle();

		result.allyCrowding = crowding;

		return result;
	}

	CombatValueContext result;
	result.meleeShare = melee / total;
	result.magicPower = magicOf(battle.battleGetFightingHero(CBattleInfoEssentials::otherSide(side)))
		+ creatureMagicWeight * casters / total;

	for(size_t mastery = 0; mastery < kings.size(); ++mastery)
		result.kingShare[mastery] = kings[mastery] / total;

	result.allyCrowding = crowding;

	return result;
}

/// Highest attack or defense that the curves are built for
static constexpr int skillCap = 120;

/// Level of the spell referenced by a bonus, with schoolless abilities treated as mid-level
static int spellLevelOf(const BonusSubtypeID & subtype)
{
	// H3 creature abilities belong to no school, which does not make them weak
	static constexpr int schoollessSpellLevel = 2;

	const auto spell = subtype.as<SpellID>();

	if(!spell.hasValue())
		return schoollessSpellLevel;

	const int level = spell.toSpell()->getLevel();

	return level > 0 ? level : schoollessSpellLevel;
}

/// Value added by one point of combat script magnitude, or zero if magnitude does not describe the effect
static double combatScriptWeight(const BonusSubtypeID & subtype)
{
	static constexpr std::array<std::pair<std::string_view, double>, 3> weights = {{
		{ "deathStare", 0.02 },
		{ "lifeDrain", 0.003 },
		{ "fireShield", 0.003 },
	}};

	static const std::map<si32, double> priced = []
	{
		std::map<si32, double> result;

		for(const auto & [name, weight] : weights)
		{
			auto index = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "script", std::string(name), false);

			if(index.has_value())
				result[*index] = weight;
		}

		return result;
	}();

	auto entry = priced.find(subtype.getNum());

	return entry == priced.end() ? 0.0 : entry->second;
}

/// Effect that ends on the next attack made or received lasts about one round
static constexpr double oneAction = 1.0 / battleRounds;

/// Fraction of a battle that a bonus lasts, from its remaining turn count alone
static double turnsWeight(const Bonus & bonus)
{
	if(bonus.duration & BonusDuration::N_TURNS)
		return std::clamp(bonus.turnsRemain / static_cast<double>(battleRounds), oneAction, 1.0);

	return 1.0;
}

/// Fraction of a battle that a bonus lasts: all of it for permanent bonuses, less for timed ones
static double durationWeight(const Bonus & bonus)
{
	static constexpr BonusDuration::Type endsOnAction =
		BonusDuration::UNTIL_ATTACK | BonusDuration::UNTIL_OWN_ATTACK
		| BonusDuration::UNTIL_BEING_ATTACKED | BonusDuration::UNTIL_AFTER_ATTACK_SEQUENCE
		| BonusDuration::UNTIL_TAKING_INDIRECT_DAMAGE | BonusDuration::STACK_GETS_TURN;

	// duration may combine several end conditions, effect ends on the earliest of them
	if(bonus.duration & endsOnAction)
		return oneAction;

	return turnsWeight(bonus);
}

/// Total magnitude of bonuses of given type, each scaled by the fraction of battle it lasts
static double lastingValue(const IBonusBearer * unit, BonusType type)
{
	double total = 0;

	for(const auto & bonus : *unit->getBonusesOfType(type))
		total += bonus->val * durationWeight(*bonus);

	return total;
}

static double lastingValue(const IBonusBearer * unit, BonusType type, const BonusSubtypeID & subtype)
{
	double total = 0;

	for(const auto & bonus : *unit->getBonusesOfType(type, subtype))
		total += bonus->val * durationWeight(*bonus);

	return total;
}

/// Largest fraction of a battle for which a bonus of given type is active
static double lastingPresence(const IBonusBearer * unit, BonusType type)
{
	double longest = 0;

	for(const auto & bonus : *unit->getBonusesOfType(type))
		longest = std::max(longest, durationWeight(*bonus));

	return longest;
}

double CombatValue::offenseAt(int attack) const
{
	return offenseCurve[std::clamp(attack, 0, skillCap)];
}

double CombatValue::defenseAt(int defense) const
{
	return defenseCurve[std::clamp(defense, 0, skillCap)];
}

double CombatValue::median(std::vector<double> values)
{
	if(values.empty())
		return 0;

	std::ranges::sort(values);

	return values[values.size() / 2];
}

CombatValue::CombatValue()
{
	std::vector<const CCreature *> builtinCreatures;

	for(const auto & creature : LIBRARY->creh->objects)
		if(!creature->special && creature->getModScope() == ModScope::scopeBuiltin())
			builtinCreatures.push_back(creature.get());

	buildCurves(builtinCreatures);
	pinScale(builtinCreatures);
	tabulateCreatures();
}

void CombatValue::tabulateCreatures()
{
	creatureValues.resize(LIBRARY->creh->objects.size());

	for(const auto & creature : LIBRARY->creh->objects)
		creatureValues.at(creature->getIndex()) = getAIValue(*creature, creature.get());
}

void CombatValue::buildCurves(const std::vector<const CCreature *> & builtinCreatures)
{
	const auto * settings = LIBRARY->engineSettings();
	const double attackPerPoint = settings->getDouble(EGameSettings::COMBAT_ATTACK_POINT_DAMAGE_FACTOR);
	const double attackCap = settings->getDouble(EGameSettings::COMBAT_ATTACK_POINT_DAMAGE_FACTOR_CAP);
	const double defensePerPoint = settings->getDouble(EGameSettings::COMBAT_DEFENSE_POINT_DAMAGE_FACTOR);
	const double defenseCap = settings->getDouble(EGameSettings::COMBAT_DEFENSE_POINT_DAMAGE_FACTOR_CAP);

	// fraction of its damage that an attack of given skill deals to a creature of given defense
	const auto damageFactor = [=](int attack, int defense)
	{
		if(attack >= defense)
			return 1.0 + std::min((attack - defense) * attackPerPoint, attackCap);
		return 1.0 - std::min((defense - attack) * defensePerPoint, defenseCap);
	};

	int shooters = 0;
	CombatValueContext::MasteryShares kings = {};

	for(const auto * creature : builtinCreatures)
	{
		averageDefense += creature->getBaseDefense();
		if(creature->hasBonusOfType(BonusType::SHOOTER))
			++shooters;

		countKing(creature, 1.0, kings);
	}

	averageDefense /= builtinCreatures.size();
	defaultContext.meleeShare = 1.0 - static_cast<double>(shooters) / builtinCreatures.size();

	for(size_t mastery = 0; mastery < kings.size(); ++mastery)
		defaultContext.kingShare[mastery] = kings[mastery] / builtinCreatures.size();

	// average battle is assumed to have every slot filled with equally strong stacks
	defaultContext.allyCrowding = 1.0 - 1.0 / GameConstants::ARMY_SIZE;

	offenseCurve.assign(skillCap + 1, 0.0);
	for(int attack = 0; attack <= skillCap; ++attack)
	{
		for(const auto * creature : builtinCreatures)
			offenseCurve[attack] += damageFactor(attack, creature->getBaseDefense());
		offenseCurve[attack] /= builtinCreatures.size();
	}

	// normalized by the average of each attacker, so that the two curves stay independent of each other
	defenseCurve.assign(skillCap + 1, 0.0);
	for(int defense = 0; defense <= skillCap; ++defense)
	{
		for(const auto * creature : builtinCreatures)
		{
			const int attack = creature->getBaseAttack();
			defenseCurve[defense] += damageFactor(attack, defense) / offenseAt(attack);
		}
		defenseCurve[defense] /= builtinCreatures.size();
	}
}

void CombatValue::pinScale(const std::vector<const CCreature *> & builtinCreatures)
{
	// computed values use an arbitrary scale, so they are normalized to H3 aiValue
	std::vector<double> scales;

	for(const auto * creature : builtinCreatures)
	{
		const double value = valueOf(*creature, uptimeOf(*creature), referenceCount(creature), defaultContext);

		if(value > 0 && creature->getAIValue() > 0)
			scales.push_back(creature->getAIValue() / value);
	}

	if(!scales.empty())
		scale = median(scales);
}

double CombatValue::valueOf(const ACreature & creature, double uptime, int count, const CombatValueContext & context) const
{
	static constexpr double meleePenalty = 0.5;

	const auto * bonuses = creature.getBonusBearer();
	const bool ranged = bonuses->hasBonusOfType(BonusType::SHOOTER);

	// curves are indexed by skill, so defense reduction is applied as extra attack instead of as damage
	double attackBonus = bonuses->valOfBonuses(BonusType::ENEMY_DEFENCE_REDUCTION) / 100.0 * averageDefense;

	// frenzy converts defense of its bearer into attack, removing its defense entirely
	attackBonus += lastingValue(bonuses, BonusType::IN_FRENZY) / 100.0 * creature.getDefense(false);

	// slayer adds attack only against KING units that its mastery level can target
	for(const auto & bonus : *bonuses->getBonusesOfType(BonusType::SLAYER))
	{
		const size_t mastery = bonus->parameters
			? std::clamp<size_t>(bonus->parameters->toNumber(), 0, context.kingShare.size() - 1)
			: 0;

		attackBonus += bonus->val * durationWeight(*bonus) * context.kingShare[mastery];
	}

	// bless and curse do not scale damage, they fix it at one end of its range
	const double damageShift = lastingValue(bonuses, BonusType::ALWAYS_MAXIMUM_DAMAGE)
		- lastingValue(bonuses, BonusType::ALWAYS_MINIMUM_DAMAGE);
	const double blessed = lastingPresence(bonuses, BonusType::ALWAYS_MAXIMUM_DAMAGE);
	const double cursed = lastingPresence(bonuses, BonusType::ALWAYS_MINIMUM_DAMAGE);

	const auto strike = [&](bool shooting)
	{
		const double low = std::max(1.0, creature.getMinDamage(shooting) + damageShift);
		const double high = std::max(1.0, creature.getMaxDamage(shooting) + damageShift);
		const double mean = (low + high) / 2.0;

		// damage is only fixed while the spell lasts, and opposed spells cancel out
		const double damage = mean + (high - mean) * blessed - (mean - low) * cursed;

		return damage * offenseAt(static_cast<int>(std::lround(creature.getAttack(shooting) + attackBonus)));
	};

	double ownOutput = strike(ranged) * attacksPerRound(creature) * targetsPerAttack(creature);

	// jousting scales with distance covered, which is at most one turn of movement
	const int charge = std::min<int>(startingDistance(), creature.getMovementRange());
	ownOutput *= 1.0 + bonuses->valOfBonuses(BonusType::JOUSTING) / 100.0 * charge;

	const bool retaliatesAtRange = ranged && bonuses->hasBonusOfType(BonusType::RANGED_RETALIATION);
	const double retaliatedAgainst = bonuses->hasBonusOfType(BonusType::RANGED_RETALIATION) ? 1.0 : context.meleeShare;

	double retaliationOutput = strike(retaliatesAtRange) * retaliationsPerRound(creature) * retaliatedAgainst;
	if(ranged && !retaliatesAtRange && !bonuses->hasBonusOfType(BonusType::NO_MELEE_PENALTY))
		retaliationOutput *= meleePenalty;

	const double output = (ownOutput + retaliationOutput) * offenseMultiplier(creature) * situationalOffense(creature, context);
	const double defense = defenseAt(effectiveDefense(creature)) * (1.0 + retaliationSuffered(creature));
	const double hitPoints = creature.getMaxHealth() + regeneratedHitPoints(creature, count);
	const double effectiveHitPoints = hitPoints / defense * survivalMultiplier(creature) * situationalSurvival(creature, context);

	return combine(output, effectiveHitPoints, uptime);
}

double CombatValue::attacksPerRound(const ACreature & creature)
{
	// additional attacks count for less than the first, which may have already killed the target
	static constexpr double extraAttackWeight = 0.87;

	const auto * bonuses = creature.getBonusBearer();
	const bool ranged = bonuses->hasBonusOfType(BonusType::SHOOTER);
	const int attacks = 1 + bonuses->valOfBonuses(Selector::typeSubtype(BonusType::ADDITIONAL_ATTACK, BonusCustomSubtype::damageTypeAll))
		+ bonuses->valOfBonuses(Selector::typeSubtype(BonusType::ADDITIONAL_ATTACK,
			ranged ? BonusCustomSubtype::damageTypeRanged : BonusCustomSubtype::damageTypeMelee));

	return 1.0 + (attacks - 1) * extraAttackWeight;
}

double CombatValue::targetsPerAttack(const ACreature & creature)
{
	// extra units hit by a multi-target attack, given how rarely the required hexes are occupied
	static constexpr double breathExtraTargets = 0.45;
	static constexpr double threeHeadedExtraTargets = 0.0;
	static constexpr double allAdjacentExtraTargets = 0.4;

	const auto * bonuses = creature.getBonusBearer();
	double extra = 0.0;

	if(bonuses->hasBonusOfType(BonusType::ATTACKS_ALL_ADJACENT))
		extra += allAdjacentExtraTargets;
	else if(bonuses->hasBonusOfType(BonusType::THREE_HEADED_ATTACK))
		extra += threeHeadedExtraTargets;
	else if(bonuses->hasBonusOfType(BonusType::TWO_HEX_ATTACK_BREATH) || bonuses->hasBonusOfType(BonusType::WIDE_BREATH))
		extra += breathExtraTargets;

	if(bonuses->hasBonusOfType(BonusType::SHOOTS_ALL_ADJACENT))
		extra += allAdjacentExtraTargets;

	return 1.0 + extra;
}

double CombatValue::retaliationsPerRound(const ACreature & creature)
{
	// estimated times per round that a creature is attacked, i.e. how often it can retaliate
	static constexpr double attacksSufferedPerRound = 1.4;
	static constexpr double unlimitedRetaliationsPerRound = 2.5;

	// fraction of melee attacks that reach a shooter positioned behind its own units
	static constexpr double shooterMeleeExposure = 0.35;

	const auto * bonuses = creature.getBonusBearer();

	if(bonuses->hasBonusOfType(BonusType::NO_RETALIATION))
		return 0.0;

	if(bonuses->hasBonusOfType(BonusType::UNLIMITED_RETALIATIONS))
		return unlimitedRetaliationsPerRound;

	const double retaliations = std::min(attacksSufferedPerRound, 1.0 + bonuses->valOfBonuses(BonusType::ADDITIONAL_RETALIATION));

	return retaliations * (bonuses->hasBonusOfType(BonusType::SHOOTER) ? shooterMeleeExposure : 1.0);
}

int CombatValue::effectiveDefense(const ACreature & creature)
{
	// defending is optional, so only a fraction of turns is spent on it
	static constexpr double defendingShare = 0.25;

	const auto * bonuses = creature.getBonusBearer();

	const double stance = bonuses->valOfBonuses(BonusType::DEFENSIVE_STANCE) * defendingShare;

	// frenzy removes defense of its bearer for as long as it lasts
	const double frenzied = lastingPresence(bonuses, BonusType::IN_FRENZY);

	return static_cast<int>(std::lround((creature.getDefense(false) + stance) * (1.0 - frenzied)));
}

double CombatValue::retaliationSuffered(const ACreature & creature)
{
	// damage received from retaliation over a round, as a fraction of damage enemies deal on their turn
	static constexpr double retaliationShare = 0.6;

	const auto * bonuses = creature.getBonusBearer();
	const bool safe = bonuses->hasBonusOfType(BonusType::SHOOTER) || bonuses->hasBonusOfType(BonusType::BLOCKS_RETALIATION);

	return safe ? 0.0 : retaliationShare;
}

double CombatValue::uptimeOf(const ACreature & creature)
{
	return uptimeOf(creature, startingDistance());
}

double CombatValue::uptimeOf(const ACreature & creature, int hexesToEnemy)
{
	// flying units cross the battlefield unobstructed, and shooters attack from the first round
	static constexpr double flyingApproachBonus = 1.09;
	static constexpr double shootingUptimeBonus = 1.25;

	const auto * bonuses = creature.getBonusBearer();
	const int speed = std::max(1, static_cast<int>(creature.getMovementRange()));
	const int hexes = std::max(0, hexesToEnemy);
	const int turnsApproaching = (hexes + speed - 1) / speed - 1;
	const double uptime = std::max(0.1, static_cast<double>(battleRounds - turnsApproaching) / battleRounds);

	if(bonuses->hasBonusOfType(BonusType::SHOOTER))
		return uptime * shootingUptimeBonus;

	return uptime * (bonuses->hasBonusOfType(BonusType::FLYING) ? flyingApproachBonus : 1.0);
}

double CombatValue::offenseMultiplier(const ACreature & creature)
{
	// value of one spell level, cast in place of an attack or for free after a successful attack
	static constexpr double castWeight = 0.045;
	static constexpr double spellAfterAttackWeight = 0.09;

	// a second spell cast with the same attack hits an already weakened target
	static constexpr double secondSpellWeight = 0.5;

	// most shots are taken after the enemy has approached, so the distance penalty rarely applies
	static constexpr double shootingRangeFactor = 0.87;

	const auto * unit = creature.getBonusBearer();
	double result = 1.0;

	// morale and luck grant extra turns and extra damage - about 2% per point
	result *= 1.0 + unit->valOfBonuses(BonusType::MORALE) * 0.02;
	result *= 1.0 + unit->valOfBonuses(BonusType::LUCK) * 0.02;

	// loses the extra turns that an average creature gets from morale
	if(unit->hasBonusOfType(BonusType::NO_MORALE))
		result *= 0.96;

	result *= 1.0 + unit->valOfBonuses(BonusType::DOUBLE_DAMAGE_CHANCE) / 100.0;

	// only one spell is cast per turn, however many the creature knows
	int bestSpellLevel = 0;
	for(const auto & bonus : *unit->getBonusesOfType(BonusType::SPELLCASTER))
		bestSpellLevel = std::max(bestSpellLevel, spellLevelOf(bonus->subtype));

	result *= 1.0 + castWeight * bestSpellLevel;

	// enchanters cast in addition to their own attack instead of in place of it
	if(unit->hasBonusOfType(BonusType::ENCHANTER))
		result *= 1.18;

	// a spell outlasts the attack that applied it, so its value grows slower than its chance to apply
	double best = 0;
	double rest = 0;
	for(auto type : {BonusType::SPELL_AFTER_ATTACK, BonusType::SPELL_BEFORE_ATTACK})
	{
		for(const auto & bonus : *unit->getBonusesOfType(type))
		{
			const double landed = std::sqrt(std::clamp(bonus->val, 0, 100) / 100.0) * spellLevelOf(bonus->subtype);

			rest += std::min(best, landed);
			best = std::max(best, landed);
		}
	}

	result *= 1.0 + spellAfterAttackWeight * (best + secondSpellWeight * rest);
	if(unit->hasBonusOfType(BonusType::SPELL_LIKE_ATTACK))
		result *= 1.95;
	// healing is counted as damage output, since it removes damage dealt by enemy
	if(unit->hasBonusOfType(BonusType::HEALER))
		result *= 1.10;

	// fallback value for scripts whose magnitude does not describe their effect
	static constexpr double unpricedScriptValue = 0.10;

	for(const auto & bonus : *unit->getBonusesOfType(BonusType::COMBAT_EVENT_TRIGGER))
	{
		const double weight = combatScriptWeight(bonus->subtype);

		result *= 1.0 + (weight > 0 ? weight * bonus->val : unpricedScriptValue);
	}

	if(unit->hasBonusOfType(BonusType::POISON))
		result *= 1.08;

	// blindness and paralysis reduce attack of their bearer
	result *= std::max(0.0, 1.0 - lastingValue(unit, BonusType::GENERAL_ATTACK_REDUCTION) / 100.0);

	if(unit->hasBonusOfType(BonusType::SHOOTER))
	{
		// fraction of its value that a shooter retains when reduced to melee
		static constexpr double meleeFallback = 0.4;

		double shooting = 1.0;

		if(!unit->hasBonusOfType(BonusType::NO_DISTANCE_PENALTY))
			shooting *= shootingRangeFactor;

		// shooter fights in melee for the rest of the battle once it runs out of ammo
		const int shots = unit->valOfBonuses(BonusType::SHOTS);
		if(shots > 0 && shots < battleRounds)
			shooting *= (shots + (battleRounds - shots) * meleeFallback) / battleRounds;

		// forgetfulness affects only shooting, and blocks it completely at 100%
		shooting *= 1.0 - std::min(100.0, lastingValue(unit, BonusType::FORGETFULL)) / 100.0;

		// shooter always has the option of attacking in melee instead
		result *= std::max(meleeFallback, shooting);
	}

	if(unit->hasBonusOfType(BonusType::HYPNOTIZED))
		result *= 0.5;

	// value gained by the source of fear is not counted - a propagated bonus is indistinguishable from it
	result *= 1.0 - unit->valOfBonuses(BonusType::FEARFUL) / 100.0;

	return result;
}

double CombatValue::survivalMultiplier(const ACreature & creature)
{
	const auto * unit = creature.getBonusBearer();
	double result = 1.0;

	result *= 1.0 + unit->valOfBonuses(BonusType::REBIRTH) / 100.0;

	if(unit->hasBonusOfType(BonusType::RETURN_AFTER_STRIKE))
		result *= 1.10;

	return result;
}

double CombatValue::situationalSurvival(const ACreature & creature, const CombatValueContext & context)
{
	const auto * unit = creature.getBonusBearer();
	double result = 1.0;

	// immunity and vulnerability are granted one spell at a time, so bonus count is what matters
	const auto bonusCount = [unit](BonusType type)
	{
		return static_cast<int>(unit->getBonusesOfType(type)->size());
	};

	double magic = 1.0;

	magic *= std::pow(1.015, bonusCount(BonusType::SPELL_IMMUNITY));
	magic *= std::pow(0.97, bonusCount(BonusType::MORE_DAMAGE_FROM_SPELL));
	magic *= 1.0 + unit->valOfBonuses(BonusType::LEVEL_SPELL_IMMUNITY) * 0.02;

	if(unit->hasBonusOfType(BonusType::SPELL_SCHOOL_IMMUNITY))
		magic *= 1.04;

	magic *= 1.0 + unit->valOfBonuses(BonusType::MAGIC_RESISTANCE) / 100.0 * 0.5;
	magic *= 1.0 + unit->valOfBonuses(BonusType::SPELL_DAMAGE_REDUCTION) / 100.0 * 0.2;

	if(unit->hasBonusOfType(BonusType::MAGIC_MIRROR))
		magic *= 1.06;

	// magic defenses have no value against an enemy that casts no spells
	result *= 1.0 + (magic - 1.0) * context.magicPower;

	// shield and air shield each reduce one damage type, so their value depends on enemy composition
	const auto reductionOf = [unit](const BonusSubtypeID & subtype)
	{
		return lastingValue(unit, BonusType::GENERAL_DAMAGE_REDUCTION, subtype);
	};

	const double reduced = reductionOf(BonusCustomSubtype::damageTypeAll)
		+ reductionOf(BonusCustomSubtype::damageTypeMelee) * context.meleeShare
		+ reductionOf(BonusCustomSubtype::damageTypeRanged) * (1.0 - context.meleeShare);

	result /= std::max(0.1, 1.0 - reduced / 100.0);

	return result;
}

double CombatValue::situationalOffense(const ACreature & creature, const CombatValueContext & context)
{
	const auto * unit = creature.getBonusBearer();
	double result = 1.0;

	// attacker decides whether to break the disable, so assume it lasts its full duration
	double disabled = 0;
	for(const auto & bonus : *unit->getBonusesOfType(BonusType::NOT_ACTIVE))
		disabled = std::max(disabled, turnsWeight(*bonus));

	result *= 1.0 - 0.5 * disabled;

	// attacking own side costs twice: damage not dealt to the enemy, and damage dealt to an ally
	static constexpr double friendlyFireCost = 2.0;

	result *= std::max(0.0, 1.0 - friendlyFireCost * context.allyCrowding
		* lastingPresence(unit, BonusType::ATTACKS_NEAREST_CREATURE));

	return result;
}

double CombatValue::regeneratedHitPoints(const ACreature & creature, int count)
{
	const auto * bonuses = creature.getBonusBearer();
	const int perRound = bonuses->valOfBonuses(BonusType::HP_REGENERATION);

	if(perRound <= 0 || count <= 0)
		return 0;

	// healing on the last round of a battle changes nothing
	const int healed = std::min<int>(perRound, creature.getMaxHealth()) * (battleRounds - 1);

	return static_cast<double>(healed) / count;
}

int CombatValue::referenceCount(const Creature * creature)
{
	static constexpr int referenceWeeks = 6;

	// creature without a dwelling is only met in stacks placed on a map
	if(creature->getGrowth() <= 0)
		return std::max(1, (creature->getAdvMapAmountMin() + creature->getAdvMapAmountMax()) / 2);

	return creature->getGrowth() * referenceWeeks;
}

double CombatValue::combine(double output, double effectiveHitPoints, double uptime)
{
	if(output <= 0 || effectiveHitPoints <= 0)
		return 0;

	return std::sqrt(output * effectiveHitPoints) * uptime;
}

int CombatValue::startingDistance()
{
	static constexpr int approachDistance = 14;

	return approachDistance;
}

int64_t CombatValue::getAIValue(const ACreature & bearer, const Creature * type) const
{
	return getAIValue(bearer, type, defaultContext);
}

int64_t CombatValue::getAIValue(const ACreature & bearer, const Creature * type, const CombatValueContext & context) const
{
	return std::llround(valueOf(bearer, uptimeOf(bearer), referenceCount(type), context) * scale);
}

const CombatValueContext & CombatValue::averageBattle() const
{
	return defaultContext;
}

int64_t CombatValue::getAIValue(const Creature * creature) const
{
	return creatureValues.at(creature->getIndex());
}

int64_t CombatValue::getAIValue(const battle::Unit * unit) const
{
	return getAIValue(*unit, unit->unitType());
}

int64_t CombatValue::getAIValue(const battle::Unit * unit, const CBattleInfoCallback & battle) const
{
	int hexesToEnemy = startingDistance();

	if(unit->getPosition().isValid())
	{
		for(const auto * other : battle.battleGetUnitsIf([unit](const battle::Unit * candidate)
			{ return candidate->alive() && candidate->unitSide() != unit->unitSide() && candidate->getPosition().isValid(); }))
		{
			hexesToEnemy = std::min<int>(hexesToEnemy, BattleHex::getDistance(unit->getPosition(), other->getPosition()));
		}
	}

	return std::llround(valueOf(*unit, uptimeOf(*unit, hexesToEnemy), referenceCount(unit->unitType()), defaultContext) * scale);
}
