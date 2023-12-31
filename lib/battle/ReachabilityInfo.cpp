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

VCMI_LIB_NAMESPACE_BEGIN

ReachabilityInfo::Parameters::Parameters(const battle::Unit * Stack, BattleHex StartPosition):
	perspective(static_cast<BattlePerspective::BattlePerspective>(Stack->unitSide())),
	startPosition(StartPosition),
	doubleWide(Stack->doubleWide()),
	side(Stack->unitSide()),
	flying(Stack->hasBonusOfType(BonusType::FLYING))
{
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

uint32_t ReachabilityInfo::distToNearestNeighbour(
	const std::vector<BattleHex> & targetHexes,
	BattleHex * chosenHex) const
{
	uint32_t ret = 1000000;

	for(auto targetHex : targetHexes)
	{
		for(auto & n : targetHex.neighbouringTiles())
		{
			if(distances[n] < ret)
			{
				ret = distances[n];
				if(chosenHex)
					*chosenHex = n;
			}
		}
	}

	return ret;
}

uint32_t ReachabilityInfo::distToNearestNeighbour(
	const battle::Unit * attacker,
	const battle::Unit * defender,
	BattleHex * chosenHex) const
{
	auto attackableHexes = defender->getHexes();

	if(attacker->doubleWide())
	{
		vstd::concatenate(attackableHexes, battle::Unit::getHexes(defender->occupiedHex(), true, attacker->unitSide()));
	}

	return distToNearestNeighbour(attackableHexes, chosenHex);
}

VCMI_LIB_NAMESPACE_END
