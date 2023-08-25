/*
 * NumericConstants.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

namespace GameConstants
{
	DLL_LINKAGE extern const std::string VCMI_VERSION;

	constexpr int PUZZLE_MAP_PIECES = 48;

	constexpr int MAX_HEROES_PER_PLAYER = 8;
	constexpr int AVAILABLE_HEROES_PER_PLAYER = 2;

	constexpr int ALL_PLAYERS = 255; //bitfield

	constexpr int CREATURES_PER_TOWN = 7; //without upgrades
	constexpr int SPELL_LEVELS = 5;
	constexpr int SPELL_SCHOOL_LEVELS = 4;
	constexpr int DEFAULT_SCHOOLS = 4;
	constexpr int CRE_LEVELS = 10; // number of creature experience levels

	constexpr int HERO_GOLD_COST = 2500;
	constexpr int SPELLBOOK_GOLD_COST = 500;
	constexpr int SKILL_GOLD_COST = 2000;
	constexpr int BATTLE_SHOOTING_PENALTY_DISTANCE = 10; //if the distance is > than this, then shooting stack has distance penalty
	constexpr int BATTLE_SHOOTING_RANGE_DISTANCE = std::numeric_limits<uint8_t>::max(); // used when shooting stack has no shooting range limit
	constexpr int ARMY_SIZE = 7;
	constexpr int SKILL_PER_HERO = 8;
	constexpr ui32 HERO_HIGH_LEVEL = 10; // affects primary skill upgrade order

	constexpr int SKILL_QUANTITY=28;
	constexpr int PRIMARY_SKILLS=4;
	constexpr int RESOURCE_QUANTITY=8;
	constexpr int HEROES_PER_TYPE=8; //amount of heroes of each type

	// amounts of OH3 objects. Can be changed by mods, should be used only during H3 loading phase
	constexpr int F_NUMBER = 9;
	constexpr int ARTIFACTS_QUANTITY=171;
	constexpr int HEROES_QUANTITY=156;
	constexpr int SPELLS_QUANTITY=70;
	constexpr int CREATURES_COUNT = 197;

	constexpr ui32 BASE_MOVEMENT_COST = 100; //default cost for non-diagonal movement

	constexpr int HERO_PORTRAIT_SHIFT = 9;// 2 special frames + 7 extra portraits

	constexpr std::array<int, 11> POSSIBLE_TURNTIME = {1, 2, 4, 6, 8, 10, 15, 20, 25, 30, 0};
}

VCMI_LIB_NAMESPACE_END
