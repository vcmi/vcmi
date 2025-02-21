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

ReachabilityInfo::Parameters::Parameters(const battle::Unit * Stack, const BattleHex & StartPosition):
	perspective(static_cast<BattleSide>(Stack->unitSide())),
	startPosition(StartPosition),
	doubleWide(Stack->doubleWide()),
	side(Stack->unitSide()),
	flying(Stack->hasBonusOfType(BonusType::FLYING))
{
	knownAccessible = & battle::Unit::getHexes(startPosition, doubleWide, side);
	destructibleEnemyTurns.fill(-1);
}

ReachabilityInfo::ReachabilityInfo()
{
	distances.fill(INFINITE_DIST);
	predecessors.fill(BattleHex::INVALID);
}

bool ReachabilityInfo::isReachable(const BattleHex & hex) const
{
	return distances[hex.toInt()] < INFINITE_DIST;
}

uint32_t ReachabilityInfo::distToNearestNeighbour(
	const BattleHexArray & targetHexes,
	BattleHex * chosenHex) const
{
	uint32_t ret = 1000000;

	for(const auto & targetHex : targetHexes)
	{
		for(auto & n : targetHex.getNeighbouringTiles())
		{
			if(distances[n.toInt()] < ret)
			{
				ret = distances[n.toInt()];
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
		if(defender->doubleWide())
		{
			// It can be back to back attack  o==o  or head to head  =oo=.
			// In case of back-to-back the distance between heads (unit positions) may be up to 3 tiles
			attackableHexes.insert(battle::Unit::getHexes(defender->occupiedHex(), true, defender->unitSide()));
		}
		else
		{
			attackableHexes.insert(battle::Unit::getHexes(defender->getPosition(), true, defender->unitSide()));
		}
	}

	attackableHexes.eraseIf([defender](const BattleHex & h) -> bool
		{
			return h.getY() != defender->getPosition().getY() || !h.isAvailable();
		});

	return distToNearestNeighbour(attackableHexes, chosenHex);
}

VCMI_LIB_NAMESPACE_END
