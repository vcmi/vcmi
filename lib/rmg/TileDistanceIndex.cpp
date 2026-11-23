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

#include <cmath>
#include <limits>

namespace rmg
{

bool TileDistanceIndex::Key::operator<(const Key & other) const
{
	if(distance != other.distance)
		return distance > other.distance;
	if(tie != other.tie)
		return tie < other.tie;
	return tile < other.tile;
}

std::size_t TileDistanceIndex::tieOf(const int3 & tile)
{
	std::size_t seed = std::hash<si32>{}(tile.x);
	vstd::hash_combine(seed, tile.y);
	vstd::hash_combine(seed, tile.z);
	return seed;
}

void TileDistanceIndex::erase(const int3 & tile)
{
	auto it = byTile.find(tile);
	if(it == byTile.end())
		return;
	ordered.erase(it->second);
	byTile.erase(it);
}

void TileDistanceIndex::insert(const int3 & tile, float distance)
{
	erase(tile);
	Key key{distance, tieOf(tile), tile};
	ordered.insert(key);
	byTile.insert_or_assign(tile, key);
}

void TileDistanceIndex::reset(const std::vector<std::pair<int3, float>> & tiles)
{
	ordered.clear();
	byTile.clear();
	byTile.reserve(tiles.size());
	for(const auto & [tile, distance] : tiles)
		insert(tile, distance);
}

std::vector<std::pair<int3, float>> TileDistanceIndex::lowerTowards(const std::vector<int3> & source, const std::function<bool(const int3 &)> & keep)
{
	while(!ordered.empty())
	{
		const int3 tile = ordered.begin()->tile;
		if(keep(tile))
			break;
		erase(tile);
	}

	if(source.empty() || ordered.empty())
		return {};

	const float maxDist = ordered.begin()->distance;
	const double radius = std::ceil(std::sqrt(static_cast<double>(maxDist)));
	const auto radiusI = static_cast<int64_t>(std::min(radius, static_cast<double>(std::numeric_limits<si32>::max())));

	int64_t minX = source.front().x;
	int64_t maxX = source.front().x;
	int64_t minY = source.front().y;
	int64_t maxY = source.front().y;
	int64_t minZ = source.front().z;
	int64_t maxZ = source.front().z;
	for(const auto & tile : source)
	{
		minX = std::min(minX, static_cast<int64_t>(tile.x));
		maxX = std::max(maxX, static_cast<int64_t>(tile.x));
		minY = std::min(minY, static_cast<int64_t>(tile.y));
		maxY = std::max(maxY, static_cast<int64_t>(tile.y));
		minZ = std::min(minZ, static_cast<int64_t>(tile.z));
		maxZ = std::max(maxZ, static_cast<int64_t>(tile.z));
	}
	minX -= radiusI;
	maxX += radiusI;
	minY -= radiusI;
	maxY += radiusI;

	const int64_t boxW = maxX - minX + 1;
	const int64_t boxH = maxY - minY + 1;
	const int64_t boxD = maxZ - minZ + 1;
	const auto indexSize = static_cast<int64_t>(byTile.size());
	const bool scanIndex = boxW <= 0 || boxH <= 0 || boxD <= 0
		|| boxW > indexSize || boxH > indexSize || boxD > indexSize
		|| boxW * boxH * boxD > indexSize;

	std::vector<int3> candidates;
	if(scanIndex)
	{
		candidates.reserve(byTile.size());
		for(const auto & [tile, key] : byTile)
		{
			if(tile.x >= minX && tile.x <= maxX && tile.y >= minY && tile.y <= maxY && tile.z >= minZ && tile.z <= maxZ)
				candidates.push_back(tile);
		}
	}
	else
	{
		for(int64_t z = minZ; z <= maxZ; ++z)
		{
			for(int64_t y = minY; y <= maxY; ++y)
			{
				for(int64_t x = minX; x <= maxX; ++x)
				{
					const int3 tile(static_cast<si32>(x), static_cast<si32>(y), static_cast<si32>(z));
					if(byTile.contains(tile))
						candidates.push_back(tile);
				}
			}
		}
	}

	std::vector<std::pair<int3, float>> lowered;
	for(const auto & tile : candidates)
	{
		if(!keep(tile))
		{
			erase(tile);
			continue;
		}
		auto it = byTile.find(tile);
		if(it == byTile.end())
			continue;

		float next = it->second.distance;
		for(const auto & origin : source)
			next = std::min(next, static_cast<float>(tile.dist2dSQ(origin)));

		if(next < it->second.distance)
		{
			insert(tile, next);
			lowered.emplace_back(tile, next);
		}
	}
	return lowered;
}

void TileDistanceIndex::forEachFarthestFirst(const std::function<bool(const int3 &, float)> & visit) const
{
	for(const auto & key : ordered)
	{
		if(!visit(key.tile, key.distance))
			return;
	}
}

}
