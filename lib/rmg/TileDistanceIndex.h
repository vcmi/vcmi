/*
 * TileDistanceIndex.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"
#include "../int3.h"

#include <set>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace rmg
{
/// VCM-169: tiles of one zone ordered by squared distance to the nearest placed object, farthest first.
class TileDistanceIndex
{
public:
	/// VCM-169: replace the content with `tiles` at the given squared distances.
	void reset(const std::vector<std::pair<int3, float>> & tiles);

	/// VCM-169: lower each indexed tile to its squared 2D distance to the nearest tile of `source`
	/// where that is smaller. Indexed tiles met during the update for which `keep` is false are removed.
	/// Returns the tiles that were lowered, with their new distance.
	template<typename Keep>
	std::vector<std::pair<int3, float>> lowerTowards(const std::vector<int3> & source, const Keep & keep)
	{
		while(!ordered.empty())
		{
			const int3 farthest = ordered.begin()->tile;
			if(keep(farthest))
				break;
			erase(farthest);
		}

		std::vector<std::pair<int3, float>> lowered;
		if(source.empty() || ordered.empty())
			return lowered;

		for(const auto & tile : tilesNear(source, ordered.begin()->distance))
		{
			if(keep(tile))
				lowerTile(tile, source, lowered);
			else
				erase(tile);
		}
		return lowered;
	}

	/// VCM-169: visit tiles farthest first, ties in an order that depends only on tile coordinates;
	/// stop when `visit` returns false.
	template<typename Visit>
	void forEachFarthestFirst(const Visit & visit) const
	{
		for(const auto & key : ordered)
		{
			if(!visit(key.tile, key.distance))
				return;
		}
	}

private:
	struct Key
	{
		float distance = 0.f;
		std::size_t tie = 0;
		int3 tile;

		bool operator<(const Key & other) const;
	};

	struct Box
	{
		int64_t minX;
		int64_t maxX;
		int64_t minY;
		int64_t maxY;
		int64_t minZ;
		int64_t maxZ;
	};

	static std::size_t tieOf(const int3 & tile);
	void erase(const int3 & tile);
	void insert(const int3 & tile, float distance);
	void lowerTile(const int3 & tile, const std::vector<int3> & source, std::vector<std::pair<int3, float>> & lowered);
	/// Indexed tiles within sqrt(maxDistance) of the bounding box of `source`.
	std::vector<int3> tilesNear(const std::vector<int3> & source, float maxDistance) const;
	std::vector<int3> scanIndex(const Box & box) const;
	std::vector<int3> scanBox(const Box & box) const;

	std::set<Key> ordered;
	std::unordered_map<int3, Key> byTile;
};
}
