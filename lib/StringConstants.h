#pragma once

#include "GameConstants.h"

/*
 * GameConstants.h, part of VCMI engine
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

	const std::string HERO_CLASSES_NAMES [F_NUMBER * 2] = {
	    "knight",    "cleric",     "ranger",      "druid",       "alchemist",    "wizard",
	    "demoniac",  "heretic",    "deathknight", "necromancer", "warlock",      "overlord",
	    "barbarian", "battlemage", "beastmaster", "witch",       "planeswalker", "elementalist"
	};
	const std::string PLAYER_COLOR_NAMES [PLAYER_LIMIT] = {
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

namespace ETownType
{
	const std::string names [GameConstants::F_NUMBER] =
	{
		"castle",       "rampart",      "tower",
		"inferno",      "necropolis",   "dungeon",
		"stronghold",   "fortress",     "conflux"
	};
}
