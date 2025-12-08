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
		"wood", "mercury", "ore", "sulfur", "crystal", "gems", "gold"
	};

	const std::string PLAYER_COLOR_NAMES [PlayerColor::PLAYER_LIMIT_I] = {
		"red", "blue", "tan", "green", "orange", "purple", "teal", "pink"
	};

	const std::string ALIGNMENT_NAMES [3] = {"good", "evil", "neutral"};

	const std::string DIFFICULTY_NAMES [5] = {"pawn", "knight", "rook", "queen", "king"};
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
	const std::string names [46] =
	{
		"mageGuild1",       "mageGuild2",       "mageGuild3",       "mageGuild4",       "mageGuild5",       //  5
		"tavern",           "shipyard",         "fort",             "citadel",          "castle",           // 10
		"villageHall",      "townHall",         "cityHall",         "capitol",          "marketplace",      // 15
		"resourceSilo",     "blacksmith",       "special1",         "horde1",           "horde1Upgr",       // 20
		"ship",             "special2",         "special3",         "special4",         "horde2",           // 25
		"horde2Upgr",       "grail",            "extraTownHall",    "extraCityHall",    "extraCapitol",     // 30
		"dwellingLvl1",     "dwellingLvl2",     "dwellingLvl3",     "dwellingLvl4",     "dwellingLvl5",     // 35
		"dwellingLvl6",     "dwellingLvl7",     "dwellingUpLvl1",   "dwellingUpLvl2",   "dwellingUpLvl3",   // 40
		"dwellingUpLvl4",   "dwellingUpLvl5",   "dwellingUpLvl6",   "dwellingUpLvl7",   "dwellingLvl8",
		"dwellingUpLvl8"
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
	inline constexpr std::array namesHero =
	{
		"head", "shoulders", "neck", "rightHand", "leftHand", "torso", //5
		"rightRing", "leftRing", "feet", //8
		"misc1", "misc2", "misc3", "misc4", //12
		"mach1", "mach2", "mach3", "mach4", //16
		"spellbook", "misc5" //18
	};

	inline constexpr std::array namesCreature =
	{
		"creature1"
	};

	inline constexpr std::array namesCommander =
	{
		"commander1", "commander2", "commander3", "commander4", "commander5", "commander6", "commander7", "commander8", "commander9"
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
	static const std::map<std::string, BuildingSubID::EBuildingSubID> SPECIAL_BUILDINGS =
	{
		{ "mysticPond", BuildingSubID::MYSTIC_POND },
		{ "castleGate", BuildingSubID::CASTLE_GATE },
		{ "portalOfSummoning", BuildingSubID::PORTAL_OF_SUMMONING },
		{ "library", BuildingSubID::LIBRARY },
		{ "escapeTunnel", BuildingSubID::ESCAPE_TUNNEL },
		{ "treasury", BuildingSubID::TREASURY },
		{ "bank", BuildingSubID::BANK },
		{ "auroraBorealis", BuildingSubID::AURORA_BOREALIS },
		{ "deityOfFire", BuildingSubID::DEITY_OF_FIRE }
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
