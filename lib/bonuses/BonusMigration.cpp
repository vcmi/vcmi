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
#include "../json/JsonNode.h"
#include <vcmi/Creature.h>
#include <vcmi/spells/Spell.h>
#include "../modding/IdentifierStorage.h"
#include "../modding/ModScope.h"

namespace
{

/// Identifiers may carry the scope of the mod that defined them, which says nothing about which
/// ability this is.
std::string withoutScope(const std::string & identifier)
{
	auto separator = identifier.find(':');
	return separator == std::string::npos ? identifier : identifier.substr(separator + 1);
}

CombatScriptID resolveScript(const std::string & name)
{
	auto index = LIBRARY->identifiers()->getIdentifier(ModScope::scopeGame(), "combatScript", name, false);
	return index.has_value() ? CombatScriptID(*index) : CombatScriptID::NONE;
}

/// Name of the script each retired bonus type is now implemented by.
const std::map<std::string, std::string> retiredAbilities = {
	{ "LIFE_DRAIN",       "lifeDrain" },
	{ "SOUL_STEAL",       "soulSteal" },
	{ "TRANSMUTATION",    "transmutation" },
	{ "SUMMON_GUARDIANS", "summonGuardians" },
	{ "ENCHANTED",        "enchanted" },
	{ "FIRE_SHIELD",      "fireShield" },
	{ "DESTRUCTION",      "destruction" },
	{ "DEATH_STARE",      "deathStare" },
};

/// Situation each of the old death stare subtypes stood for.
const std::map<std::string, std::string> deathStareSituations = {
	{ "deathStareGorgon",              "melee" },
	{ "deathStareCommander",           "commander" },
	{ "deathStareNoRangePenalty",      "ranged" },
	{ "deathStareRangePenalty",        "rangedDistancePenalty" },
	{ "deathStareObstaclePenalty",     "rangedWallPenalty" },
	{ "deathStareRangeObstaclePenalty","rangedDistanceAndWallPenalty" },
};

std::string deathStareSituation(const std::string & subtype)
{
	auto situation = deathStareSituations.find(subtype);
	return situation == deathStareSituations.end() ? "melee" : situation->second;
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
const std::map<std::string, std::string> retiredWithoutMigration = {
	{ "ACID_BREATH", "a SPELL_AFTER_ATTACK bonus for the damage spell, plus SPECIFIC_SPELL_POWER for its damage" },
};

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

bool BonusMigration::migrateCombatAbility(const JsonNode & ability, JsonNode & migrated)
{
	auto retired = retiredAbilities.find(withoutScope(ability["type"].String()));

	if(retired == retiredAbilities.end())
		return false;

	const std::string & script = retired->second;
	int value = ability["val"].Integer();

	// the magnitude keeps living in val, so that several sources of the same ability still add up
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
	migrated["subtype"].String() = script;

	if(script == "enchanted")
		migrated.Struct().erase("val");

	// the script lives in whichever mod provides it, not in the mod being migrated
	migrated["subtype"].setModScope(ModScope::scopeGame());
	migrated["addInfo"] = parameters;

	return true;
}

void BonusMigration::warnIfRetired(const JsonNode & ability, const TextIdentifier & descriptionID)
{
	auto retired = retiredWithoutMigration.find(withoutScope(ability["type"].String()));

	if(retired != retiredWithoutMigration.end())
		logMod->warn("Bonus %s no longer does anything and was not converted - declare %s instead. Description: '%s'", ability["type"].String(), retired->second, descriptionID.get());
}

bool BonusMigration::migrateCombatAbility(Bonus & bonus)
{
	CombatScriptID script;
	JsonNode parameters;

	switch(bonus.type)
	{
		case BonusType::UNUSED_LIFE_DRAIN:
			script = resolveScript("lifeDrain");
			break;

		case BonusType::UNUSED_SOUL_STEAL:
			script = resolveScript("soulSteal");
			parameters["permanent"].Bool() = bonus.subtype.getNum() != 1; // 1 was soulStealBattle
			break;

		case BonusType::UNUSED_TRANSMUTATION:
			script = resolveScript("transmutation");
			parameters["transmuteBy"].String() = bonus.subtype.getNum() == 0 ? "health" : "count"; // 0 was transmutationPerHealth
			if(bonus.parameters)
				parameters["creature"].String() = jsonKeyOf(bonus.parameters->toCreature().toEntity(LIBRARY));
			break;

		case BonusType::UNUSED_SUMMON_GUARDIANS:
			script = resolveScript("summonGuardians");
			parameters["creature"].String() = jsonKeyOf(bonus.subtype.as<CreatureID>().toEntity(LIBRARY));
			break;

		case BonusType::UNUSED_FIRE_SHIELD:
			script = resolveScript("fireShield");
			break;

		case BonusType::UNUSED_DESTRUCTION:
			script = resolveScript("destruction");
			parameters["killBy"].String() = bonus.subtype.getNum() == 0 ? "percentage" : "count"; // 0 was destructionKillPercentage
			if(bonus.parameters)
				parameters["amount"].Integer() = bonus.parameters->toNumber();
			break;

		case BonusType::UNUSED_DEATH_STARE:
			script = resolveScript("deathStare");
			parameters["situation"].String() = deathStareSituation(bonus.subtype);
			if(bonus.parameters && bonus.parameters->toSpell() != SpellID::NONE)
				parameters["spell"].String() = jsonKeyOf(bonus.parameters->toSpell().toEntity(LIBRARY));
			break;

		case BonusType::UNUSED_ENCHANTED:
			script = resolveScript("enchanted");
			parameters["spell"].String() = jsonKeyOf(bonus.subtype.as<SpellID>().toEntity(LIBRARY));
			parameters["level"].Integer() = enchantedLevel(bonus.val);
			parameters["massive"].Bool() = enchantedIsMassive(bonus.val);
			bonus.val = 0; // enchanted has no magnitude, its old value was a packed level
			break;

		default:
			return false;
	}

	// the script is registered by whichever mod provides it, which may not be loaded yet
	if(script == CombatScriptID::NONE)
		return false;

	bonus.type = BonusType::COMBAT_EVENT_TRIGGER;
	bonus.subtype = BonusSubtypeID(script);
	bonus.parameters = std::make_shared<BonusParameters>(parameters);

	return true;
}
