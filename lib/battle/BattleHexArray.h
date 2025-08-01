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
#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

/**
* @brief Class representing a collection of unique, valid BattleHex objects.

* The BattleHexArray is a specialized container designed for storing instances
* of BattleHex. Key notes:
* - Each BattleHex in the array is unique.
* - Invalid BattleHex objects (e.g., those with an out-of-bounds or special
*   value) cannot be inserted into the array.
* - Maintains an efficient storage mechanism for fast access and presence tracking system using bitset for quick existence checks.
* - Attempting to insert invalid BattleHex objects will have no effect.
*
*/
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
	using const_iterator = typename StorageType::const_iterator;
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

	void checkAndPush(const BattleHex & tile)
	{
		if(tile.isAvailable() && !contains(tile))
		{
			presenceFlags.set(tile.toInt());
			internalStorage.emplace_back(tile);
		}
	}

	void insert(const BattleHex & hex) noexcept
	{
		if(!isValidToInsert(hex))
			return;

		presenceFlags.set(hex.toInt());
		internalStorage.emplace_back(hex);
	}

	void set(size_type index, const BattleHex & hex)
	{
		if(!isValidToInsert(hex))
			return;

		if(index >= internalStorage.size())
		{
			logGlobal->error("Invalid BattleHexArray::set index parameter. It is " + std::to_string(index)
				+ " and current size is " + std::to_string(internalStorage.size()));
			throw std::out_of_range("Invalid BattleHexArray::set index parameter. It is " + std::to_string(index)
				+ " and current size is " + std::to_string(internalStorage.size()));
		}

		presenceFlags.set(hex.toInt());
		internalStorage[index] = hex;
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

	template<typename Predicate>
	void sort(Predicate pred)
	{
		std::sort(internalStorage.begin(), internalStorage.end(), pred);
	}

	template<typename Predicate>
	void eraseIf(Predicate pred)
	{
		vstd::erase_if(internalStorage, pred);
		// reinit presence flags
		presenceFlags = {};
		for(const auto & hex : internalStorage)
			presenceFlags.set(hex.toInt());
	}

	void shuffle(vstd::RNG & rand)
	{
		int64_t n = internalStorage.size();

		for(int64_t i = n - 1; i > 0; --i)
		{
			auto randIndex = rand.nextInt64(0, i);
			std::swap(internalStorage[i], internalStorage[randIndex]);
		}
	}

	void clear() noexcept;
	inline void erase(const BattleHex & target) noexcept
	{
		assert(contains(target));
		vstd::erase(internalStorage, target);
		presenceFlags.reset(target.toInt());
	}

	inline void pop_back() noexcept
	{
		presenceFlags.reset(internalStorage.back().toInt());
		internalStorage.pop_back();
	}

	inline std::vector<BattleHex> toVector() const noexcept
	{
		return std::vector<BattleHex>(internalStorage.begin(), internalStorage.end());
	}

	[[nodiscard]] std::string toString(const std::string & delimiter = ", ") const noexcept
	{
		std::string result = "[";
		for(auto it = internalStorage.begin(); it != internalStorage.end(); ++it)
		{
			if(it != internalStorage.begin())
				result += delimiter;
			result += std::to_string(it->toInt());
		}
		result += "]";

		return result;
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
		for(const auto & hex : internalStorage)
		{
			if(predicate(hex))
			{
				filtered.insert(hex);
			}
		}
		return filtered;
	}

	/// get (precomputed) all possible surrounding tiles
	static const BattleHexArray & getAllNeighbouringTiles(const BattleHex & hex) noexcept
	{
		static const BattleHexArray invalid;

		if(hex.isValid())
			return allNeighbouringTiles[hex.toInt()];
		else
			return invalid;
	}

	/// get (precomputed) only valid and available surrounding tiles
	static const BattleHexArray & getNeighbouringTiles(const BattleHex & hex) noexcept
	{
		static const BattleHexArray invalid;

		if(hex.isValid())
			return neighbouringTiles[hex.toInt()];
		else
			return invalid;
	}

	/// get (precomputed) only valid and available surrounding tiles for double wide creatures
	static const BattleHexArray & getNeighbouringTilesDoubleWide(const BattleHex & hex, BattleSide side) noexcept
	{
		assert(hex.isValid() && (side == BattleSide::ATTACKER || side == BattleSide::DEFENDER));

		return neighbouringTilesDoubleWide.at(side)[hex.toInt()];
	}

	[[nodiscard]] inline bool isValidToInsert(const BattleHex & hex) const noexcept
	{
		if(!hex.isValid())
			return false;

		if(contains(hex))
			return false;

		return true;
	}

	[[nodiscard]] inline bool contains(const BattleHex & hex) const noexcept
	{
		if(hex.isValid())
			return presenceFlags.test(hex.toInt());
		
		return false;
	}

	template <typename Serializer>
	void serialize(Serializer & s)
	{
		s & internalStorage;
		if(!s.saving)
		{
			for(const auto & hex : internalStorage)
				presenceFlags.set(hex.toInt());
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

	[[nodiscard]] inline const_iterator begin() const noexcept
	{
		return internalStorage.begin();
	}

	[[nodiscard]] inline bool empty() const noexcept
	{
		return internalStorage.empty();
	}

	[[nodiscard]] inline const_iterator end() const noexcept
	{
		return internalStorage.end();
	}

	[[nodiscard]] inline const_reverse_iterator rbegin() const noexcept
	{
		return const_reverse_iterator(end());
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

	static const ArrayOfBattleHexArrays neighbouringTiles;
	static const ArrayOfBattleHexArrays allNeighbouringTiles;
	static const std::map<BattleSide, ArrayOfBattleHexArrays> neighbouringTilesDoubleWide;

	static ArrayOfBattleHexArrays precalculateNeighbouringTiles();
	static ArrayOfBattleHexArrays precalculateAllNeighbouringTiles();
	static ArrayOfBattleHexArrays precalculateNeighbouringTilesDoubleWide(BattleSide side);
};

VCMI_LIB_NAMESPACE_END
