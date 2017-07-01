/*
 * AccessibilityInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "../StdInc.h"
#include "AccessibilityInfo.h"
#include "../CStack.h"
#include "../GameConstants.h"

bool AccessibilityInfo::accessible(BattleHex tile, const CStack * stack) const
{
	return accessible(tile, stack->doubleWide(), stack->side);
}

bool AccessibilityInfo::accessible(BattleHex tile, bool doubleWide, ui8 side) const
{
	// All hexes that stack would cover if standing on tile have to be accessible.
	for(auto hex : CStack::getHexes(tile, doubleWide, side))
	{
		// If the hex is out of range then the tile isn't accessible
		if(!hex.isValid())
			return false;
		// If we're no defender which step on gate and the hex isn't accessible, then the tile
		// isn't accessible
		else if(at(hex) != EAccessibility::ACCESSIBLE &&
				!(at(hex) == EAccessibility::GATE && side == BattleSide::DEFENDER))
		{
			return false;
		}
	}
	return true;
}
