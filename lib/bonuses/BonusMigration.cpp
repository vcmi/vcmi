/*
 * BonusMigration.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "BonusMigration.h"

#include "Bonus.h"
#include "BonusParameters.h"
#include "../GameLibrary.h"
#include "../constants/NumericConstants.h"
#include "../json/JsonNode.h"
#include <vcmi/Creature.h>
#include <vcmi/spells/Spell.h>
#include "../modding/IdentifierStorage.h"
#include "../modding/ModScope.h"

#include <span>

namespace
{

/// Identifiers may carry the scope of the mod that defined them, which says nothing about which
/// ability this is.
std::string withoutScope(const std::string & identifier)
{
	auto separator = identifier.find(':');
	return separator == std::string::npos ? identifier : identifier.substr(separator + 1);
}

ScriptID resolveScript(const std::string & name)
{
	auto index = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "script", name, false);
	return index.has_value() ? ScriptID(*index) : ScriptID::NONE;
}

using NameMapping = std::pair<std::string_view, std::string_view>;

/// What `name` maps to, or an empty view when the table does not mention it. These tables are a
/// handful of entries each, read once per bonus at load, so a linear scan is what they are worth.
std::string_view lookup(std::span<const NameMapping> table, std::string_view name)
{
	auto entry = std::ranges::find(table, name, &NameMapping::first);

	return entry == table.end() ? std::string_view() : entry->second;
}

/// Name of the script each retired bonus type is now implemented by.
constexpr std::array<NameMapping, 8> retiredAbilities = {{
	{ "LIFE_DRAIN",       "lifeDrain" },
	{ "SOUL_STEAL",       "soulSteal" },
	{ "TRANSMUTATION",    "transmutation" },
	{ "SUMMON_GUARDIANS", "summonGuardians" },
	{ "ENCHANTED",        "enchanted" },
	{ "FIRE_SHIELD",      "fireShield" },
	{ "DESTRUCTION",      "destruction" },
	{ "DEATH_STARE",      "deathStare" },
}};

/// Situation each of the old death stare subtypes stood for.
constexpr std::array<NameMapping, 6> deathStareSituations = {{
	{ "deathStareGorgon",              "melee" },
	{ "deathStareCommander",           "commander" },
	{ "deathStareNoRangePenalty",      "ranged" },
	{ "deathStareRangePenalty",        "rangedDistancePenalty" },
	{ "deathStareObstaclePenalty",     "rangedWallPenalty" },
	{ "deathStareRangeObstaclePenalty","rangedDistanceAndWallPenalty" },
}};

std::string deathStareSituation(const std::string & subtype)
{
	std::string_view situation = lookup(deathStareSituations, subtype);

	return situation.empty() ? "melee" : std::string(situation);
}

/// Saved bonuses hold the numeric subtype the retired ability used, which no longer has a name.
std::string deathStareSituation(const BonusSubtypeID & subtype)
{
	switch(subtype.getNum())
	{
		case 1:  return "commander";
		case 2:  return "ranged";
		case 3:  return "rangedDistancePenalty";
		case 4:  return "rangedWallPenalty";
		case 5:  return "rangedDistanceAndWallPenalty";
		default: return "melee";
	}
}

/// Retired abilities that have no conversion, and what to declare instead.
constexpr std::array<NameMapping, 1> retiredWithoutMigration = {{
	{ "ACID_BREATH", "a SPELL_AFTER_ATTACK bonus for the damage spell, plus SPECIFIC_SPELL_POWER for its damage" },
}};

/// ENCHANTED packed the mastery level and whether the whole side is affected into a single value.
int enchantedLevel(int value)
{
	return value > 3 ? value - 3 : value;
}

bool enchantedIsMassive(int value)
{
	return value > 3;
}

template<typename Entity>
std::string jsonKeyOf(const Entity * entity)
{
	return entity ? entity->getJsonKey() : std::string();
}

}

bool BonusMigration::migrateBonus(const JsonNode & ability, JsonNode & migrated)
{
	// BATTLE_NO_FLEEING is deprecated in 1.8 - it blocked retreating unconditionally, so its value is ignored
	if(withoutScope(ability["type"].String()) == "BATTLE_NO_FLEEING")
	{
		logMod->warn("Bonus BATTLE_NO_FLEEING is deprecated. Use BATTLE_CAN_FLEE with negative value instead");

		migrated = ability;
		migrated["type"].String() = "BATTLE_CAN_FLEE";
		migrated["val"].Float() = -GameConstants::BATTLE_RETREAT_BLOCK;
		return true;
	}

	// NOTE: NONEVIL_ALIGNMENT_MIX is also deprecated, but can not be converted here - it maps to two
	// ALIGNMENT_MIX bonuses. It is handled by CArmedInstance instead

	std::string_view script = lookup(retiredAbilities, withoutScope(ability["type"].String()));

	if(script.empty())
		return false;

	int value = ability["val"].Integer();

	// the magnitude keeps living in val and only the rest of the ability becomes parameters, so that
	// limiters, updaters and the description still see it as an ordinary bonus value
	JsonNode parameters;
	if(script == "soulSteal")
	{
		parameters["permanent"].Bool() = withoutScope(ability["subtype"].String()) != "soulStealBattle";
	}
	else if(script == "transmutation")
	{
		parameters["transmuteBy"].String() = withoutScope(ability["subtype"].String()) == "transmutationPerHealth" ? "health" : "count";
		// without a creature the script keeps the default of transmuting into the attacker's own
		if(!ability["addInfo"].isNull())
			parameters["creature"] = ability["addInfo"];
	}
	else if(script == "summonGuardians")
	{
		parameters["creature"] = ability["subtype"];
	}
	else if(script == "destruction")
	{
		parameters["killBy"].String() = withoutScope(ability["subtype"].String()) == "destructionKillPercentage" ? "percentage" : "count";
		// the old addInfo accepted both a bare number and a single-element array
		const JsonNode & amount = ability["addInfo"].isVector() ? ability["addInfo"][0] : ability["addInfo"];
		parameters["amount"].Integer() = amount.Integer();
	}
	else if(script == "deathStare")
	{
		parameters["situation"].String() = deathStareSituation(withoutScope(ability["subtype"].String()));
		// without an override the script casts death stare itself
		if(!ability["addInfo"].isNull())
			parameters["spell"] = ability["addInfo"];
	}
	else if(script == "enchanted")
	{
		// enchanted packs a mastery level and a flag rather than a magnitude, so val is unused
		parameters["spell"] = ability["subtype"];
		parameters["level"].Integer() = enchantedLevel(value);
		parameters["massive"].Bool() = enchantedIsMassive(value);
	}

	// everything else the config says - duration, limiters, icon, description - still applies
	migrated = ability;
	migrated["type"].String() = "COMBAT_EVENT_TRIGGER";
	migrated["subtype"].String() = std::string(script);

	if(script == "enchanted")
		migrated.Struct().erase("val");

	// the script lives in whichever mod provides it, not in the mod being migrated
	migrated["subtype"].setModScope(ModScope::scopeGame());
	migrated["addInfo"] = parameters;

	return true;
}

void BonusMigration::warnIfRetired(const JsonNode & ability, const TextIdentifier & descriptionID)
{
	std::string_view replacement = lookup(retiredWithoutMigration, withoutScope(ability["type"].String()));

	if(!replacement.empty())
		logMod->warn("Bonus %s no longer does anything and was not converted - declare %s instead. Description: '%s'", ability["type"].String(), replacement, descriptionID.get());
}

bool BonusMigration::migrateCombatAbility(Bonus & bonus)
{
	std::string scriptName;
	JsonNode parameters;

	switch(bonus.type)
	{
		case BonusType::UNUSED_LIFE_DRAIN:
			scriptName = "lifeDrain";
			break;

		case BonusType::UNUSED_SOUL_STEAL:
			scriptName = "soulSteal";
			parameters["permanent"].Bool() = bonus.subtype.getNum() != 1; // 1 was soulStealBattle
			break;

		case BonusType::UNUSED_TRANSMUTATION:
			scriptName = "transmutation";
			parameters["transmuteBy"].String() = bonus.subtype.getNum() == 0 ? "health" : "count"; // 0 was transmutationPerHealth
			if(bonus.parameters)
				parameters["creature"].String() = jsonKeyOf(bonus.parameters->toCreature().toEntity(LIBRARY));
			break;

		case BonusType::UNUSED_SUMMON_GUARDIANS:
			scriptName = "summonGuardians";
			parameters["creature"].String() = jsonKeyOf(bonus.subtype.as<CreatureID>().toEntity(LIBRARY));
			break;

		case BonusType::UNUSED_FIRE_SHIELD:
			scriptName = "fireShield";
			break;

		case BonusType::UNUSED_DESTRUCTION:
			scriptName = "destruction";
			parameters["killBy"].String() = bonus.subtype.getNum() == 0 ? "percentage" : "count"; // 0 was destructionKillPercentage
			if(bonus.parameters)
				parameters["amount"].Integer() = bonus.parameters->toNumber();
			break;

		case BonusType::UNUSED_DEATH_STARE:
			scriptName = "deathStare";
			parameters["situation"].String() = deathStareSituation(bonus.subtype);
			if(bonus.parameters && bonus.parameters->toSpell() != SpellID::NONE)
				parameters["spell"].String() = jsonKeyOf(bonus.parameters->toSpell().toEntity(LIBRARY));
			break;

		case BonusType::UNUSED_ENCHANTED:
			scriptName = "enchanted";
			parameters["spell"].String() = jsonKeyOf(bonus.subtype.as<SpellID>().toEntity(LIBRARY));
			parameters["level"].Integer() = enchantedLevel(bonus.val);
			parameters["massive"].Bool() = enchantedIsMassive(bonus.val);
			bonus.val = 0; // enchanted has no magnitude, its old value was a packed level
			break;

		default:
			return false;
	}

	ScriptID script = resolveScript(scriptName);

	// nothing provides the script this ability became, so leaving the bonus as the retired type it
	// already is - which the engine ignores - beats pointing it at no script at all
	if(!script.hasValue())
	{
		logMod->warn("Combat ability '%s' can not be converted - no such script is available!", scriptName);
		return false;
	}

	bonus.type = BonusType::COMBAT_EVENT_TRIGGER;
	bonus.subtype = BonusSubtypeID(script);
	bonus.parameters = std::make_shared<BonusParameters>(parameters);

	return true;
}
