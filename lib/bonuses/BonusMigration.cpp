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

bool BonusMigration::migrateCombatAbility(Bonus & bonus)
{
	CombatScriptID script;
	JsonNode parameters;

	switch(bonus.type)
	{
		case BonusType::LIFE_DRAIN:
			script = resolveScript("lifeDrain");
			break;

		case BonusType::SOUL_STEAL:
			script = resolveScript("soulSteal");
			parameters["permanent"].Bool() = bonus.subtype != BonusCustomSubtype::soulStealBattle;
			break;

		case BonusType::TRANSMUTATION:
			script = resolveScript("transmutation");
			parameters["transmuteBy"].String() = bonus.subtype == BonusCustomSubtype::transmutationPerHealth ? "health" : "count";
			if(bonus.parameters)
				parameters["creature"].String() = jsonKeyOf(bonus.parameters->toCreature().toEntity(LIBRARY));
			break;

		case BonusType::SUMMON_GUARDIANS:
			script = resolveScript("summonGuardians");
			parameters["creature"].String() = jsonKeyOf(bonus.subtype.as<CreatureID>().toEntity(LIBRARY));
			break;

		case BonusType::ENCHANTED:
			script = resolveScript("enchanted");
			parameters["spell"].String() = jsonKeyOf(bonus.subtype.as<SpellID>().toEntity(LIBRARY));
			parameters["level"].Integer() = enchantedLevel(bonus.val);
			parameters["massive"].Bool() = enchantedIsMassive(bonus.val);
			bonus.val = 0; // enchanted has no magnitude, its old value was a packed level
			break;

		default:
			return false;
	}

	bonus.type = BonusType::COMBAT_EVENT_TRIGGER;
	bonus.subtype = BonusSubtypeID(script);
	bonus.parameters = std::make_shared<BonusParameters>(parameters);

	return true;
}
