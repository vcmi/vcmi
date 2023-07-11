/*
 * TavernSlot.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

enum class TavernHeroSlot : int8_t
{
	NONE = -1,
	NATIVE, // 1st / left slot in tavern, contains hero native to player's faction on new week
	RANDOM  // 2nd / right slot in tavern, contains hero of random class
};

enum class TavernSlotRole : int8_t
{
	NONE = -1,

	SINGLE_UNIT, // hero was added after buying hero from this slot, and only has 1 creature in army
	FULL_ARMY, // hero was added to tavern on new week and still has full army

	RETREATED, // hero was owned by player before, but have retreated from battle and only has 1 creature in army
	SURRENDERED, // hero was owned by player before, but have surrendered in battle and kept some troops

//	SURRENDERED_DAY7, // helper value for heroes that surrendered after 7th day during enemy turn
//	RETREATED_DAY7,
};

VCMI_LIB_NAMESPACE_END
