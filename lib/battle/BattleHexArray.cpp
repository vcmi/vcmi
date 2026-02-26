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
	for(const auto & hex : initList)
	{
		insert(hex);
	}
}

void BattleHexArray::insert(const BattleHexArray & other) noexcept
{
	for(const auto & hex : other)
	{
		insert(hex);
	}
}

void BattleHexArray::clear() noexcept
{
	presenceFlags = {};
	internalStorage.clear();
}

BattleHexArray::ArrayOfBattleHexArrays BattleHexArray::precalculateNeighbouringTiles()
{
	BattleHexArray::ArrayOfBattleHexArrays ret;

	for(si16 hex = 0; hex < GameConstants::BFIELD_SIZE; hex++)
	{
		BattleHexArray hexes;

		for(auto dir : BattleHex::hexagonalDirections())
			hexes.checkAndPush(BattleHex(hex).cloneInDirection(dir, false));

		size_t index = 0;
		ret[hex].resize(hexes.size());
		for(auto neighbour : hexes)
			ret[hex].set(index++, neighbour);
	}

	return ret;
}

BattleHexArray::ArrayOfBattleHexArrays BattleHexArray::precalculateAllNeighbouringTiles()
{
	ArrayOfBattleHexArrays ret;

	for(si16 hex = 0; hex < GameConstants::BFIELD_SIZE; hex++)
	{
		ret[hex].resize(6);

		for(auto dir : BattleHex::hexagonalDirections())
			ret[hex].set(dir, BattleHex(hex).cloneInDirection(dir, false));
	}

	return ret;
}

BattleHexArray::ArrayOfBattleHexArrays BattleHexArray::precalculateNeighbouringTilesDoubleWide(BattleSide side)
{
	ArrayOfBattleHexArrays ret;

	for(si16 h = 0; h < GameConstants::BFIELD_SIZE; h++)
	{
		BattleHexArray hexes;
		BattleHex hex(h);

		if(side == BattleSide::ATTACKER)
		{
			const BattleHex otherHex = h - 1;

			for(auto dir = static_cast<BattleHex::EDir>(0); dir <= static_cast<BattleHex::EDir>(4); dir = static_cast<BattleHex::EDir>(dir + 1))
				hexes.checkAndPush(hex.cloneInDirection(dir, false));

			hexes.checkAndPush(otherHex.cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false));
			hexes.checkAndPush(otherHex.cloneInDirection(BattleHex::EDir::LEFT, false));
			hexes.checkAndPush(otherHex.cloneInDirection(BattleHex::EDir::TOP_LEFT, false));
		}
		else if(side == BattleSide::DEFENDER)
		{
			const BattleHex otherHex = h + 1;

			hexes.checkAndPush(hex.cloneInDirection(BattleHex::EDir::TOP_LEFT, false));

			for(auto dir = static_cast<BattleHex::EDir>(0); dir <= static_cast<BattleHex::EDir>(4); dir = static_cast<BattleHex::EDir>(dir + 1))
				hexes.checkAndPush(otherHex.cloneInDirection(dir, false));

			hexes.checkAndPush(hex.cloneInDirection(BattleHex::EDir::BOTTOM_LEFT, false));
			hexes.checkAndPush(hex.cloneInDirection(BattleHex::EDir::LEFT, false));
		}
		ret[h] = std::move(hexes);
	}

	return ret;
}

const BattleHexArray::ArrayOfBattleHexArrays BattleHexArray::neighbouringTiles = precalculateNeighbouringTiles();
const BattleHexArray::ArrayOfBattleHexArrays BattleHexArray::allNeighbouringTiles = precalculateAllNeighbouringTiles();
const std::map<BattleSide, BattleHexArray::ArrayOfBattleHexArrays> BattleHexArray::neighbouringTilesDoubleWide =
	{
	   { BattleSide::ATTACKER, precalculateNeighbouringTilesDoubleWide(BattleSide::ATTACKER) },
	   { BattleSide::DEFENDER, precalculateNeighbouringTilesDoubleWide(BattleSide::DEFENDER) }
	};

VCMI_LIB_NAMESPACE_END
