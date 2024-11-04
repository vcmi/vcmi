/*
 * BattleHexArray.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "BattleHex.h"

VCMI_LIB_NAMESPACE_BEGIN

/// Class representing an array of unique BattleHex objects
class DLL_LINKAGE BattleHexArray
{
public:
	static constexpr uint8_t totalSize = GameConstants::BFIELD_SIZE;
	using StorageType = std::vector<BattleHex>;

	using value_type = BattleHex;
	using size_type = StorageType::size_type;
	using reference = value_type &;
	using const_reference = const value_type &;
	using pointer = value_type *;
	using const_pointer = const value_type *;
	using difference_type = typename StorageType::difference_type;
	using iterator = typename StorageType::iterator;
	using const_iterator = typename StorageType::const_iterator;
	using reverse_iterator = typename StorageType::reverse_iterator;
	using const_reverse_iterator = typename StorageType::const_reverse_iterator;

	using NeighbouringTilesCache = std::array<BattleHexArray, GameConstants::BFIELD_SIZE>;

	static const NeighbouringTilesCache neighbouringTilesCache;
	static const BattleHexArray closestTilesCacheForAttacker;
	static const BattleHexArray closestTilesCacheForDefender;

	BattleHexArray() noexcept
	{
		internalStorage.reserve(totalSize);
	}

	template <typename Container, typename = std::enable_if_t<
		std::is_convertible_v<typename Container::value_type, BattleHex>>>
		BattleHexArray(const Container & container) noexcept
			: BattleHexArray()
	{
		for(auto value : container)
		{
			insert(value); 
		}
	}

	void resize(size_type size)
	{
		clear();
		internalStorage.resize(size);
	}
	
	BattleHexArray(std::initializer_list<BattleHex> initList) noexcept;

	/// returns all tiles, unavailable tiles will be set as invalid
	/// order of returned tiles matches EDir enum
	static BattleHexArray generateAllNeighbouringTiles(BattleHex hex)
	{
		BattleHexArray ret;

		ret.resize(6);

		for(auto dir : BattleHex::hexagonalDirections())
			ret.set(dir, hex.cloneInDirection(dir, false));

		return ret;
	}

	void checkAndPush(BattleHex tile)
	{
		if(tile.isAvailable() && !contains(tile))
		{
			presenceFlags[tile] = 1;
			internalStorage.emplace_back(tile);
		}
	}

	void insert(BattleHex hex) noexcept
	{
		/*if(isNotValidForInsertion(hex))
			return;*/

		if(contains(hex))
			return;

		presenceFlags[hex] = 1;
		internalStorage.emplace_back(hex);
	}

	void set(size_type index, BattleHex hex)
	{
		/*if(isNotValidForInsertion(hex))
			return;*/

		if(index >= internalStorage.size())
		{
			logGlobal->error("Invalid BattleHexArray::set index parameter. It is " + std::to_string(index)
				+ " and current size is " + std::to_string(internalStorage.size()));
			throw std::out_of_range("Invalid BattleHexArray::set index parameter. It is " + std::to_string(index)
				+ " and current size is " + std::to_string(internalStorage.size()));
		}

		if(contains(hex))
			return;

		presenceFlags[hex] = 1;
		internalStorage[index] = hex;
	}

	iterator BattleHexArray::insert(iterator pos, BattleHex hex) noexcept
	{
		/*if(isNotValidForInsertion(hex))
			return pos;*/

		if(contains(hex))
			return pos;

		presenceFlags[hex] = 1;
		return internalStorage.insert(pos, hex);
	}

	static BattleHex getClosestTileFromAllPossibleNeighbours(BattleSide side, BattleHex pos);

	BattleHex getClosestTile(BattleSide side, BattleHex initialPos) const;

	void merge(const BattleHexArray & other) noexcept;

	void clear() noexcept;
	inline void erase(size_type index) noexcept
	{
		assert(index < totalSize);
		internalStorage[index] = BattleHex::INVALID;
		presenceFlags[index] = 0;
	}
	void erase(iterator first, iterator last) noexcept;
	inline void pop_back() noexcept
	{
		presenceFlags[internalStorage.back()] = 0;
		internalStorage.pop_back();
	}

	inline std::vector<BattleHex> toVector() const noexcept
	{
		return internalStorage;
	}

	template <typename Predicate>
	iterator findIf(Predicate predicate) noexcept
	{
		return std::find_if(begin(), end(), predicate);
	}

	template <typename Predicate>
	const_iterator findIf(Predicate predicate) const noexcept
	{
		return std::find_if(begin(), end(), predicate);
	}

	template <typename Predicate>
	BattleHexArray filterBy(Predicate predicate) const noexcept
	{
		BattleHexArray filtered;
		for(auto hex : internalStorage)
		{
			if(predicate(hex))
			{
				filtered.insert(hex);
			}
		}
		return filtered;
	}

	[[nodiscard]] inline bool BattleHexArray::contains(BattleHex hex) const noexcept
	{
		if(hex.isValid())
			return presenceFlags[hex];
		/*
		if(!isTower(hex))
			logGlobal->warn("BattleHexArray::contains( %d ) - invalid BattleHex!", hex);
		*/
		
		// return true for invalid hexes
		return true;
	}

	template <typename Serializer>
	void serialize(Serializer & s)
	{
		s & internalStorage;
		if(!internalStorage.empty() && presenceFlags[internalStorage.front()] == 0)
		{
			for(auto hex : internalStorage)
				presenceFlags[hex] = 1;
		}
	}

	[[nodiscard]] inline const BattleHex & BattleHexArray::back() const noexcept
	{
		return internalStorage.back();
	}

	[[nodiscard]] inline const BattleHex & BattleHexArray::front() const noexcept
	{
		return internalStorage.front();
	}

	[[nodiscard]] inline const BattleHex & BattleHexArray::operator[](size_type index) const noexcept
	{
		return internalStorage[index];
	}

	[[nodiscard]] inline const BattleHex & BattleHexArray::at(size_type index) const
	{
		return internalStorage.at(index);
	}

	[[nodiscard]] inline size_type size() const noexcept
	{
		return internalStorage.size();
	}

	[[nodiscard]] inline BattleHexArray::iterator BattleHexArray::begin() noexcept
	{
		return internalStorage.begin();
	}

	[[nodiscard]] inline BattleHexArray::const_iterator BattleHexArray::begin() const noexcept
	{
		return internalStorage.begin();
	}

	[[nodiscard]] inline bool BattleHexArray::empty() const noexcept
	{
		return internalStorage.empty();
	}

	[[nodiscard]] inline BattleHexArray::iterator BattleHexArray::end() noexcept
	{
		return internalStorage.end();
	}

	[[nodiscard]] inline BattleHexArray::const_iterator BattleHexArray::end() const noexcept
	{
		return internalStorage.end();
	}

	[[nodiscard]] inline BattleHexArray::reverse_iterator BattleHexArray::rbegin() noexcept
	{
		return reverse_iterator(end());
	}

	[[nodiscard]] inline BattleHexArray::const_reverse_iterator BattleHexArray::rbegin() const noexcept
	{
		return const_reverse_iterator(end());
	}

	[[nodiscard]] inline BattleHexArray::reverse_iterator BattleHexArray::rend() noexcept
	{
		return reverse_iterator(begin());
	}

	[[nodiscard]] inline BattleHexArray::const_reverse_iterator BattleHexArray::rend() const noexcept
	{
		return const_reverse_iterator(begin());
	}

private:
	StorageType internalStorage;
	std::array<uint8_t, totalSize> presenceFlags = {};

	[[nodiscard]] inline bool BattleHexArray::isNotValidForInsertion(BattleHex hex) const
	{
		if(isTower(hex))
			return true;
		if(!hex.isValid())
		{
			//logGlobal->warn("BattleHexArray::insert( %d ) - invalid BattleHex!", hex);
			return true;
		}

		return contains(hex) || internalStorage.size() >= totalSize;
	}

	[[nodiscard]] inline bool isTower(BattleHex hex) const
	{
		return hex == BattleHex::CASTLE_CENTRAL_TOWER || hex == BattleHex::CASTLE_UPPER_TOWER || hex == BattleHex::CASTLE_BOTTOM_TOWER;
	}

	/// returns all valid neighbouring tiles
	static BattleHexArray::NeighbouringTilesCache calculateNeighbouringTiles();
	static BattleHexArray generateNeighbouringTiles(BattleHex hex);
	static BattleHexArray generateAttackerClosestTilesCache();
	static BattleHexArray generateDefenderClosestTilesCache();
};

VCMI_LIB_NAMESPACE_END
