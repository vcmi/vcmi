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
#include "../mapping/CMapEditManager.h"
#include "../CTownHandler.h"
#include "ObjectManager.h"
#include "RoadPlacer.h"
#include "TreasurePlacer.h"
#include "Functions.h"

static const int3 dirs4[] = {int3(0,1,0),int3(0,-1,0),int3(-1,0,0),int3(+1,0,0)};
static const int3 dirsDiagonal[] = { int3(1,1,0),int3(1,-1,0),int3(-1,1,0),int3(-1,-1,0) };

RmgMap::RmgMap(const CMapGenOptions& mapGenOptions) :
	mapGenOptions(mapGenOptions), zonesTotal(0), tiles(nullptr)
{
	mapInstance = std::make_unique<CMap>();
	getEditManager()->getUndoManager().setUndoRedoLimit(0);
}

void RmgMap::foreach_neighbour(const int3 &pos, std::function<void(int3& pos)> foo)
{
	for(const int3 &dir : int3::getDirs())
	{
		int3 n = pos + dir;
		/*important notice: perform any translation before this function is called,
		 so the actual mapInstance->position is checked*/
		if(mapInstance->isInTheMap(n))
			foo(n);
	}
}

void RmgMap::foreachDirectNeighbour(const int3& pos, std::function<void(int3& pos)> foo)
{
	for(const int3 &dir : dirs4)
	{
		int3 n = pos + dir;
		if(mapInstance->isInTheMap(n))
			foo(n);
	}
}

void RmgMap::foreachDiagonalNeighbour(const int3& pos, std::function<void(int3& pos)> foo)
{
	for (const int3 &dir : dirsDiagonal)
	{
		int3 n = pos + dir;
		if (mapInstance->isInTheMap(n))
			foo(n);
	}
}

void RmgMap::initTiles(CRandomGenerator & generator)
{
	mapInstance->initTerrain();
	
	int width = mapInstance->width;
	int height = mapInstance->height;
	
	int level = mapInstance->twoLevel ? 2 : 1;
	tiles = new CTileInfo**[width];
	for (int i = 0; i < width; ++i)
	{
		tiles[i] = new CTileInfo*[height];
		for (int j = 0; j < height; ++j)
		{
			tiles[i][j] = new CTileInfo[level];
		}
	}
	
	zoneColouring.resize(boost::extents[mapInstance->twoLevel ? 2 : 1][mapInstance->width][mapInstance->height]);
	
	//init native town count with 0
	for (auto faction : VLC->townh->getAllowedFactions())
		zonesPerFaction[faction] = 0;
	
	getEditManager()->clearTerrain(&generator);
	getEditManager()->getTerrainSelection().selectRange(MapRect(int3(0, 0, 0), mapGenOptions.getWidth(), mapGenOptions.getHeight()));
	getEditManager()->drawTerrain(Terrain("grass"), &generator);
	
	auto tmpl = mapGenOptions.getMapTemplate();
	zones.clear();
	for(const auto & option : tmpl->getZones())
	{
		auto zone = std::make_shared<Zone>(*this, generator);
		zone->setOptions(*option.second.get());
		zones[zone->getId()] = zone;
		zone->addModificator<ObjectManager>(generator);
		zone->addModificator<TreasurePlacer>(generator);
		zone->addModificator<RoadPlacer>(generator);
	}
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

CMap & RmgMap::map() const
{
	return *mapInstance;
}

CMapEditManager* RmgMap::getEditManager() const
{
	return mapInstance->getEditManager();
}

bool RmgMap::isOnMap(const int3 & tile) const
{
	return mapInstance->isInTheMap(tile);
}

const CMapGenOptions& RmgMap::getMapGenOptions() const
{
	return mapGenOptions;
}

void RmgMap::checkIsOnMap(const int3& tile) const
{
	if (!mapInstance->isInTheMap(tile))
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

void RmgMap::setNearestObjectDistance(int3 &tile, float value)
{
	checkIsOnMap(tile);
	
	tiles[tile.x][tile.y][tile.z].setNearestObjectDistance(value);
}

float RmgMap::getNearestObjectDistance(const int3 &tile) const
{
	checkIsOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z].getNearestObjectDistance();
}

void RmgMap::registerZone(TFaction faction)
{
	zonesPerFaction[faction]++;
	zonesTotal++;
}

ui32 RmgMap::getZoneCount(TFaction faction)
{
	return zonesPerFaction[faction];
}

ui32 RmgMap::getTotalZoneCount() const
{
	return zonesTotal;
}

bool RmgMap::isAllowedSpell(SpellID sid) const
{
	assert(sid >= 0);
	if (sid < mapInstance->allowedSpell.size())
	{
		return mapInstance->allowedSpell[sid];
	}
	else
		return false;
}

void RmgMap::dump(bool zoneId) const
{
	static int id = 0;
	std::ofstream out(boost::to_string(boost::format("zone_%d.txt") % id++));
	int levels = mapInstance->twoLevel ? 2 : 1;
	int width =  mapInstance->width;
	int height = mapInstance->height;
	for (int k = 0; k < levels; k++)
	{
		for(int j=0; j<height; j++)
		{
			for (int i=0; i<width; i++)
			{
				if(zoneId)
				{
					out << getZoneID(int3(i, j, k));
				}
				else
				{
					char t = '?';
					switch (getTile(int3(i, j, k)).getTileType())
					{
						case ETileType::FREE:
							t = ' '; break;
						case ETileType::BLOCKED:
							t = '#'; break;
						case ETileType::POSSIBLE:
							t = '-'; break;
						case ETileType::USED:
							t = 'O'; break;
					}
					out << t;
				}
			}
			out << std::endl;
		}
		out << std::endl;
	}
	out << std::endl;
}
