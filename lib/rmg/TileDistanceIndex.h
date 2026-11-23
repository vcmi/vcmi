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

#include <functional>
#include <set>
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
	std::vector<std::pair<int3, float>> lowerTowards(const std::vector<int3> & source, const std::function<bool(const int3 &)> & keep);

	/// VCM-169: visit tiles farthest first, ties in an order that depends only on tile coordinates;
	/// stop when `visit` returns false.
	void forEachFarthestFirst(const std::function<bool(const int3 &, float)> & visit) const;

private:
	struct Key
	{
		float distance = 0.f;
		std::size_t tie = 0;
		int3 tile;

		bool operator<(const Key & other) const;
	};

	static std::size_t tieOf(const int3 & tile);
	void erase(const int3 & tile);
	void insert(const int3 & tile, float distance);

	std::set<Key> ordered;
	std::unordered_map<int3, Key> byTile;
};
}
