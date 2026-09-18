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
#include "../modding/IdentifierStorage.h"
#include "../modding/ModScope.h"
#include "../spells/CSpellHandler.h"

/// Estimated length of an average battle, in rounds
static constexpr int battleRounds = 8;

/// Highest attack or defense that the curves are built for
static constexpr int skillCap = 120;

/// Level of the spell that a bonus names, with schoolless abilities read as mid-level
static int spellLevelOf(const BonusSubtypeID & subtype)
{
	// H3 creature abilities belong to no school, and a missing school does not make them weak
	static constexpr int schoollessSpellLevel = 2;

	const auto spell = subtype.as<SpellID>();

	if(!spell.hasValue())
		return schoollessSpellLevel;

	const int level = spell.toSpell()->getLevel();

	return level > 0 ? level : schoollessSpellLevel;
}

/// What one point of a combat script's magnitude adds to the value of its bearer, or zero for a
/// script whose effect the magnitude does not describe.
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
		creatureValues[creature->getIndex()] = getAIValue(*creature, creature.get());
}

void CombatValue::buildCurves(const std::vector<const CCreature *> & builtinCreatures)
{
	const auto * settings = LIBRARY->engineSettings();
	const double attackPerPoint = settings->getDouble(EGameSettings::COMBAT_ATTACK_POINT_DAMAGE_FACTOR);
	const double attackCap = settings->getDouble(EGameSettings::COMBAT_ATTACK_POINT_DAMAGE_FACTOR_CAP);
	const double defensePerPoint = settings->getDouble(EGameSettings::COMBAT_DEFENSE_POINT_DAMAGE_FACTOR);
	const double defenseCap = settings->getDouble(EGameSettings::COMBAT_DEFENSE_POINT_DAMAGE_FACTOR_CAP);

	// share of its damage that an attack of given skill deals to a creature of given defense
	const auto damageFactor = [=](int attack, int defense)
	{
		if(attack >= defense)
			return 1.0 + std::min((attack - defense) * attackPerPoint, attackCap);
		return 1.0 - std::min((defense - attack) * defensePerPoint, defenseCap);
	};

	int shooters = 0;

	for(const auto * creature : builtinCreatures)
	{
		averageDefense += creature->getBaseDefense();
		if(creature->hasBonusOfType(BonusType::SHOOTER))
			++shooters;
	}

	averageDefense /= builtinCreatures.size();
	meleeAttackerShare = 1.0 - static_cast<double>(shooters) / builtinCreatures.size();

	offenseCurve.assign(skillCap + 1, 0.0);
	for(int attack = 0; attack <= skillCap; ++attack)
	{
		for(const auto * creature : builtinCreatures)
			offenseCurve[attack] += damageFactor(attack, creature->getBaseDefense());
		offenseCurve[attack] /= builtinCreatures.size();
	}

	// measured against each attacker's own average, so the two curves stay independent of each other
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
	// the model has a scale of its own, so it is pinned to H3 values
	std::vector<double> scales;
	std::vector<double> fightScales;

	for(const auto * creature : builtinCreatures)
	{
		const double uptime = uptimeOf(*creature);
		const double value = valueOf(*creature, uptime, referenceCount(creature));

		if(value <= 0)
			continue;

		if(creature->getAIValue() > 0)
			scales.push_back(creature->getAIValue() / value);
		if(creature->getFightValue() > 0)
			fightScales.push_back(creature->getFightValue() / (value / uptime));
	}

	if(scales.empty() || fightScales.empty())
		return;

	scale = median(scales);
	fightScale = median(fightScales);
}

double CombatValue::valueOf(const ACreature & creature, double uptime, int count) const
{
	static constexpr double meleePenalty = 0.5;

	const auto * bonuses = creature.getBonusBearer();
	const bool ranged = bonuses->hasBonusOfType(BonusType::SHOOTER);

	// curves are indexed by skill, so defense reduction is priced as extra attack instead of as damage
	const double attackBonus = bonuses->valOfBonuses(BonusType::ENEMY_DEFENCE_REDUCTION) / 100.0 * averageDefense;

	const auto blow = [&](bool shooting)
	{
		const double damage = (creature.getMinDamage(shooting) + creature.getMaxDamage(shooting)) / 2.0;
		return damage * offenseAt(static_cast<int>(std::lround(creature.getAttack(shooting) + attackBonus)));
	};

	double ownBlow = blow(ranged) * attacksPerRound(creature) * targetsPerAttack(creature);

	// jousting scales with distance covered, which is at most one turn of movement
	const int charge = std::min<int>(startingDistance(), creature.getMovementRange());
	ownBlow *= 1.0 + bonuses->valOfBonuses(BonusType::JOUSTING) / 100.0 * charge;

	const bool retaliatesAtRange = ranged && bonuses->hasBonusOfType(BonusType::RANGED_RETALIATION);
	const double retaliatedAgainst = bonuses->hasBonusOfType(BonusType::RANGED_RETALIATION) ? 1.0 : meleeAttackerShare;

	double retaliationBlow = blow(retaliatesAtRange) * retaliationsPerRound(creature) * retaliatedAgainst;
	if(ranged && !retaliatesAtRange && !bonuses->hasBonusOfType(BonusType::NO_MELEE_PENALTY))
		retaliationBlow *= meleePenalty;

	const double output = (ownBlow + retaliationBlow) * offenseMultiplier(creature) * situationalOffense(creature);
	const double defense = defenseAt(effectiveDefense(creature)) * (1.0 + retaliationSuffered(creature));
	const double hitPoints = creature.getMaxHealth() + regeneratedHitPoints(creature, count);
	const double effectiveHitPoints = hitPoints / defense * survivalMultiplier(creature) * situationalSurvival(creature);

	return combine(output, effectiveHitPoints, uptime);
}

double CombatValue::attacksPerRound(const ACreature & creature)
{
	// a second strike is worth less than the first, which may have already destroyed the target
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
	// extra units a multi-target attack reaches, given how rarely the needed hexes are occupied
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

	// share of melee attacks that reach a shooter standing behind its own line
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
	// defending is never forced, so only a share of turns is spent on it
	static constexpr double defendingShare = 0.25;

	const auto * bonuses = creature.getBonusBearer();
	const double stance = bonuses->valOfBonuses(BonusType::DEFENSIVE_STANCE) * defendingShare;

	return static_cast<int>(std::lround(creature.getDefense(false) + stance));
}

double CombatValue::retaliationSuffered(const ACreature & creature)
{
	// damage taken back over a round, as a share of what enemies deal on their own turn
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
	// flying crosses the field unobstructed, and a shooter fights from the first round
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
	// value of one spell level, cast in place of an attack or for free after a blow that lands
	static constexpr double castWeight = 0.045;
	static constexpr double spellAfterAttackWeight = 0.09;

	// a second spell cast with the same blow lands on an already crippled target
	static constexpr double secondSpellWeight = 0.5;

	// most shots are taken after the enemy has closed in, so the distance penalty rarely applies
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
	const auto castings = unit->getBonuses(Selector::type()(BonusType::SPELLCASTER));
	for(const auto & bonus : *castings)
		bestSpellLevel = std::max(bestSpellLevel, spellLevelOf(bonus->subtype));

	result *= 1.0 + castWeight * bestSpellLevel;

	// enchanters cast on top of their own attack instead of in place of it
	if(unit->hasBonusOfType(BonusType::ENCHANTER))
		result *= 1.18;

	// a spell outlasts the blow that applied it, so its value grows slower than its chance to land
	double best = 0;
	double rest = 0;
	const auto attackSpells = unit->getBonuses(Selector::type()(BonusType::SPELL_AFTER_ATTACK)
		.Or(Selector::type()(BonusType::SPELL_BEFORE_ATTACK)));
	for(const auto & bonus : *attackSpells)
	{
		const double landed = std::sqrt(std::clamp(bonus->val, 0, 100) / 100.0) * spellLevelOf(bonus->subtype);

		rest += std::min(best, landed);
		best = std::max(best, landed);
	}

	result *= 1.0 + spellAfterAttackWeight * (best + secondSpellWeight * rest);
	if(unit->hasBonusOfType(BonusType::SPELL_LIKE_ATTACK))
		result *= 1.95;
	// healing is counted as damage output, since it removes damage dealt by enemy
	if(unit->hasBonusOfType(BonusType::HEALER))
		result *= 1.10;

	// a script whose magnitude does not describe its effect can only be guessed at
	static constexpr double unpricedScriptValue = 0.10;

	for(const auto & bonus : *unit->getBonusesOfType(BonusType::COMBAT_EVENT_TRIGGER))
	{
		const double weight = combatScriptWeight(bonus->subtype);

		result *= 1.0 + (weight > 0 ? weight * bonus->val : unpricedScriptValue);
	}

	if(unit->hasBonusOfType(BonusType::POISON))
		result *= 1.08;

	if(unit->hasBonusOfType(BonusType::SHOOTER))
	{
		if(!unit->hasBonusOfType(BonusType::NO_DISTANCE_PENALTY))
			result *= shootingRangeFactor;

		// a shooter out of ammunition finishes the battle in melee, at an estimated 40% of its value
		const int shots = unit->valOfBonuses(BonusType::SHOTS);
		if(shots > 0 && shots < battleRounds)
			result *= (shots + (battleRounds - shots) * 0.4) / battleRounds;
	}

	if(unit->hasBonusOfType(BonusType::HYPNOTIZED))
		result *= 0.5;

	// what the source of fear gains is not priced - a propagated bonus can not be told apart from
	// the bonus of its propagator
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

double CombatValue::situationalSurvival(const ACreature & creature)
{
	const auto * unit = creature.getBonusBearer();
	double result = 1.0;

	// immunity and vulnerability are granted one spell at a time, so bonus count is what matters
	const auto bonusCount = [unit](BonusType type)
	{
		return static_cast<int>(unit->getBonuses(Selector::type()(type))->size());
	};

	result *= std::pow(1.015, bonusCount(BonusType::SPELL_IMMUNITY));
	result *= std::pow(0.97, bonusCount(BonusType::MORE_DAMAGE_FROM_SPELL));
	result *= 1.0 + unit->valOfBonuses(BonusType::LEVEL_SPELL_IMMUNITY) * 0.02;

	if(unit->hasBonusOfType(BonusType::SPELL_SCHOOL_IMMUNITY))
		result *= 1.04;

	result *= 1.0 + unit->valOfBonuses(BonusType::MAGIC_RESISTANCE) / 100.0 * 0.5;
	result *= 1.0 + unit->valOfBonuses(BonusType::SPELL_DAMAGE_REDUCTION) / 100.0 * 0.2;

	if(unit->hasBonusOfType(BonusType::MAGIC_MIRROR))
		result *= 1.06;

	return result;
}

double CombatValue::situationalOffense(const ACreature & creature)
{
	const auto * unit = creature.getBonusBearer();
	double result = 1.0;

	if(unit->hasBonusOfType(BonusType::NOT_ACTIVE))
		result *= 0.5;

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
	return std::llround(valueOf(bearer, uptimeOf(bearer), referenceCount(type)) * scale);
}

int64_t CombatValue::getAIValue(const Creature * creature) const
{
	// the table is only filled once the model is ready, so early callers are answered directly
	if(creature->getIndex() < creatureValues.size())
		return creatureValues[creature->getIndex()];

	return getAIValue(*creature, creature);
}

int64_t CombatValue::getFightValue(const Creature * creature) const
{
	const double uptime = uptimeOf(*creature);

	return std::llround(valueOf(*creature, uptime, referenceCount(creature)) / uptime * fightScale);
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

	return std::llround(valueOf(*unit, uptimeOf(*unit, hexesToEnemy), referenceCount(unit->unitType())) * scale);
}
