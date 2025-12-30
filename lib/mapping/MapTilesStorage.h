/*
 * MapTilesStorage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../int3.h"

VCMI_LIB_NAMESPACE_BEGIN

	template<typename DataType>
class MapTilesStorage
{
	using TInternalContainer = std::vector<DataType>;
	TInternalContainer storage;
	int3 dimensions = {0,0,0};

	size_t getTileIndex(const int3 & tile) const
	{
		int32_t result = 0;
		result += tile.z * dimensions.x * dimensions.y;
		result += tile.x * dimensions.y;
		result += tile.y;

		assert(tile.x >= 0 && tile.x < dimensions.x);
		assert(tile.y >= 0 && tile.y < dimensions.y);
		assert(tile.z >= 0 && tile.z < dimensions.z);
		return result;
	}

public:
	using const_reference = typename TInternalContainer::const_reference;
	using value_type = typename TInternalContainer::value_type;

	using const_iterator = typename TInternalContainer::const_iterator;
	using iterator = typename TInternalContainer::iterator;

	MapTilesStorage() = default;
	MapTilesStorage(int3 dimensions) :
		storage(dimensions.x * dimensions.y * dimensions.z),
		dimensions(dimensions)
	{}

	const_iterator begin() const { return storage.begin(); }
	const_iterator end() const { return storage.end(); }
	iterator begin() { return storage.begin(); }
	iterator end() { return storage.end(); }

	const DataType & operator[] (const int3 & tile) const { return storage[getTileIndex(tile)];}
	DataType & operator[](const int3 & tile) { return storage[getTileIndex(tile)];}

	template <typename Handler>
	void serialize(Handler &h)
	{
		uint32_t lengthUnused = 0;
		h & lengthUnused;
		h & dimensions.z;
		h & dimensions.x;
		h & dimensions.y;
		if (!h.saving)
			storage.resize(dimensions.x * dimensions.y * dimensions.z);

		for (auto & element : storage)
			h & element;
	}
};

VCMI_LIB_NAMESPACE_END
