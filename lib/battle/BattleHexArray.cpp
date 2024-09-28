/*
 * BattleHexArray.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BattleHexArray.h"

VCMI_LIB_NAMESPACE_BEGIN

BattleHexArray::BattleHexArray(std::initializer_list<BattleHex> initList) noexcept 
	: BattleHexArray()
{
	for(auto hex : initList)
	{
		insert(hex);
	}
}

BattleHex BattleHexArray::getClosestTile(BattleSide side, BattleHex initialPos)
{
	BattleHex initialHex = BattleHex(initialPos);
	auto compareDistance = [initialHex](const BattleHex left, const BattleHex right) -> bool
	{
		return initialHex.getDistance(initialHex, left) < initialHex.getDistance(initialHex, right);
	};
	BattleHexArray sortedTiles(*this);
	boost::sort(sortedTiles, compareDistance); //closest tiles at front
	int closestDistance = initialHex.getDistance(initialPos, sortedTiles.front()); //sometimes closest tiles can be many hexes away
	auto notClosest = [closestDistance, initialPos](const BattleHex here) -> bool
	{
		return closestDistance < here.getDistance(initialPos, here);
	};
	vstd::erase_if(sortedTiles, notClosest); //only closest tiles are interesting
	auto compareHorizontal = [side, initialPos](const BattleHex left, const BattleHex right) -> bool
	{
		if(left.getX() != right.getX())
		{
			if(side == BattleSide::ATTACKER)
				return left.getX() > right.getX(); //find furthest right
			else
				return left.getX() < right.getX(); //find furthest left
		}
		else
		{
			//Prefer tiles in the same row.
			return std::abs(left.getY() - initialPos.getY()) < std::abs(right.getY() - initialPos.getY());
		}
	};
	boost::sort(sortedTiles, compareHorizontal);
	return sortedTiles.front();
}

void BattleHexArray::merge(const BattleHexArray & other) noexcept
{
	for(auto hex : other)
	{
		insert(hex);
	}
}

void BattleHexArray::erase(iterator first, iterator last) noexcept
{
	for(auto it = first; it != last && it != internalStorage.end(); ++it)
	{
		presenceFlags[*it] = 0;
	}

	internalStorage.erase(first, last);
}

void BattleHexArray::clear() noexcept
{
	for(auto hex : internalStorage)
		presenceFlags[hex] = 0;

	internalStorage.clear();
}

static BattleHexArray::NeighbouringTilesCache calculateNeighbouringTiles()
{
	BattleHexArray::NeighbouringTilesCache ret;

	for(si16 hex = 0; hex < GameConstants::BFIELD_SIZE; hex++)
	{
		auto hexes = BattleHexArray::generateNeighbouringTiles(hex);

		size_t index = 0;
		for(auto neighbour : hexes)
			ret[hex].at(index++) = neighbour;
	}

	return ret;
}

const BattleHexArray::NeighbouringTilesCache BattleHexArray::neighbouringTilesCache = calculateNeighbouringTiles();

VCMI_LIB_NAMESPACE_END