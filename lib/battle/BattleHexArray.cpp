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

BattleHex BattleHexArray::getClosestTile(BattleSide side, BattleHex initialPos) const
{
	if(this->empty())
		return BattleHex();

	BattleHex initialHex = BattleHex(initialPos);
	int closestDistance = std::numeric_limits<int>::max();
	BattleHexArray closestTiles;

	for(auto hex : internalStorage)
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

BattleHexArray::ArrayOfBattleHexArrays BattleHexArray::calculateNeighbouringTiles()
{
	BattleHexArray::ArrayOfBattleHexArrays ret;

	for(si16 hex = 0; hex < GameConstants::BFIELD_SIZE; hex++)
	{
		BattleHexArray hexes = BattleHexArray::generateNeighbouringTiles(hex);

		size_t index = 0;
		ret[hex].resize(hexes.size());
		for(auto neighbour : hexes)
			ret[hex].set(index++, neighbour);
	}

	return ret;
}

BattleHexArray::ArrayOfBattleHexArrays BattleHexArray::calculateNeighbouringTilesDblWide(BattleSide side)
{
	ArrayOfBattleHexArrays ret;

	for(BattleHex hex = 0; hex < GameConstants::BFIELD_SIZE; hex.hex++)
	{
		BattleHexArray hexes;

		if(side == BattleSide::ATTACKER)
		{
			const BattleHex otherHex = hex - 1;

			for(auto dir = static_cast<BattleHex::EDir>(0); dir <= static_cast<BattleHex::EDir>(4); dir = static_cast<BattleHex::EDir>(dir + 1))
				hexes.checkAndPush(hex.cloneInDirection(dir, false));

			hexes.checkAndPush(otherHex.cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false));
			hexes.checkAndPush(otherHex.cloneInDirection(BattleHex::EDir::LEFT, false));
			hexes.checkAndPush(otherHex.cloneInDirection(BattleHex::EDir::TOP_LEFT, false));
		}
		else if(side == BattleSide::DEFENDER)
		{
			const BattleHex otherHex = hex + 1;

			hexes.checkAndPush(hex.cloneInDirection(BattleHex::EDir::TOP_LEFT, false));

			for(auto dir = static_cast<BattleHex::EDir>(0); dir <= static_cast<BattleHex::EDir>(4); dir = static_cast<BattleHex::EDir>(dir + 1))
				hexes.checkAndPush(otherHex.cloneInDirection(dir, false));

			hexes.checkAndPush(hex.cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false));
			hexes.checkAndPush(hex.cloneInDirection(BattleHex::EDir::LEFT, false));
		}
		ret[hex.hex] = std::move(hexes);
	}

	return ret;
}

BattleHexArray BattleHexArray::generateNeighbouringTiles(BattleHex hex)
{
	BattleHexArray ret;
	for(auto dir : BattleHex::hexagonalDirections())
		ret.checkAndPush(hex.cloneInDirection(dir, false));
	
	return ret;
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

const BattleHexArray::ArrayOfBattleHexArrays BattleHexArray::neighbouringTilesCache = calculateNeighbouringTiles();
const std::map<BattleSide, BattleHexArray::ArrayOfBattleHexArrays> BattleHexArray::neighbouringTilesDblWide = 
	{ { BattleSide::ATTACKER, calculateNeighbouringTilesDblWide(BattleSide::ATTACKER) },
	{ BattleSide::DEFENDER, calculateNeighbouringTilesDblWide(BattleSide::DEFENDER) } };

VCMI_LIB_NAMESPACE_END