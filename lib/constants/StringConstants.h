/*
 * constants/StringConstants.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

///
/// String ID which are pointless to move to config file - these types are mostly hardcoded
///
namespace GameConstants
{
	const std::string RESOURCE_NAMES [RESOURCE_QUANTITY] = {
	    "wood", "mercury", "ore", "sulfur", "crystal", "gems", "gold", "mithril"
	};

	const std::string PLAYER_COLOR_NAMES [PlayerColor::PLAYER_LIMIT_I] = {
		"red", "blue", "tan", "green", "orange", "purple", "teal", "pink"
	};

	const std::string ALIGNMENT_NAMES [3] = {"good", "evil", "neutral"};
}

namespace NPrimarySkill
{
	const std::string names [GameConstants::PRIMARY_SKILLS] = { "attack", "defence", "spellpower", "knowledge" };
}

namespace NSecondarySkill
{
	const std::string names [GameConstants::SKILL_QUANTITY] =
	{
		"pathfinding",  "archery",      "logistics",    "scouting",     "diplomacy",    //  5
		"navigation",   "leadership",   "wisdom",       "mysticism",    "luck",         // 10
		"ballistics",   "eagleEye",     "necromancy",   "estates",      "fireMagic",    // 15
		"airMagic",     "waterMagic",   "earthMagic",   "scholar",      "tactics",      // 20
		"artillery",    "learning",     "offence",      "armorer",      "intelligence", // 25
		"sorcery",      "resistance",   "firstAid"
	};

	const std::vector<std::string> levels =
	{
	    "none", "basic", "advanced", "expert"
	};
}

namespace EBuildingType
{
	const std::string names [44] =
	{
		"mageGuild1",       "mageGuild2",       "mageGuild3",       "mageGuild4",       "mageGuild5",       //  5
		"tavern",           "shipyard",         "fort",             "citadel",          "castle",           // 10
		"villageHall",      "townHall",         "cityHall",         "capitol",          "marketplace",      // 15
		"resourceSilo",     "blacksmith",       "special1",         "horde1",           "horde1Upgr",       // 20
		"ship",             "special2",         "special3",         "special4",         "horde2",           // 25
		"horde2Upgr",       "grail",            "extraTownHall",    "extraCityHall",    "extraCapitol",     // 30
		"dwellingLvl1",     "dwellingLvl2",     "dwellingLvl3",     "dwellingLvl4",     "dwellingLvl5",     // 35
		"dwellingLvl6",     "dwellingLvl7",     "dwellingUpLvl1",   "dwellingUpLvl2",   "dwellingUpLvl3",   // 40
		"dwellingUpLvl4",   "dwellingUpLvl5",   "dwellingUpLvl6",   "dwellingUpLvl7"
	};
}

namespace NFaction
{
	const std::string names [GameConstants::F_NUMBER] =
	{
		"castle",       "rampart",      "tower",
		"inferno",      "necropolis",   "dungeon",
		"stronghold",   "fortress",     "conflux"
	};
}

namespace NArtifactPosition
{
	const std::string namesHero [19] =
	{
		"head", "shoulders", "neck", "rightHand", "leftHand", "torso", //5
		"rightRing", "leftRing", "feet", //8
		"misc1", "misc2", "misc3", "misc4", //12
		"mach1", "mach2", "mach3", "mach4", //16
		"spellbook", "misc5" //18
	};

	const std::string namesCreature[1] =
	{
		"creature1"
	};

	const std::string namesCommander[6] =
	{
		"commander1", "commander2", "commander3", "commander4", "commander5", "commander6",
	};


	const std::string backpack = "backpack";
}

namespace NMetaclass
{
    const std::string names [16] =
    {
		"",
		"artifact", "creature", "faction", "experience", "hero",
		"heroClass", "luck", "mana", "morale", "movement",
		"object", "primarySkill", "secondarySkill", "spell", "resource"
    };
}

namespace NPathfindingLayer
{
	const std::string names[EPathfindingLayer::NUM_LAYERS] =
	{
		"land", "sail", "water", "air"
	};
}

namespace MappedKeys
{

	static const std::map<std::string, BuildingID> BUILDING_NAMES_TO_TYPES =
	{
		{ "special1", BuildingID::SPECIAL_1 },
		{ "special2", BuildingID::SPECIAL_2 },
		{ "special3", BuildingID::SPECIAL_3 },
		{ "special4", BuildingID::SPECIAL_4 },
		{ "grail", BuildingID::GRAIL },
		{ "mageGuild1", BuildingID::MAGES_GUILD_1 },
		{ "mageGuild2", BuildingID::MAGES_GUILD_2 },
		{ "mageGuild3", BuildingID::MAGES_GUILD_3 },
		{ "mageGuild4", BuildingID::MAGES_GUILD_4 },
		{ "mageGuild5", BuildingID::MAGES_GUILD_5 },
		{ "tavern", BuildingID::TAVERN },
		{ "shipyard", BuildingID::SHIPYARD },
		{ "fort", BuildingID::FORT },
		{ "citadel", BuildingID::CITADEL },
		{ "castle", BuildingID::CASTLE },
		{ "villageHall", BuildingID::VILLAGE_HALL },
		{ "townHall", BuildingID::TOWN_HALL },
		{ "cityHall", BuildingID::CITY_HALL },
		{ "capitol", BuildingID::CAPITOL },
		{ "marketplace", BuildingID::MARKETPLACE },
		{ "resourceSilo", BuildingID::RESOURCE_SILO },
		{ "blacksmith", BuildingID::BLACKSMITH },
		{ "horde1", BuildingID::HORDE_1 },
		{ "horde1Upgr", BuildingID::HORDE_1_UPGR },
		{ "horde2", BuildingID::HORDE_2 },
		{ "horde2Upgr", BuildingID::HORDE_2_UPGR },
		{ "ship", BuildingID::SHIP },
		{ "dwellingLvl1", BuildingID::DWELL_LVL_1 },
		{ "dwellingLvl2", BuildingID::DWELL_LVL_2 },
		{ "dwellingLvl3", BuildingID::DWELL_LVL_3 },
		{ "dwellingLvl4", BuildingID::DWELL_LVL_4 },
		{ "dwellingLvl5", BuildingID::DWELL_LVL_5 },
		{ "dwellingLvl6", BuildingID::DWELL_LVL_6 },
		{ "dwellingLvl7", BuildingID::DWELL_LVL_7 },
		{ "dwellingUpLvl1", BuildingID::DWELL_LVL_1_UP },
		{ "dwellingUpLvl2", BuildingID::DWELL_LVL_2_UP },
		{ "dwellingUpLvl3", BuildingID::DWELL_LVL_3_UP },
		{ "dwellingUpLvl4", BuildingID::DWELL_LVL_4_UP },
		{ "dwellingUpLvl5", BuildingID::DWELL_LVL_5_UP },
		{ "dwellingUpLvl6", BuildingID::DWELL_LVL_6_UP },
		{ "dwellingUpLvl7", BuildingID::DWELL_LVL_7_UP },
	};

	static const std::map<std::string, BuildingSubID::EBuildingSubID> SPECIAL_BUILDINGS =
	{
		{ "mysticPond", BuildingSubID::MYSTIC_POND },
		{ "artifactMerchant", BuildingSubID::ARTIFACT_MERCHANT },
		{ "freelancersGuild", BuildingSubID::FREELANCERS_GUILD },
		{ "magicUniversity", BuildingSubID::MAGIC_UNIVERSITY },
		{ "castleGate", BuildingSubID::CASTLE_GATE },
		{ "creatureTransformer", BuildingSubID::CREATURE_TRANSFORMER },//only skeleton transformer yet
		{ "portalOfSummoning", BuildingSubID::PORTAL_OF_SUMMONING },
		{ "ballistaYard", BuildingSubID::BALLISTA_YARD },
		{ "stables", BuildingSubID::STABLES },
		{ "manaVortex", BuildingSubID::MANA_VORTEX },
		{ "lookoutTower", BuildingSubID::LOOKOUT_TOWER },
		{ "library", BuildingSubID::LIBRARY },
		{ "brotherhoodOfSword", BuildingSubID::BROTHERHOOD_OF_SWORD },//morale garrison bonus
		{ "fountainOfFortune", BuildingSubID::FOUNTAIN_OF_FORTUNE },//luck garrison bonus
		{ "spellPowerGarrisonBonus", BuildingSubID::SPELL_POWER_GARRISON_BONUS },//such as 'stormclouds', but this name is not ok for good towns
		{ "attackGarrisonBonus", BuildingSubID::ATTACK_GARRISON_BONUS },
		{ "defenseGarrisonBonus", BuildingSubID::DEFENSE_GARRISON_BONUS },
		{ "escapeTunnel", BuildingSubID::ESCAPE_TUNNEL },
		{ "attackVisitingBonus", BuildingSubID::ATTACK_VISITING_BONUS },
		{ "defenceVisitingBonus", BuildingSubID::DEFENSE_VISITING_BONUS },
		{ "spellPowerVisitingBonus", BuildingSubID::SPELL_POWER_VISITING_BONUS },
		{ "knowledgeVisitingBonus", BuildingSubID::KNOWLEDGE_VISITING_BONUS },
		{ "experienceVisitingBonus", BuildingSubID::EXPERIENCE_VISITING_BONUS },
		{ "lighthouse", BuildingSubID::LIGHTHOUSE },
		{ "treasury", BuildingSubID::TREASURY }
	};

	static const std::map<std::string, EMarketMode> MARKET_NAMES_TO_TYPES =
	{
		{ "resource-resource", EMarketMode::RESOURCE_RESOURCE },
		{ "resource-player", EMarketMode::RESOURCE_PLAYER },
		{ "creature-resource", EMarketMode::CREATURE_RESOURCE },
		{ "resource-artifact", EMarketMode::RESOURCE_ARTIFACT },
		{ "artifact-resource", EMarketMode::ARTIFACT_RESOURCE },
		{ "artifact-experience", EMarketMode::ARTIFACT_EXP },
		{ "creature-experience", EMarketMode::CREATURE_EXP },
		{ "creature-undead", EMarketMode::CREATURE_UNDEAD },
		{ "resource-skill", EMarketMode::RESOURCE_SKILL },
	};
}


VCMI_LIB_NAMESPACE_END
