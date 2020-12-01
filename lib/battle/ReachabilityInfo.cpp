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

int ReachabilityInfo::distToNearestNeighbour(
	const battle::Unit * attacker,
	const battle::Unit * defender,
	BattleHex * chosenHex) const
{
	int ret = 1000000;
	auto defenderHexes = battle::Unit::getHexes(
		defender->getPosition(),
		defender->doubleWide(),
		defender->unitSide());

	std::vector<BattleHex> targetableHexes;

	for(auto defenderHex : defenderHexes)
	{
		vstd::concatenate(targetableHexes, battle::Unit::getHexes(
			defenderHex,
			attacker->doubleWide(),
			defender->unitSide()));
	}

	vstd::removeDuplicates(targetableHexes);

	for(auto targetableHex : targetableHexes)
	{
		for(auto & n : targetableHex.neighbouringTiles())
		{
			if(distances[n] >= 0 && distances[n] < ret)
			{
				ret = distances[n];
				if(chosenHex)
					*chosenHex = n;
			}
		}
	}

	return ret;
}
