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
	// Farthest first, then by coordinate-derived tie, then by tile.
	return std::tie(other.distance, tie, tile) < std::tie(distance, other.tie, other.tile);
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

void TileDistanceIndex::lowerTile(const int3 & tile, const std::vector<int3> & source, std::vector<std::pair<int3, float>> & lowered)
{
	auto it = byTile.find(tile);
	if(it == byTile.end())
		return;

	const float current = it->second.distance;
	float next = current;
	for(const auto & origin : source)
		next = std::min(next, static_cast<float>(tile.dist2dSQ(origin)));

	if(next < current)
	{
		insert(tile, next);
		lowered.emplace_back(tile, next);
	}
}

std::vector<int3> TileDistanceIndex::tilesNear(const std::vector<int3> & source, float maxDistance) const
{
	const double radius = std::ceil(std::sqrt(static_cast<double>(maxDistance)));
	const auto radiusI = static_cast<int64_t>(std::min(radius, static_cast<double>(std::numeric_limits<si32>::max())));

	Box box{source.front().x, source.front().x, source.front().y, source.front().y, source.front().z, source.front().z};
	for(const auto & tile : source)
	{
		box.minX = std::min(box.minX, static_cast<int64_t>(tile.x));
		box.maxX = std::max(box.maxX, static_cast<int64_t>(tile.x));
		box.minY = std::min(box.minY, static_cast<int64_t>(tile.y));
		box.maxY = std::max(box.maxY, static_cast<int64_t>(tile.y));
		box.minZ = std::min(box.minZ, static_cast<int64_t>(tile.z));
		box.maxZ = std::max(box.maxZ, static_cast<int64_t>(tile.z));
	}
	box.minX -= radiusI;
	box.maxX += radiusI;
	box.minY -= radiusI;
	box.maxY += radiusI;

	const int64_t boxW = box.maxX - box.minX + 1;
	const int64_t boxH = box.maxY - box.minY + 1;
	const int64_t boxD = box.maxZ - box.minZ + 1;
	const auto indexSize = static_cast<int64_t>(byTile.size());
	const bool boxLargerThanIndex = boxW <= 0 || boxH <= 0 || boxD <= 0
		|| boxW > indexSize || boxH > indexSize || boxD > indexSize
		|| boxW * boxH * boxD > indexSize;

	return boxLargerThanIndex ? scanIndex(box) : scanBox(box);
}

std::vector<int3> TileDistanceIndex::scanIndex(const Box & box) const
{
	std::vector<int3> candidates;
	candidates.reserve(byTile.size());
	for(const auto & entry : byTile)
	{
		const int3 & tile = entry.first;
		if(tile.x >= box.minX && tile.x <= box.maxX && tile.y >= box.minY && tile.y <= box.maxY && tile.z >= box.minZ && tile.z <= box.maxZ)
			candidates.push_back(tile);
	}
	return candidates;
}

std::vector<int3> TileDistanceIndex::scanBox(const Box & box) const
{
	std::vector<int3> candidates;
	for(int64_t z = box.minZ; z <= box.maxZ; ++z)
	{
		for(int64_t y = box.minY; y <= box.maxY; ++y)
		{
			for(int64_t x = box.minX; x <= box.maxX; ++x)
			{
				const int3 tile(static_cast<si32>(x), static_cast<si32>(y), static_cast<si32>(z));
				if(byTile.contains(tile))
					candidates.push_back(tile);
			}
		}
	}
	return candidates;
}

}
