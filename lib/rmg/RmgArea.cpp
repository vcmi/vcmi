/*
 * RmgArea.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "RmgArea.h"
#include "CMapGenerator.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace rmg
{
namespace
{
constexpr size_t kIncrementalSortedCacheLimit = 256;
}


void toAbsolute(Tileset & tiles, const int3 & position)
{
	std::vector vec(tiles.begin(), tiles.end());
	tiles.clear();
	std::transform(vec.begin(), vec.end(), vstd::set_inserter(tiles), [position](const int3 & tile)
	{
		return tile + position;
	});
}

void toRelative(Tileset & tiles, const int3 & position)
{
	toAbsolute(tiles, -position);
}

Area::Area(const Area & area): dTiles(area.dTiles), dTotalShiftCache(area.dTotalShiftCache)
{
}

Area::Area(Area && area) noexcept: dTiles(std::move(area.dTiles)), dTotalShiftCache(area.dTotalShiftCache)
{
}

Area & Area::operator=(const Area & area)
{
	clear();
	dTiles = area.dTiles;
	dTotalShiftCache = area.dTotalShiftCache;
	return *this;
}

Area::Area(Tileset tiles): dTiles(std::move(tiles))
{
}

Area::Area(Tileset relative, const int3 & position): dTiles(std::move(relative)), dTotalShiftCache(position)
{
}

void Area::invalidate()
{
	getTiles();
	dTilesVectorCache.clear();
	dTilesVectorShiftCache = int3();
	dBorderCacheValid = false;
	dBorderOutsideCacheValid = false;
}

bool Area::connected(bool noDiagonals) const
{
	std::list<int3> queue({*std::min_element(dTiles.begin(), dTiles.end())});
	Tileset connected = dTiles; //use invalidated cache - ok

	while(!queue.empty())
	{
		auto t = queue.front();
		connected.erase(t);
		queue.pop_front();
		
		if (noDiagonals)
		{
			for (auto& i : dirs4)
			{
				if (connected.count(t + i))
				{
					queue.push_back(t + i);
				}
			}
		}
		else
		{
			for (auto& i : int3::getDirs())
			{
				if (connected.count(t + i))
				{
					queue.push_back(t + i);
				}
			}
		}
	}
	
	return connected.empty();
}

std::list<Area> connectedAreas(const Area & area, bool disableDiagonalConnections)
{
	auto allDirs = int3::getDirs();
	std::vector<int3> dirs(allDirs.begin(), allDirs.end());
	if(disableDiagonalConnections)
		dirs.assign(rmg::dirs4.begin(), rmg::dirs4.end());
	
	std::list<Area> result;
	Tileset connected = area.getTiles();
	while(!connected.empty())
	{
		auto first = *std::min_element(connected.begin(), connected.end());
		result.emplace_back();
		std::list<int3> queue({first});
		std::set<int3> queueSet({first});
		while(!queue.empty())
		{
			auto t = queue.front();
			connected.erase(t);
			result.back().add(t);
			queue.pop_front();
			
			for(auto & i : dirs)
			{
				auto tile = t + i;
				if(!queueSet.count(tile) && connected.count(tile) && !result.back().contains(tile))
				{
					queueSet.insert(tile);
					queue.push_back(tile);
				}
			}
		}
	}
	return result;
}

const Tileset & Area::getTiles() const
{
	if(dTotalShiftCache != int3())
	{
		toAbsolute(dTiles, dTotalShiftCache);
		dTotalShiftCache = int3();
	}
	return dTiles;
}

const std::vector<int3> & Area::getTilesVector() const
{
	if(dTilesVectorCache.empty())
	{
		getTiles();
		dTilesVectorCache.assign(dTiles.begin(), dTiles.end());
		std::sort(dTilesVectorCache.begin(), dTilesVectorCache.end());
		dTilesVectorShiftCache = int3();
	}
	else if(dTilesVectorShiftCache != int3())
	{
		for(auto & t : dTilesVectorCache)
		{
			t += dTilesVectorShiftCache;
		}
		dTilesVectorShiftCache = int3();
	}
	return dTilesVectorCache;
}

const Tileset & Area::getBorder() const
{
	if(dBorderCacheValid)
		return dBorderCache;
	
	//compute border cache
	dBorderCache.clear();
	dBorderCache.reserve(dTiles.bucket_count());
	for(const auto & t : dTiles)
	{
		for(auto & i : int3::getDirs())
		{
			if(!dTiles.count(t + i))
			{
				dBorderCache.insert(t + dTotalShiftCache);
				break;
			}
		}
	}
	dBorderCacheValid = true;
	dBorderOutsideCacheValid = false;
	
	return dBorderCache;
}

const Tileset & Area::getBorderOutside() const
{
	if(dBorderOutsideCacheValid)
		return dBorderOutsideCache;
	
	//compute outside border cache
	dBorderOutsideCache.clear();
	dBorderOutsideCache.reserve(dBorderCache.bucket_count() * 2);
	for(const auto & t : dTiles)
	{
		for(auto & i : int3::getDirs())
		{
			if(!dTiles.count(t + i))
				dBorderOutsideCache.insert(t + i + dTotalShiftCache);
		}
	}
	dBorderOutsideCacheValid = true;
	
	return dBorderOutsideCache;
}

DistanceMap Area::computeDistanceMap(std::map<int, Tileset> & reverseDistanceMap) const
{
	reverseDistanceMap.clear();
	DistanceMap result;
	auto area = *this;
	int distance = 0;
	
	while(!area.empty())
	{
		for(const auto & tile : area.getBorder())
			result[tile] = distance;
		reverseDistanceMap[distance++] = area.getBorder();
		area.subtract(area.getBorder());
	}
	return result;
}

int3 Area::getCenterOfMass() const
{
	auto tiles = getTilesVector();
	int3 total(0, 0, 0);
	for(const auto & tile : tiles)
	{
		total += tile;
	}
	int size = static_cast<int>(tiles.size());
	assert(size);
	return int3(total.x / size, total.y / size, total.z / size);
}

bool Area::empty() const
{
	return dTiles.empty();
}

bool Area::contains(const int3 & tile) const
{
	return dTiles.count(tile - dTotalShiftCache);
}

bool Area::contains(const std::vector<int3> & tiles) const
{
	for(const auto & t : tiles)
	{
		if(!contains(t))
			return false;
	}
	return true;
}

bool Area::contains(const Area & area) const
{
	for(const auto & t : area.getTiles())
	{
		if(!contains(t))
			return false;
	}
	return true;
}

bool Area::overlap(const std::vector<int3> & tiles) const
{
	// Important: Make sure that tiles.size < area.size
	for(const auto & t : tiles)
	{
		if(contains(t))
			return true;
	}
	return false;
}

bool Area::overlap(const Area & area) const
{
	for(const auto & t : area.getTiles())
	{
		if(contains(t))
			return true;
	}
	return false;
}

int Area::distance(const int3 & tile) const
{
	return nearest(tile).dist2d(tile);
}

int Area::distanceSqr(const int3 & tile) const
{
	return nearest(tile).dist2dSQ(tile);
}

int Area::distanceSqr(const Area & area) const
{
	int dist = std::numeric_limits<int>::max();
	int3 nearTile = *getTilesVector().begin();
	int3 otherNearTile = area.nearest(nearTile);
	
	while(dist != otherNearTile.dist2dSQ(nearTile))
	{
		dist = otherNearTile.dist2dSQ(nearTile);
		nearTile = nearest(otherNearTile);
		otherNearTile = area.nearest(nearTile);
	}
	
	return dist;
}

int3 Area::nearest(const int3 & tile) const
{
	return findClosestTile(getTilesVector(), tile);
}

int3 Area::nearest(const Area & area) const
{
	int dist = std::numeric_limits<int>::max();
	int3 nearTile = *getTilesVector().begin();
	int3 otherNearTile = area.nearest(nearTile);
	
	while(dist != otherNearTile.dist2dSQ(nearTile))
	{
		dist = otherNearTile.dist2dSQ(nearTile);
		nearTile = nearest(otherNearTile);
		otherNearTile = area.nearest(nearTile);
	}
	
	return nearTile;
}

Area Area::getSubarea(const std::function<bool(const int3 &)> & filter) const
{
	Area subset;
	const auto & tiles = getTiles();
	subset.dTiles.reserve(tiles.size());
	vstd::copy_if(tiles, vstd::set_inserter(subset.dTiles), filter);
	return subset;
}

void Area::clear()
{
	dTiles.clear();
	dTilesVectorCache.clear();
	dTilesVectorShiftCache = int3();
	dBorderCache.clear();
	dBorderCacheValid = false;
	dBorderOutsideCache.clear();
	dBorderOutsideCacheValid = false;
	dTotalShiftCache = int3();
}

void Area::assign(const Tileset tiles)
{
	clear();
	dTiles = tiles;
}

void Area::reserve(size_t capacity)
{
	getTiles();
	dTiles.reserve(capacity);
}

void Area::add(const int3 & tile)
{
	getTiles();
	const auto [it, inserted] = dTiles.insert(tile);
	(void)it;
	if(!inserted)
		return;

	dBorderCacheValid = false;
	dBorderOutsideCacheValid = false;

	if(!dTilesVectorCache.empty())
	{
		if(dTilesVectorShiftCache != int3())
		{
			for(auto & t : dTilesVectorCache)
			{
				t += dTilesVectorShiftCache;
			}
			dTilesVectorShiftCache = int3();
		}
		const auto position = std::lower_bound(dTilesVectorCache.begin(), dTilesVectorCache.end(), tile);
		dTilesVectorCache.insert(position, tile);
	}
}

void Area::erase(const int3 & tile)
{
	getTiles();
	const size_t erased = dTiles.erase(tile);
	if(!erased)
		return;

	dBorderCacheValid = false;
	dBorderOutsideCacheValid = false;

	if(!dTilesVectorCache.empty())
	{
		if(dTilesVectorShiftCache != int3())
		{
			for(auto & t : dTilesVectorCache)
			{
				t += dTilesVectorShiftCache;
			}
			dTilesVectorShiftCache = int3();
		}
		const auto position = std::lower_bound(dTilesVectorCache.begin(), dTilesVectorCache.end(), tile);
		if(position == dTilesVectorCache.end() || *position != tile)
		{
			dTilesVectorCache.clear();
			dTilesVectorShiftCache = int3();
		}
		else
		{
			dTilesVectorCache.erase(position);
		}
	}
}
void Area::unite(const Area & area)
{
	getTiles();
	const auto & tiles = area.getTiles();
	if(tiles.empty())
		return;

	dBorderCacheValid = false;
	dBorderOutsideCacheValid = false;

	// For larger unions, rebuilding the sorted cache once is cheaper than
	// maintaining it tile-by-tile.
	const bool keepSortedCache = !dTilesVectorCache.empty() && tiles.size() <= kIncrementalSortedCacheLimit;
	if(!keepSortedCache)
	{
		dTilesVectorCache.clear();
		dTilesVectorShiftCache = int3();
		dTiles.reserve(dTiles.size() + tiles.size());
		dTiles.insert(tiles.begin(), tiles.end());
		return;
	}
	if(dTilesVectorShiftCache != int3())
	{
		for(auto & t : dTilesVectorCache)
		{
			t += dTilesVectorShiftCache;
		}
		dTilesVectorShiftCache = int3();
	}

	for(const auto & t : tiles)
	{
		const auto [it, inserted] = dTiles.insert(t);
		(void)it;
		if(!inserted)
			continue;

		const auto position = std::lower_bound(dTilesVectorCache.begin(), dTilesVectorCache.end(), t);
		dTilesVectorCache.insert(position, t);
	}
}

void Area::intersect(const Area & area)
{
	invalidate();
	Tileset result;
	const auto & tiles = area.getTiles();
	const Tileset * smaller = &dTiles;
	const Tileset * larger = &tiles;
	if(smaller->size() > larger->size())
		std::swap(smaller, larger);

	result.reserve(smaller->size());
	for(const auto & t : *smaller)
	{
		if(larger->count(t))
			result.insert(t);
	}
	dTiles = result;
}

void Area::subtract(const Area & area)
{
	getTiles();
	const auto & tiles = area.getTiles();
	if(tiles.empty())
		return;

	dBorderCacheValid = false;
	dBorderOutsideCacheValid = false;

	const bool keepSortedCache = !dTilesVectorCache.empty() && tiles.size() <= kIncrementalSortedCacheLimit;
	if(!keepSortedCache)
	{
		dTilesVectorCache.clear();
		dTilesVectorShiftCache = int3();
		for(const auto & t : tiles)
			dTiles.erase(t);
		return;
	}
	if(dTilesVectorShiftCache != int3())
	{
		for(auto & t : dTilesVectorCache)
		{
			t += dTilesVectorShiftCache;
		}
		dTilesVectorShiftCache = int3();
	}

	for(const auto & t : tiles)
	{
		if(!dTiles.erase(t))
			continue;

		const auto position = std::lower_bound(dTilesVectorCache.begin(), dTilesVectorCache.end(), t);
		if(position == dTilesVectorCache.end() || *position != t)
		{
			dTilesVectorCache.clear();
			dTilesVectorShiftCache = int3();
			continue;
		}
		dTilesVectorCache.erase(position);
	}
}

void Area::translate(const int3 & shift)
{
	dBorderCacheValid = false;
	dBorderOutsideCacheValid = false;
	
	//avoid recomputation within std::set, use vector instead
	dTotalShiftCache += shift;

	if(!dTilesVectorCache.empty())
		dTilesVectorShiftCache += shift;
}

void Area::erase_if(std::function<bool(const int3&)> predicate)
{
	invalidate();
	vstd::erase_if(dTiles, predicate);
}

Area operator- (const Area & l, const int3 & r)
{
	Area result(l);
	result.translate(-r);
	return result;
}

Area operator+ (const Area & l, const int3 & r)
{
	Area result(l);
	result.translate(r);
	return result;
}

Area operator+ (const Area & l, const Area & r)
{
	Area result;
	const auto & lTiles = l.getTiles();
	const auto & rTiles = r.getTiles();
	result.dTiles.reserve(lTiles.size() + rTiles.size());
	result.dTiles.insert(lTiles.begin(), lTiles.end());
	result.dTiles.insert(rTiles.begin(), rTiles.end());
	return result;
}

Area operator- (const Area & l, const Area & r)
{
	Area result(l);
	result.subtract(r);
	return result;
}

Area operator* (const Area & l, const Area & r)
{
	Area result(l);
	result.intersect(r);
	return result;
}

bool operator== (const Area & l, const Area & r)
{
	return l.getTiles() == r.getTiles();
}

}

VCMI_LIB_NAMESPACE_END
