/*
 * ReachabilityInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ReachabilityInfo.h"
#include "Unit.h"


ReachabilityInfo::Parameters::Parameters()
{
	perspective = BattlePerspective::ALL_KNOWING;
	side = 0;
	doubleWide = flying = false;
}

ReachabilityInfo::Parameters::Parameters(const battle::Unit * Stack, BattleHex StartPosition)
{
	perspective = (BattlePerspective::BattlePerspective)(Stack->unitSide());
	startPosition = StartPosition;
	doubleWide = Stack->doubleWide();
	side = Stack->unitSide();
	flying = Stack->hasBonusOfType(Bonus::FLYING);
	knownAccessible = battle::Unit::getHexes(startPosition, doubleWide, side);
}

ReachabilityInfo::ReachabilityInfo()
{
	distances.fill(INFINITE_DIST);
	predecessors.fill(BattleHex::INVALID);
}

bool ReachabilityInfo::isReachable(BattleHex hex) const
{
	return distances[hex] < INFINITE_DIST;
}
