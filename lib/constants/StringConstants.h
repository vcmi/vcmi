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

///
/// String ID which are pointless to move to config file - these types are mostly hardcoded
///
/// Stored as raw pointers so that the tables end up in read-only data instead of
/// being constructed at startup by every translation unit that includes this header.
///
namespace GameConstants
{
	inline constexpr std::array<const char *, RESOURCE_QUANTITY> RESOURCE_NAMES = {
		"wood", "mercury", "ore", "sulfur", "crystal", "gems", "gold"
	};

	inline constexpr std::array<const char *, PlayerColor::PLAYER_LIMIT_I> PLAYER_COLOR_NAMES = {
		"red", "blue", "tan", "green", "orange", "purple", "teal", "pink"
	};

	inline constexpr std::array<const char *, 4> ALIGNMENT_NAMES = {"good", "evil", "neutral", "none"};

	inline constexpr std::array<const char *, 5> DIFFICULTY_NAMES = {"pawn", "knight", "rook", "queen", "king"};
}

namespace NPrimarySkill
{
	inline constexpr std::array<const char *, GameConstants::PRIMARY_SKILLS> names = { "attack", "defence", "spellpower", "knowledge" };
}

namespace NSecondarySkill
{
	inline constexpr std::array<const char *, GameConstants::SKILL_QUANTITY> names =
	{
		"pathfinding",  "archery",      "logistics",    "scouting",     "diplomacy",    //  5
		"navigation",   "leadership",   "wisdom",       "mysticism",    "luck",         // 10
		"ballistics",   "eagleEye",     "necromancy",   "estates",      "fireMagic",    // 15
		"airMagic",     "waterMagic",   "earthMagic",   "scholar",      "tactics",      // 20
		"artillery",    "learning",     "offence",      "armorer",      "intelligence", // 25
		"sorcery",      "resistance",   "firstAid"
	};

	inline constexpr std::array<const char *, 4> levels =
	{
		"none", "basic", "advanced", "expert"
	};
}

namespace EBuildingType
{
	inline constexpr std::array<const char *, 46> names =
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
	inline constexpr std::array<const char *, GameConstants::F_NUMBER> names =
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

	inline constexpr const char * backpack = "backpack";
}

namespace NPathfindingLayer
{
	inline constexpr std::array<const char *, EPathfindingLayer::NUM_LAYERS> names =
	{
		"land", "sail", "water", "aviate", "air"
	};
}

namespace MappedKeys
{
	extern DLL_LINKAGE const std::map<std::string, BuildingSubID::EBuildingSubID> SPECIAL_BUILDINGS;

	extern DLL_LINKAGE const std::map<std::string, EMarketMode> MARKET_NAMES_TO_TYPES;
}
