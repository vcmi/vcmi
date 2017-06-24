/*
 * AccessibilityInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "AccessibilityInfo.h"
#include "CStack.h"

bool AccessibilityInfo::accessible(BattleHex tile, const CStack *stack) const
{
	return accessible(tile, stack->doubleWide(), stack->attackerOwned);
}

bool AccessibilityInfo::accessible(BattleHex tile, bool doubleWide, bool attackerOwned) const
{
	// All hexes that stack would cover if standing on tile have to be accessible.
	for(auto hex : CStack::getHexes(tile, doubleWide, attackerOwned))
	{
		// If the hex is out of range then the tile isn't accessible
		if(!hex.isValid())
			return false;
		// If we're no defender which step on gate and the hex isn't accessible, then the tile
		// isn't accessible
		else if(at(hex) != EAccessibility::ACCESSIBLE &&
				!(at(hex) == EAccessibility::GATE && !attackerOwned))
		{
			return false;
		}
	}
	return true;
}

bool AccessibilityInfo::occupiable(const CStack *stack, BattleHex tile) const
{
	//obviously, we can occupy tile by standing on it
	if(accessible(tile, stack))
		return true;

	if(stack->doubleWide())
	{
		//Check the tile next to -> if stack stands there, it'll also occupy considered hex
		const BattleHex anotherTile = tile + (stack->attackerOwned ? BattleHex::RIGHT : BattleHex::LEFT);
		if(accessible(anotherTile, stack))
			return true;
	}

	return false;
}
