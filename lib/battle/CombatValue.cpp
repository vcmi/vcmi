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

/// Share of a battle that a bonus is expected to last. What a creature carries permanently lasts all
/// of it; a spell is worth only for as long as its effect stays on.
static double durationWeight(const Bonus & bonus)
{
	// an effect that ends on the next blow struck or taken is worth about one round
	static constexpr double oneAction = 1.0 / battleRounds;

	static constexpr BonusDuration::Type endsOnAction =
		BonusDuration::UNTIL_ATTACK | BonusDuration::UNTIL_OWN_ATTACK
		| BonusDuration::UNTIL_BEING_ATTACKED | BonusDuration::UNTIL_AFTER_ATTACK_SEQUENCE
		| BonusDuration::UNTIL_TAKING_INDIRECT_DAMAGE | BonusDuration::STACK_GETS_TURN;

	// several ends can be declared at once, and the effect stops at whichever of them comes first
	if(bonus.duration & endsOnAction)
		return oneAction;

	if(bonus.duration & BonusDuration::N_TURNS)
		return std::clamp(bonus.turnsRemain / static_cast<double>(battleRounds), oneAction, 1.0);

	return 1.0;
}

/// Total magnitude of the bonuses of given type, each counted only for as long as it lasts
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

/// Longest share of a battle that a bonus of given type is present for
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
	double attackBonus = bonuses->valOfBonuses(BonusType::ENEMY_DEFENCE_REDUCTION) / 100.0 * averageDefense;

	// frenzy buys attack with the defense of its bearer, which it gives up entirely
	attackBonus += lastingValue(bonuses, BonusType::IN_FRENZY) / 100.0 * creature.getDefense(false);

	// bless and curse do not scale damage, they collapse its range onto one of its ends
	const double damageShift = lastingValue(bonuses, BonusType::ALWAYS_MAXIMUM_DAMAGE)
		- lastingValue(bonuses, BonusType::ALWAYS_MINIMUM_DAMAGE);
	const double blessed = lastingPresence(bonuses, BonusType::ALWAYS_MAXIMUM_DAMAGE);
	const double cursed = lastingPresence(bonuses, BonusType::ALWAYS_MINIMUM_DAMAGE);

	const auto blow = [&](bool shooting)
	{
		const double low = std::max(1.0, creature.getMinDamage(shooting) + damageShift);
		const double high = std::max(1.0, creature.getMaxDamage(shooting) + damageShift);
		const double mean = (low + high) / 2.0;

		// the range only stays collapsed while the spell holds, and two opposed spells cancel out
		const double damage = mean + (high - mean) * blessed - (mean - low) * cursed;

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

	// a frenzied unit has traded its defense away for attack, for as long as the frenzy holds
	const double frenzied = lastingPresence(bonuses, BonusType::IN_FRENZY);

	return static_cast<int>(std::lround((creature.getDefense(false) + stance) * (1.0 - frenzied)));
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

	// blindness and paralysis leave their bearer striking feebly
	result *= std::max(0.0, 1.0 - lastingValue(unit, BonusType::GENERAL_ATTACK_REDUCTION) / 100.0);

	if(unit->hasBonusOfType(BonusType::SHOOTER))
	{
		// a shooter that can not shoot finishes the battle in melee, at this share of its value
		static constexpr double meleeFallback = 0.4;

		if(!unit->hasBonusOfType(BonusType::NO_DISTANCE_PENALTY))
			result *= shootingRangeFactor;

		// a shooter out of ammunition is reduced to melee for the rest of the battle
		const int shots = unit->valOfBonuses(BonusType::SHOTS);
		if(shots > 0 && shots < battleRounds)
			result *= (shots + (battleRounds - shots) * meleeFallback) / battleRounds;

		// forgetfulness spoils shooting alone, and at full strength forbids it outright
		const double forgetful = std::min(100.0, lastingValue(unit, BonusType::FORGETFULL));
		result *= forgetful < 100 ? 1.0 - forgetful / 100.0 : meleeFallback;
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

double CombatValue::situationalSurvival(const ACreature & creature) const
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

	// shield and air shield each turn aside one kind of blow, so what they are worth depends on how
	// the enemy fights - known here only as the share of creatures that close in rather than shoot
	const auto reductionOf = [unit](const BonusSubtypeID & subtype)
	{
		return lastingValue(unit, BonusType::GENERAL_DAMAGE_REDUCTION, subtype);
	};

	const double reduced = reductionOf(BonusCustomSubtype::damageTypeAll)
		+ reductionOf(BonusCustomSubtype::damageTypeMelee) * meleeAttackerShare
		+ reductionOf(BonusCustomSubtype::damageTypeRanged) * (1.0 - meleeAttackerShare);

	result /= std::max(0.1, 1.0 - reduced / 100.0);

	return result;
}

double CombatValue::situationalOffense(const ACreature & creature)
{
	const auto * unit = creature.getBonusBearer();
	double result = 1.0;

	// being unable to act is crippling, but often ends the moment the unit is struck
	result *= 1.0 - 0.5 * lastingPresence(unit, BonusType::NOT_ACTIVE);

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
