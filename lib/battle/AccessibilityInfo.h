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
	SIDE_COLUMN //used for first and last columns of hexes that are unavailable but war machines can stand there
};


using TAccessibilityArray = std::array<EAccessibility, GameConstants::BFIELD_SIZE>;
using TBattlefieldTurnsArray = std::array<int8_t, GameConstants::BFIELD_SIZE>;

struct DLL_LINKAGE AccessibilityInfo : TAccessibilityArray
{
	std::shared_ptr<const TBattlefieldTurnsArray> destructibleEnemyTurns; //used only as a view for destructibleEnemyTurns from ReachabilityInfo::Parameters

	public:
		bool accessible(const BattleHex & tile, const battle::Unit * stack) const; //checks for both tiles if stack is double wide
		bool accessible(const BattleHex & tile, bool doubleWide, BattleSide side) const; //checks for both tiles if stack is double wide
	private:
		bool tileAccessibleWithGate(const BattleHex & tile, BattleSide side) const;
};

VCMI_LIB_NAMESPACE_END
