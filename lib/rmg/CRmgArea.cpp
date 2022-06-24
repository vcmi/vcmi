/*
 * CRmgArea.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "CRmgArea.h"
#include "CMapGenerator.h"

using namespace Rmg;

Tileset Rmg::toAbsolute(const Tileset & tiles, const int3 & position)
{
	Tileset result;
	std::transform(tiles.cbegin(), tiles.cend(), std::inserter(result, result.begin()), [&position](const int3 & t)
	{
		return t + position;
	});
	return result;
}

Tileset Rmg::toRelative(const Tileset & tiles, const int3 & position)
{
	return Rmg::toAbsolute(tiles, -position);
}

Area::Area(const Area & area): dTiles(area.dTiles), dBorderCache(area.dBorderCache), dBorderOutsideCache(area.dBorderOutsideCache)
{
}

Area::Area(const Area && area): dTiles(std::move(area.dTiles)), dBorderCache(std::move(area.dBorderCache)), dBorderOutsideCache(std::move(area.dBorderOutsideCache))
{
}


Area & Area::operator=(const Area & area)
{
	dTiles = area.dTiles;
	dBorderCache = area.dBorderCache;
	dBorderOutsideCache = area.dBorderOutsideCache;
	return *this;
}

Area::Area(const Tileset & tiles): dTiles(tiles)
{
}

Area::Area(const Tileset & relative, const int3 & position): Area(Rmg::toAbsolute(relative, position))
{
}

bool Area::connected() const
{
	std::list<int3> queue({*dTiles.begin()});
	Tileset connected = dTiles;
	while(!queue.empty())
	{
		auto t = queue.front();
		queue.pop_front();
		
		for(auto & i : int3::getDirs())
		{
			if(connected.count(t + i))
			{
				connected.erase(t + i);
				queue.push_back(t + i);
			}
		}
	}
	
	return connected.empty();
}

std::list<Area> Rmg::connectedAreas(const Area & area)
{
	std::list<Area> result;
	Tileset connected = area.dTiles;
	while(!connected.empty())
	{
		result.emplace_back();
		std::list<int3> queue({*connected.begin()});
		while(!queue.empty())
		{
			auto t = queue.front();
			queue.pop_front();
			
			for(auto & i : int3::getDirs())
			{
				if(connected.count(t + i))
				{
					connected.erase(t + i);
					queue.push_back(t + i);
					result.back().add(t + i);
				}
			}
		}
	}
	return result;
}

const Tileset & Area::getTiles() const
{
	return dTiles;
}

const Tileset & Area::getBorder() const
{
	if(!dBorderCache.empty() || dTiles.empty())
		return dBorderCache;
	
	//compute border cache
	for(auto & t : dTiles)
	{
		for(auto & i : int3::getDirs())
		{
			if(!dTiles.count(t + i))
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
	for(auto & t : dTiles)
	{
		for(auto & i : int3::getDirs())
		{
			if(!dTiles.count(t + i))
				dBorderOutsideCache.insert(t + i);
		}
	}
	
	return getBorderOutside();
}

Tileset Area::relative(const int3 & position) const
{
	return Rmg::toRelative(dTiles, position);
}

bool Area::empty() const
{
	return dTiles.empty();
}

bool Area::contains(const int3 & tile) const
{
	return dTiles.count(tile);
}

bool Area::contains(const Area & area) const
{
	for(auto & t : area.dTiles)
	{
		if(!contains(t))
			return false;
	}
	return true;
}

bool Area::overlap(const Area & area) const
{
	for(auto & t : area.dTiles)
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
	int3 nearTile = *dTiles.begin();
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
	return *std::min_element(dTiles.cbegin(), dTiles.cend(), std::bind(&int3::dist2dSQ, &tile, std::placeholders::_1));
}

int3 Area::nearest(const Area & area) const
{
	int dist = std::numeric_limits<int>::max();
	int3 nearTile = *dTiles.begin();
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
	Tileset subset;
	for(auto & t : dTiles)
		if(filter(t))
			subset.insert(t);
	return Area(subset);
}

void Area::clear()
{
	dTiles.clear();
	dBorderCache.clear();
	dBorderOutsideCache.clear();
}

void Area::assign(const Tileset & tiles)
{
	dTiles = tiles;
	dBorderCache.clear();
	dBorderOutsideCache.clear();
}

void Area::add(const int3 & tile)
{
	if(!dTiles.insert(tile).second)
		return;
		
	dBorderCache.clear();
	dBorderOutsideCache.clear();
}

void Area::erase(const int3 & tile)
{
	if(!dTiles.erase(tile))
		return;
	
	dBorderCache.clear();
	dBorderOutsideCache.clear();
}
void Area::unite(const Area & area)
{
	for(auto & t : area.dTiles)
	{
		dTiles.insert(t);
	}
	
	dBorderCache.clear();
	dBorderOutsideCache.clear();
}
void Area::intersect(const Area & area)
{
	Tileset result;
	for(auto & t : area.dTiles)
	{
		if(dTiles.count(t))
			result.insert(t);
	}
	dTiles = result;
	
	dBorderCache.clear();
	dBorderOutsideCache.clear();
}

void Area::subtract(const Area & area)
{
	for(auto & t : area.dTiles)
	{
		auto iter = dTiles.find(t);
		if(iter != dTiles.end())
			dTiles.erase(iter);
	}
	
	dBorderCache.clear();
	dBorderOutsideCache.clear();
}

void Area::translate(const int3 & shift)
{
	dTiles = Rmg::toAbsolute(dTiles, shift);
	
	//trivial change - do not invalidate cache
	dBorderCache = Rmg::toAbsolute(dBorderCache, shift);
	dBorderOutsideCache = Rmg::toAbsolute(dBorderOutsideCache, shift);
}

Area Rmg::operator- (const Area & l, const int3 & r)
{
	Area result(l.dTiles);
	result.translate(-r);
	return result;
}

Area Rmg::operator+ (const Area & l, const int3 & r)
{
	Area result(l.dTiles);
	result.translate(r);
	return result;
}

Area Rmg::operator+ (const Area & l, const Area & r)
{
	Area result(l.dTiles);
	result.unite(r);
	return result;
}

Area Rmg::operator- (const Area & l, const Area & r)
{
	Area result(l.dTiles);
	result.subtract(r);
	return result;
}

Area Rmg::operator* (const Area & l, const Area & r)
{
	Area result(l.dTiles);
	result.intersect(r);
	return result;
}

bool Rmg::operator== (const Area & l, const Area & r)
{
	return l.dTiles == r.dTiles;
}
