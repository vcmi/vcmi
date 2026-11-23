/*
 * TileDistanceIndex.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "TileDistanceIndex.h"

namespace rmg
{

void TileDistanceIndex::reset(const std::vector<std::pair<int3, float>> & tiles)
{
}

std::vector<std::pair<int3, float>> TileDistanceIndex::lowerTowards(const std::vector<int3> & source, const std::function<bool(const int3 &)> & keep)
{
	return {};
}

void TileDistanceIndex::forEachFarthestFirst(const std::function<bool(const int3 &, float)> & visit) const
{
}

}
