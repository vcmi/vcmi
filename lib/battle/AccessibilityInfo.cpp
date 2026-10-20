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
#include "BattleHex.h"
#include "Unit.h"
#include "../GameConstants.h"

bool AccessibilityInfo::tileAccessibleWithGate(const BattleHex & tile, bool canUseGate) const
{
	const auto & accessibility = at(tile.toInt());

	if(accessibility == EAccessibility::ALIVE_STACK)
	{
		if(!destructibleEnemyTurns)
			return false;

		return destructibleEnemyTurns->at(tile.toInt()) >= 0;
	}

	if(accessibility != EAccessibility::ACCESSIBLE)
		if(accessibility != EAccessibility::GATE || !canUseGate)
			return false;

	return true;
}

bool AccessibilityInfo::accessible(const BattleHex & tile, const battle::Unit * stack) const
{
	return accessible(tile, stack->doubleWide(), stack->unitSide(), stack->controllingSide() == BattleSide::DEFENDER);
}

bool AccessibilityInfo::accessible(const BattleHex & tile, bool doubleWide, BattleSide side, bool canUseGate) const
{
	// All hexes that stack would cover if standing on tile have to be accessible.
	//do not use getHexes for speed reasons
	if(!tile.isValid())
		return false;
	if(!tileAccessibleWithGate(tile, canUseGate))
		return false;

	if(doubleWide)
	{
		auto otherHex = battle::Unit::occupiedHex(tile, doubleWide, side);
		if(!otherHex.isValid())
			return false;
		if(!tileAccessibleWithGate(otherHex, canUseGate))
			return false;
	}

	return true;
}
