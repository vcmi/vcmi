/*
 * ReachabilityInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "../StdInc.h"
#include "ReachabilityInfo.h"
#include "../CStack.h"


ReachabilityInfo::Parameters::Parameters()
{
	stack = nullptr;
	perspective = BattlePerspective::ALL_KNOWING;
	attackerOwned = doubleWide = flying = false;
}

ReachabilityInfo::Parameters::Parameters(const CStack * Stack)
{
	stack = Stack;
	perspective = (BattlePerspective::BattlePerspective)(!Stack->attackerOwned);
	startPosition = Stack->position;
	doubleWide = stack->doubleWide();
	attackerOwned = stack->attackerOwned;
	flying = stack->hasBonusOfType(Bonus::FLYING);
	knownAccessible = stack->getHexes();
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
