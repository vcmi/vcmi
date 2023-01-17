/*
 * RmgMap.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "RmgMap.h"
#include "TileInfo.h"
#include "CMapGenOptions.h"
#include "Zone.h"
#include "../mapping/CMapEditManager.h"
#include "../CTownHandler.h"
#include "ObjectManager.h"
#include "RoadPlacer.h"
#include "TreasurePlacer.h"
#include "ConnectionsPlacer.h"
#include "TownPlacer.h"
#include "WaterAdopter.h"
#include "WaterProxy.h"
#include "WaterRoutes.h"
#include "RockPlacer.h"
#include "ObstaclePlacer.h"
#include "RiverPlacer.h"
#include "TerrainPainter.h"
#include "Functions.h"
#include "CMapGenerator.h"

VCMI_LIB_NAMESPACE_BEGIN

RmgMap::RmgMap(const CMapGenOptions& mapGenOptions) :
	mapGenOptions(mapGenOptions), zonesTotal(0)
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
	for(const int3 &dir : rmg::dirs4)
	{
		int3 n = pos + dir;
		if(mapInstance->isInTheMap(n))
			foo(n);
	}
}

void RmgMap::foreachDiagonalNeighbour(const int3& pos, std::function<void(int3& pos)> foo)
{
	for (const int3 &dir : rmg::dirsDiagonal)
	{
		int3 n = pos + dir;
		if (mapInstance->isInTheMap(n))
			foo(n);
	}
}

void RmgMap::initTiles(CMapGenerator & generator)
{
	mapInstance->initTerrain();
	
	tiles.resize(boost::extents[mapInstance->width][mapInstance->height][mapInstance->levels()]);
	zoneColouring.resize(boost::extents[mapInstance->width][mapInstance->height][mapInstance->levels()]);
	
	//init native town count with 0
	for (auto faction : VLC->townh->getAllowedFactions())
		zonesPerFaction[faction] = 0;
	
	getEditManager()->clearTerrain(&generator.rand);
	getEditManager()->getTerrainSelection().selectRange(MapRect(int3(0, 0, 0), mapGenOptions.getWidth(), mapGenOptions.getHeight()));
	getEditManager()->drawTerrain(ETerrainId::GRASS, &generator.rand);
	
	auto tmpl = mapGenOptions.getMapTemplate();
	zones.clear();
	for(const auto & option : tmpl->getZones())
	{
		auto zone = std::make_shared<Zone>(*this, generator);
		zone->setOptions(*option.second.get());
		zones[zone->getId()] = zone;
	}
	
	switch(mapGenOptions.getWaterContent())
	{
		case EWaterContent::NORMAL:
		case EWaterContent::ISLANDS:
			TRmgTemplateZoneId waterId = zones.size() + 1;
			rmg::ZoneOptions options;
			options.setId(waterId);
			options.setType(ETemplateZoneType::WATER);
			auto zone = std::make_shared<Zone>(*this, generator);
			zone->setOptions(options);
			zones[zone->getId()] = zone;
			break;
	}
}

void RmgMap::addModificators()
{
	for(auto & z : getZones())
	{
		auto zone = z.second;
		
		zone->addModificator<ObjectManager>();
		zone->addModificator<TreasurePlacer>();
		zone->addModificator<ObstaclePlacer>();
		zone->addModificator<TerrainPainter>();
		
		if(zone->getType() == ETemplateZoneType::WATER)
		{
			for(auto & z1 : getZones())
			{
				z1.second->addModificator<WaterAdopter>();
				z1.second->getModificator<WaterAdopter>()->setWaterZone(zone->getId());
			}
			zone->addModificator<WaterProxy>();
			zone->addModificator<WaterRoutes>();
		}
		else
		{
			zone->addModificator<TownPlacer>();
			zone->addModificator<ConnectionsPlacer>();
			zone->addModificator<RoadPlacer>();
			zone->addModificator<RiverPlacer>();
		}
		
		if(zone->isUnderground())
		{
			zone->addModificator<RockPlacer>();
		}
		
	}
}

RmgMap::~RmgMap()
{
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

void RmgMap::assertOnMap(const int3& tile) const
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
	assertOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z].isBlocked();
}

bool RmgMap::shouldBeBlocked(const int3 &tile) const
{
	assertOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z].shouldBeBlocked();
}

bool RmgMap::isPossible(const int3 &tile) const
{
	assertOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z].isPossible();
}

bool RmgMap::isFree(const int3 &tile) const
{
	assertOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z].isFree();
}

bool RmgMap::isUsed(const int3 &tile) const
{
	assertOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z].isUsed();
}

bool RmgMap::isRoad(const int3& tile) const
{
	assertOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z].isRoad();
}

void RmgMap::setOccupied(const int3 &tile, ETileType::ETileType state)
{
	assertOnMap(tile);
	
	tiles[tile.x][tile.y][tile.z].setOccupied(state);
}

void RmgMap::setRoad(const int3& tile, RoadId roadType)
{
	assertOnMap(tile);
	
	tiles[tile.x][tile.y][tile.z].setRoadType(roadType);
}

TileInfo RmgMap::getTile(const int3& tile) const
{
	assertOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z];
}

TRmgTemplateZoneId RmgMap::getZoneID(const int3& tile) const
{
	assertOnMap(tile);
	
	return zoneColouring[tile.x][tile.y][tile.z];
}

void RmgMap::setZoneID(const int3& tile, TRmgTemplateZoneId zid)
{
	assertOnMap(tile);
	
	zoneColouring[tile.x][tile.y][tile.z] = zid;
}

void RmgMap::setNearestObjectDistance(int3 &tile, float value)
{
	assertOnMap(tile);
	
	tiles[tile.x][tile.y][tile.z].setNearestObjectDistance(value);
}

float RmgMap::getNearestObjectDistance(const int3 &tile) const
{
	assertOnMap(tile);
	
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
	int levels = mapInstance->levels();
	int width =  mapInstance->width;
	int height = mapInstance->height;
	for (int k = 0; k < levels; k++)
	{
		for(int j = 0; j < height; j++)
		{
			for (int i = 0; i < width; i++)
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

VCMI_LIB_NAMESPACE_END
