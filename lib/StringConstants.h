/*
 * StringConstants.h, part of VCMI engine
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

namespace PrimarySkill
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
		"mageGuild1",       "mageGuild2",       "mageGuild3",       "mageGuild4",       "mageGuild5",
		"tavern",           "shipyard",         "fort",             "citadel",          "castle",
		"villageHall",      "townHall",         "cityHall",         "capitol",          "marketplace",
		"resourceSilo",     "blacksmith",       "special1",         "horde1",           "horde1Upgr",
		"ship",             "special2",         "special3",         "special4",         "horde2",
		"horde2Upgr",       "grail",            "extraTownHall",    "extraCityHall",    "extraCapitol",
		"dwellingLvl1",     "dwellingLvl2",     "dwellingLvl3",     "dwellingLvl4",     "dwellingLvl5",
		"dwellingLvl6",     "dwellingLvl7",     "dwellingUpLvl1",   "dwellingUpLvl2",   "dwellingUpLvl3",
		"dwellingUpLvl4",   "dwellingUpLvl5",   "dwellingUpLvl6",   "dwellingUpLvl7"
	};
}

namespace ETownType
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

VCMI_LIB_NAMESPACE_END
