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
#include <boost/container/small_vector.hpp>

VCMI_LIB_NAMESPACE_BEGIN

/// Class representing an array of unique BattleHex objects
class DLL_LINKAGE BattleHexArray
{
public:
	static constexpr uint8_t totalSize = GameConstants::BFIELD_SIZE;
	using StorageType = boost::container::small_vector<BattleHex, 8>;
	using ArrayOfBattleHexArrays = std::array<BattleHexArray, totalSize>;

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

	BattleHexArray() = default;

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
		if(contains(hex))
			return;

		presenceFlags[hex] = 1;
		internalStorage.emplace_back(hex);
	}

	void set(size_type index, BattleHex hex)
	{
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

	iterator insert(iterator pos, BattleHex hex) noexcept
	{
		if(contains(hex))
			return pos;

		presenceFlags[hex] = 1;
		return internalStorage.insert(pos, hex);
	}

	void insert(const BattleHexArray & other) noexcept;

	template <typename Container, typename = std::enable_if_t<
		std::is_convertible_v<typename Container::value_type, BattleHex>>>
		void insert(const Container & container) noexcept
	{
		for(auto value : container)
		{
			insert(value);
		}
	}

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
		return std::vector<BattleHex>(internalStorage.begin(), internalStorage.end());
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

	/// get (precomputed) all possible surrounding tiles
	static const BattleHexArray & getAllNeighbouringTiles(BattleHex hex)
	{
		assert(hex.isValid());

		return allNeighbouringTiles[hex];
	}

	/// get (precomputed) only valid and available surrounding tiles
	static const BattleHexArray & getNeighbouringTiles(BattleHex hex)
	{
		assert(hex.isValid());

		return neighbouringTiles[hex];
	}

	/// get (precomputed) only valid and available surrounding tiles for double wide creatures
	static const BattleHexArray & getNeighbouringTilesDblWide(BattleHex hex, BattleSide side)
	{
		assert(hex.isValid() && (side == BattleSide::ATTACKER || side == BattleSide::DEFENDER));

		return neighbouringTilesDblWide.at(side)[hex];
	}

	[[nodiscard]] inline bool contains(BattleHex hex) const noexcept
	{
		if(hex.isValid())
			return presenceFlags[hex];
		/*
		if(!isTower(hex))
			logGlobal->warn("BattleHexArray::contains( %d ) - invalid BattleHex!", hex);
		*/
		
		// returns true also for invalid hexes
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

	[[nodiscard]] inline const BattleHex & back() const noexcept
	{
		return internalStorage.back();
	}

	[[nodiscard]] inline const BattleHex & front() const noexcept
	{
		return internalStorage.front();
	}

	[[nodiscard]] inline const BattleHex & operator[](size_type index) const noexcept
	{
		return internalStorage[index];
	}

	[[nodiscard]] inline const BattleHex & at(size_type index) const
	{
		return internalStorage.at(index);
	}

	[[nodiscard]] inline size_type size() const noexcept
	{
		return internalStorage.size();
	}

	[[nodiscard]] inline iterator begin() noexcept
	{
		return internalStorage.begin();
	}

	[[nodiscard]] inline const_iterator begin() const noexcept
	{
		return internalStorage.begin();
	}

	[[nodiscard]] inline bool empty() const noexcept
	{
		return internalStorage.empty();
	}

	[[nodiscard]] inline iterator end() noexcept
	{
		return internalStorage.end();
	}

	[[nodiscard]] inline const_iterator end() const noexcept
	{
		return internalStorage.end();
	}

	[[nodiscard]] inline reverse_iterator rbegin() noexcept
	{
		return reverse_iterator(end());
	}

	[[nodiscard]] inline const_reverse_iterator rbegin() const noexcept
	{
		return const_reverse_iterator(end());
	}

	[[nodiscard]] inline reverse_iterator rend() noexcept
	{
		return reverse_iterator(begin());
	}

	[[nodiscard]] inline const_reverse_iterator rend() const noexcept
	{
		return const_reverse_iterator(begin());
	}

	bool operator ==(const BattleHexArray & other) const noexcept
	{
		if(internalStorage != other.internalStorage || presenceFlags != other.presenceFlags)
			return false;

		return true;
	}

private:
	StorageType internalStorage;
	std::bitset<totalSize> presenceFlags;

	[[nodiscard]] inline bool isNotValidForInsertion(BattleHex hex) const
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

	static const ArrayOfBattleHexArrays neighbouringTiles;
	static const ArrayOfBattleHexArrays allNeighbouringTiles;
	static const std::map<BattleSide, ArrayOfBattleHexArrays> neighbouringTilesDblWide;

	static ArrayOfBattleHexArrays precalculateNeighbouringTiles();
	static ArrayOfBattleHexArrays precalculateAllNeighbouringTiles();
	static ArrayOfBattleHexArrays precalculateNeighbouringTilesDblWide(BattleSide side);
};

VCMI_LIB_NAMESPACE_END
