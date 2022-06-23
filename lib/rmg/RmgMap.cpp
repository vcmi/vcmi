//
//  RmgMap.cpp
//  vcmi
//
//  Created by nordsoft on 23.06.2022.
//

#include "RmgMap.h"
#include "TileInfo.h"
#include "CMapGenOptions.h"
#include "Zone.h"
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"

static const int3 dirs4[] = {int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0)};
static const int3 dirsDiagonal[] = { int3(1,1,0),int3(1,-1,0),int3(-1,1,0),int3(-1,-1,0) };

RmgMap::RmgMap(const CMapGenOptions& mapGenOptions) :
mapGenOptions(mapGenOptions), zonesTotal(0), tiles(nullptr)
{
}

void RmgMap::foreach_neighbour(const int3 &pos, std::function<void(int3& pos)> foo)
{
	for(const int3 &dir : int3::getDirs())
	{
		int3 n = pos + dir;
		/*important notice: perform any translation before this function is called,
		 so the actual map position is checked*/
		if(map->isInTheMap(n))
			foo(n);
	}
}

void RmgMap::foreachDirectNeighbour(const int3& pos, std::function<void(int3& pos)> foo)
{
	for(const int3 &dir : dirs4)
	{
		int3 n = pos + dir;
		if(map->isInTheMap(n))
			foo(n);
	}
}

void RmgMap::foreachDiagonalNeighbour(const int3& pos, std::function<void(int3& pos)> foo)
{
	for (const int3 &dir : dirsDiagonal)
	{
		int3 n = pos + dir;
		if (map->isInTheMap(n))
			foo(n);
	}
}

void RmgMap::initTiles()
{
	map->initTerrain();
	
	int width = map->width;
	int height = map->height;
	
	int level = map->twoLevel ? 2 : 1;
	tiles = new CTileInfo**[width];
	for (int i = 0; i < width; ++i)
	{
		tiles[i] = new CTileInfo*[height];
		for (int j = 0; j < height; ++j)
		{
			tiles[i][j] = new CTileInfo[level];
		}
	}
	
	zoneColouring.resize(boost::extents[map->twoLevel ? 2 : 1][map->width][map->height]);
}

RmgMap::~RmgMap()
{
	if (tiles)
	{
		int width = mapGenOptions.getWidth();
		int height = mapGenOptions.getHeight();
		for (int i=0; i < width; i++)
		{
			for(int j=0; j < height; j++)
			{
				delete [] tiles[i][j];
			}
			delete [] tiles[i];
		}
		delete [] tiles;
	}
}

CMapEditManager* RmgMap::getEditManager() const
{
	if(!map)
		return nullptr;
	return map->getEditManager();
}

void RmgMap::checkIsOnMap(const int3& tile) const
{
	if (!map->isInTheMap(tile))
		throw rmgException(boost::to_string(boost::format("Tile %s is outside the map") % tile.toString()));
}

RmgMap::Zones & RmgMap::getZones()
{
	return zones;
}

bool RmgMap::isBlocked(const int3 &tile) const
{
	checkIsOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z].isBlocked();
}

bool RmgMap::shouldBeBlocked(const int3 &tile) const
{
	checkIsOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z].shouldBeBlocked();
}

bool RmgMap::isPossible(const int3 &tile) const
{
	checkIsOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z].isPossible();
}

bool RmgMap::isFree(const int3 &tile) const
{
	checkIsOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z].isFree();
}

bool RmgMap::isUsed(const int3 &tile) const
{
	checkIsOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z].isUsed();
}

bool RmgMap::isRoad(const int3& tile) const
{
	checkIsOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z].isRoad();
}

void RmgMap::setOccupied(const int3 &tile, ETileType::ETileType state)
{
	checkIsOnMap(tile);
	
	tiles[tile.x][tile.y][tile.z].setOccupied(state);
}

void RmgMap::setRoad(const int3& tile, const std::string & roadType)
{
	checkIsOnMap(tile);
	
	tiles[tile.x][tile.y][tile.z].setRoadType(roadType);
}

CTileInfo RmgMap::getTile(const int3& tile) const
{
	checkIsOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z];
}

TRmgTemplateZoneId RmgMap::getZoneID(const int3& tile) const
{
	checkIsOnMap(tile);
	
	return zoneColouring[tile.z][tile.x][tile.y];
}

void RmgMap::setZoneID(const int3& tile, TRmgTemplateZoneId zid)
{
	checkIsOnMap(tile);
	
	zoneColouring[tile.z][tile.x][tile.y] = zid;
}
