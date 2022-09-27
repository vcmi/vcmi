/*
 * AccessibilityInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "BattleHex.h"
#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace battle
{
	class Unit;
}

//Accessibility is property of hex in battle. It doesn't depend on stack, side's perspective and so on.
enum class EAccessibility
{
	ACCESSIBLE,
	ALIVE_STACK,
	OBSTACLE,
	DESTRUCTIBLE_WALL,
	GATE, //sieges -> gate opens only for defender stacks
	UNAVAILABLE, //indestructible wall parts, special battlefields (like boat-to-boat)
	SIDE_COLUMN //used for first and last columns of hexes that are unavailable but wat machines can stand there
};


typedef std::array<EAccessibility, GameConstants::BFIELD_SIZE> TAccessibilityArray;

struct DLL_LINKAGE AccessibilityInfo : TAccessibilityArray
{
	bool accessible(BattleHex tile, const battle::Unit * stack) const; //checks for both tiles if stack is double wide
	bool accessible(BattleHex tile, bool doubleWide, ui8 side) const; //checks for both tiles if stack is double wide
};

VCMI_LIB_NAMESPACE_END
