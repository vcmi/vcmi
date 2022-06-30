/*
 * RmgArea.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "RmgArea.h"
#include "CMapGenerator.h"

using namespace rmg;

size_t HashInt3::operator() (const int3 & t) const
{
	static const int maxSize = 500;
	static const int maxSizeSq = maxSize * maxSize;
	return (t.z + 1) * maxSizeSq + (t.y + 1) * maxSize + (t.x + 1);
}

void rmg::toAbsolute(Tileset & tiles, const int3 & position)
{
	Tileset temp;
	for(auto & tile : tiles)
	{
		temp.insert(tile + position);
	}
	tiles = std::move(temp);
}

void rmg::toRelative(Tileset & tiles, const int3 & position)
{
	rmg::toAbsolute(tiles, -position);
}

Area::Area(const Area & area): dTiles(area.getTiles())
{
}

Area::Area(const Area && area): dTiles(std::move(area.dTiles)), dTotalShiftCache(std::move(area.dTotalShiftCache))
{
}


Area & Area::operator=(const Area & area)
{
	invalidate();
	dTiles = area.getTiles();
	return *this;
}

Area::Area(const Tileset & tiles): dTiles(tiles)
{
}

Area::Area(const Tileset & relative, const int3 & position): dTiles(relative)
{
	toAbsolute(dTiles, position);
}

void Area::invalidate()
{
	getTiles();
	dTilesVectorCache.clear();
	dBorderCache.clear();
	dBorderOutsideCache.clear();
	dTotalShiftCache = int3();
}

bool Area::connected() const
{
	std::list<int3> queue({*getTiles().begin()});
	Tileset connected = getTiles();
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

std::list<Area> rmg::connectedAreas(const Area & area)
{
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
			
			for(auto & i : int3::getDirs())
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
		dTilesVectorCache.assign(dTiles.begin(), dTiles.end());
	return dTilesVectorCache;
}

const Tileset & Area::getBorder() const
{
	if(!dBorderCache.empty() || dTiles.empty())
		return dBorderCache;
	
	//compute border cache
	for(auto & t : getTiles())
	{
		for(auto & i : int3::getDirs())
		{
			if(!getTiles().count(t + i))
				dBorderCache.insert(t);
		}
	}
	
	return getBorder();
}

const Tileset & Area::getBorderOutside() const
{
	if(!dBorderOutsideCache.empty() || dTiles.empty())
		return dBorderOutsideCache;
	
	//compute outside border cache
	for(auto & t : getTiles())
	{
		for(auto & i : int3::getDirs())
		{
			if(!getTiles().count(t + i))
				dBorderOutsideCache.insert(t + i);
		}
	}
	
	return getBorderOutside();
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
	return getTiles().count(tile);
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
	for(auto & t : area.getTiles())
	{
		if(!contains(t))
			return false;
	}
	return true;
}

bool Area::overlap(const Area & area) const
{
	for(auto & t : area.getTiles())
	{
		if(contains(t))
			return true;
	}
	return false;
}

int Area::distanceSqr(const int3 & tile) const
{
	return nearest(tile).dist2dSQ(tile);
}

int Area::distanceSqr(const Area & area) const
{
	int dist = std::numeric_limits<int>::max();
	int3 nearTile = *getTiles().begin();
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
	return findClosestTile(getTiles(), tile);
}

int3 Area::nearest(const Area & area) const
{
	int dist = std::numeric_limits<int>::max();
	int3 nearTile = *getTiles().begin();
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
	for(auto & t : getTiles())
		if(filter(t))
			subset.add(t);
	return subset;
}

void Area::clear()
{
	invalidate();
	dTiles.clear();
}

/*void Area::assign(const std::set<int3> & tiles)
{
	dTiles = Tileset(tiles.begin(), tiles.end());
	dBorderCache.clear();
	dBorderOutsideCache.clear();
}*/

void Area::assign(const Tileset & tiles)
{
	invalidate();
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
	for(auto & t : area.getTiles())
	{
		dTiles.insert(t);
	}
}
void Area::intersect(const Area & area)
{
	invalidate();
	Tileset result;
	for(auto & t : area.getTiles())
	{
		if(dTiles.count(t))
			result.insert(t);
	}
	dTiles = result;
}

void Area::subtract(const Area & area)
{
	invalidate();
	for(auto & t : area.getTiles())
	{
		auto iter = dTiles.find(t);
		if(iter != dTiles.end())
			dTiles.erase(iter);
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

Area rmg::operator- (const Area & l, const int3 & r)
{
	Area result(l.getTiles(), -r);
	return result;
}

Area rmg::operator+ (const Area & l, const int3 & r)
{
	Area result(l.getTiles(), r);
	return result;
}

Area rmg::operator+ (const Area & l, const Area & r)
{
	Area result(l.getTiles());
	result.unite(r);
	return result;
}

Area rmg::operator- (const Area & l, const Area & r)
{
	Area result(l.getTiles());
	result.subtract(r);
	return result;
}

Area rmg::operator* (const Area & l, const Area & r)
{
	Area result(l.getTiles());
	result.intersect(r);
	return result;
}

bool rmg::operator== (const Area & l, const Area & r)
{
	return l.getTiles() == r.getTiles();
}
