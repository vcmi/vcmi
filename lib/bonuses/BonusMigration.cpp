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

	JsonNode parameters;
	if(script == "lifeDrain")
	{
		parameters["percentage"].Integer() = value;
	}
	else if(script == "soulSteal")
	{
		parameters["creaturesPerKill"].Integer() = value;
		parameters["permanent"].Bool() = withoutScope(ability["subtype"].String()) != "soulStealBattle";
	}
	else if(script == "transmutation")
	{
		parameters["chance"].Integer() = value;
		parameters["transmuteBy"].String() = withoutScope(ability["subtype"].String()) == "transmutationPerHealth" ? "health" : "count";
		// without a creature the script keeps the default of transmuting into the attacker's own
		if(!ability["addInfo"].isNull())
			parameters["creature"] = ability["addInfo"];
	}
	else if(script == "summonGuardians")
	{
		parameters["creature"] = ability["subtype"];
		parameters["percentage"].Integer() = value;
	}
	else if(script == "enchanted")
	{
		parameters["spell"] = ability["subtype"];
		parameters["level"].Integer() = enchantedLevel(value);
		parameters["massive"].Bool() = enchantedIsMassive(value);
	}

	// everything else the config says - duration, limiters, icon, description - still applies
	migrated = ability;
	migrated.Struct().erase("subtype");
	migrated.Struct().erase("val");

	migrated["type"].String() = "COMBAT_EVENT_TRIGGER";

	JsonNode addInfo;
	addInfo["eventScript"].String() = script;
	addInfo["eventParameters"] = parameters;
	// the replacement lives in whichever mod provides the script, not in the mod being migrated
	addInfo.setModScope(ModScope::scopeGame());

	migrated["addInfo"] = addInfo;

	return true;
}

bool BonusMigration::migrateCombatAbility(Bonus & bonus)
{
	BonusParametersCombatScript data;

	switch(bonus.type)
	{
		case BonusType::LIFE_DRAIN:
			data.eventScript = resolveScript("lifeDrain");
			data.eventParameters["percentage"].Integer() = bonus.val;
			break;

		case BonusType::SOUL_STEAL:
			data.eventScript = resolveScript("soulSteal");
			data.eventParameters["creaturesPerKill"].Integer() = bonus.val;
			data.eventParameters["permanent"].Bool() = bonus.subtype != BonusCustomSubtype::soulStealBattle;
			break;

		case BonusType::TRANSMUTATION:
			data.eventScript = resolveScript("transmutation");
			data.eventParameters["chance"].Integer() = bonus.val;
			data.eventParameters["transmuteBy"].String() = bonus.subtype == BonusCustomSubtype::transmutationPerHealth ? "health" : "count";
			if(bonus.parameters)
				data.eventParameters["creature"].String() = jsonKeyOf(bonus.parameters->toCreature().toEntity(LIBRARY));
			break;

		case BonusType::SUMMON_GUARDIANS:
			data.eventScript = resolveScript("summonGuardians");
			data.eventParameters["creature"].String() = jsonKeyOf(bonus.subtype.as<CreatureID>().toEntity(LIBRARY));
			data.eventParameters["percentage"].Integer() = bonus.val;
			break;

		case BonusType::ENCHANTED:
			data.eventScript = resolveScript("enchanted");
			data.eventParameters["spell"].String() = jsonKeyOf(bonus.subtype.as<SpellID>().toEntity(LIBRARY));
			data.eventParameters["level"].Integer() = enchantedLevel(bonus.val);
			data.eventParameters["massive"].Bool() = enchantedIsMassive(bonus.val);
			break;

		default:
			return false;
	}

	bonus.type = BonusType::COMBAT_EVENT_TRIGGER;
	bonus.subtype = BonusSubtypeID();
	bonus.val = 0;
	bonus.parameters = std::make_shared<BonusParameters>(data);

	return true;
}
