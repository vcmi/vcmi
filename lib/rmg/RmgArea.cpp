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
	Tileset temp;
	for(auto & tile : tiles)
	{
		temp.insert(tile + position);
	}
	tiles = std::move(temp);
}

void toRelative(Tileset & tiles, const int3 & position)
{
	toAbsolute(tiles, -position);
}

Area::Area(const Area & area): dTiles(area.dTiles), dTotalShiftCache(area.dTotalShiftCache)
{
}

Area::Area(const Area && area): dTiles(std::move(area.dTiles)), dTotalShiftCache(std::move(area.dTotalShiftCache))
{
}


Area & Area::operator=(const Area & area)
{
	clear();
	dTiles = area.dTiles;
	dTotalShiftCache = area.dTotalShiftCache;
	return *this;
}

Area::Area(const Tileset & tiles): dTiles(tiles)
{
}

Area::Area(const Tileset & relative, const int3 & position): dTiles(relative), dTotalShiftCache(position)
{
}

void Area::invalidate()
{
	getTiles();
	dTilesVectorCache.clear();
	dBorderCache.clear();
	dBorderOutsideCache.clear();
}

bool Area::connected() const
{
	std::list<int3> queue({*dTiles.begin()});
	Tileset connected = dTiles; //use invalidated cache - ok
	while(!queue.empty())
	{
		auto t = queue.front();
		connected.erase(t);
		queue.pop_front();
		
		for(auto & i : int3::getDirs())
		{
			if(connected.count(t + i))
			{
				queue.push_back(t + i);
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
	for(auto & t : dTiles)
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
	for(auto & t : dTiles)
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
		for(auto & tile : area.getBorder())
			result[tile] = distance;
		reverseDistanceMap[distance++] = area.getBorder();
		area.subtract(area.getBorder());
	}
	return result;
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
	for(auto & t : tiles)
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
	for(auto & t : tiles)
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

Area Area::getSubarea(std::function<bool(const int3 &)> filter) const
{
	Area subset;
	for(auto & t : getTilesVector())
		if(filter(t))
			subset.add(t);
	return subset;
}

void Area::clear()
{
	dTiles.clear();
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
	for(auto & t : area.getTilesVector())
	{
		dTiles.insert(t);
	}
}
void Area::intersect(const Area & area)
{
	invalidate();
	Tileset result;
	for(auto & t : area.getTilesVector())
	{
		if(dTiles.count(t))
			result.insert(t);
	}
	dTiles = result;
}

void Area::subtract(const Area & area)
{
	invalidate();
	for(auto & t : area.getTilesVector())
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
		getTiles();
		getTilesVector();
	}
	
	//avoid recomputation within std::set, use vector instead
	dTotalShiftCache += shift;
	
	for(auto & t : dTilesVectorCache)
	{
		t += shift;
	}
	//toAbsolute(dTiles, shift);
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
	Area result(l);
	result.unite(r);
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
