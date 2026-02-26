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
	dBorderCache.clear();
	dBorderOutsideCache.clear();
}

bool Area::connected(bool noDiagonals) const
{
	std::list<int3> queue({*dTiles.begin()});
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
		result.emplace_back();
		std::list<int3> queue({*connected.begin()});
		std::set<int3> queueSet({*connected.begin()});
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
	}
	return dTilesVectorCache;
}

const Tileset & Area::getBorder() const
{
	if(!dBorderCache.empty())
		return dBorderCache;
	
	//compute border cache
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
	
	return dBorderCache;
}

const Tileset & Area::getBorderOutside() const
{
	if(!dBorderOutsideCache.empty())
		return dBorderOutsideCache;
	
	//compute outside border cache
	dBorderOutsideCache.reserve(dBorderCache.bucket_count() * 2);
	for(const auto & t : dTiles)
	{
		for(auto & i : int3::getDirs())
		{
			if(!dTiles.count(t + i))
				dBorderOutsideCache.insert(t + i + dTotalShiftCache);
		}
	}
	
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
	return contains(area.getTilesVector());
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
	return overlap(area.getTilesVector());
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
	subset.dTiles.reserve(getTilesVector().size());
	vstd::copy_if(getTilesVector(), vstd::set_inserter(subset.dTiles), filter);
	return subset;
}

void Area::clear()
{
	dTiles.clear();
	dTilesVectorCache.clear();
	dTotalShiftCache = int3();
	invalidate();
}

void Area::assign(const Tileset tiles)
{
	clear();
	dTiles = tiles;
}

void Area::add(const int3 & tile)
{
	invalidate();
	dTiles.insert(tile);
}

void Area::erase(const int3 & tile)
{
	invalidate();
	dTiles.erase(tile);
}
void Area::unite(const Area & area)
{
	invalidate();
	const auto & vec = area.getTilesVector();
	dTiles.reserve(dTiles.size() + vec.size());
	dTiles.insert(vec.begin(), vec.end());
}

void Area::intersect(const Area & area)
{
	invalidate();
	Tileset result;
	result.reserve(std::max(dTiles.size(), area.getTilesVector().size()));
	for(const auto & t : area.getTilesVector())
	{
		if(dTiles.count(t))
			result.insert(t);
	}
	dTiles = result;
}

void Area::subtract(const Area & area)
{
	invalidate();
	for(const auto & t : area.getTilesVector())
	{
		dTiles.erase(t);
	}
}

void Area::translate(const int3 & shift)
{
	dBorderCache.clear();
	dBorderOutsideCache.clear();

	if(dTilesVectorCache.empty())
	{
		getTilesVector();
	}
	
	//avoid recomputation within std::set, use vector instead
	dTotalShiftCache += shift;
	
	for(auto & t : dTilesVectorCache)
	{
		t += shift;
	}
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
	const auto & lTiles = l.getTilesVector();
	const auto & rTiles = r.getTilesVector();
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
	return l.getTilesVector() == r.getTilesVector();
}

}

VCMI_LIB_NAMESPACE_END
