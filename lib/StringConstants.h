#pragma once

#include "GameConstants.h"

/*
 * StringConstants.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */


///
/// String ID which are pointless to move to config file - these types are mostly hardcoded
///
namespace GameConstants
{
	const std::string TERRAIN_NAMES [TERRAIN_TYPES] = {
	    "dirt", "sand", "grass", "snow", "swamp", "rough", "subterra", "lava", "water", "rock"
	};
	
	const std::string RESOURCE_NAMES [RESOURCE_QUANTITY] = {
	    "wood", "mercury", "ore", "sulfur", "crystal", "gems", "gold", "mithril"
	};

	const std::string PLAYER_COLOR_NAMES [PlayerColor::PLAYER_LIMIT_I] = {
		"red", "blue", "tan", "green", "orange", "purple", "teal", "pink"
	};
}

namespace EAlignment
{
	const std::string names [3] = {"good", "evil", "neutral"};
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

	const std::string levels [4] =
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
