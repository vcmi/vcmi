/*
 * BattleHex.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "BattleHex.h"
#include "BattleHexArray.h"

VCMI_LIB_NAMESPACE_BEGIN

BattleHex BattleHex::getClosestTile(BattleSide side, const BattleHex & initialPos, const BattleHexArray & hexes)
{
	if(hexes.empty())
		return BattleHex();

	BattleHex initialHex = BattleHex(initialPos);
	int closestDistance = std::numeric_limits<int>::max();
	BattleHexArray closestTiles;

	for(const auto & hex : hexes)
	{
		int distance = initialHex.getDistance(initialHex, hex);
		if(distance < closestDistance)
		{
			closestDistance = distance;
			closestTiles.clear();
			closestTiles.insert(hex);
		}
		else if(distance == closestDistance)
			closestTiles.insert(hex);
	}

	auto compareHorizontal = [side, initialPos](const BattleHex & left, const BattleHex & right)
	{
		if(left.getX() != right.getX())
		{
			return (side == BattleSide::ATTACKER) ? (left.getX() > right.getX()) : (left.getX() < right.getX());
		}
		return std::abs(left.getY() - initialPos.getY()) < std::abs(right.getY() - initialPos.getY());
	};

	auto bestTile = std::min_element(closestTiles.begin(), closestTiles.end(), compareHorizontal);
	return (bestTile != closestTiles.end()) ? *bestTile : BattleHex();
}

const BattleHexArray & BattleHex::getAllNeighbouringTiles() const noexcept
{
	return BattleHexArray::getAllNeighbouringTiles(*this);
}

const BattleHexArray & BattleHex::getNeighbouringTiles() const noexcept
{
	return BattleHexArray::getNeighbouringTiles(*this);
}

const BattleHexArray & BattleHex::getNeighbouringTilesDoubleWide(BattleSide side) const noexcept
{
	return BattleHexArray::getNeighbouringTilesDoubleWide(*this, side);
}

std::ostream & operator<<(std::ostream & os, const BattleHex & hex)
{
	return os << hex.toInt();
}

VCMI_LIB_NAMESPACE_END
