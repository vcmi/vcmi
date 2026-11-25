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
#include "../entities/faction/CTownHandler.h"
#include "../mapping/CMapEditManager.h"
#include "../mapping/CMap.h"
#include "../GameLibrary.h"
#include "modificators/ObjectManager.h"
#include "modificators/RoadPlacer.h"
#include "modificators/TreasurePlacer.h"
#include "modificators/PrisonHeroPlacer.h"
#include "modificators/QuestArtifactPlacer.h"
#include "modificators/ConnectionsPlacer.h"
#include "modificators/TownPlacer.h"
#include "modificators/MinePlacer.h"
#include "modificators/ObjectPlacer.h"
#include "modificators/ObjectDistributor.h"
#include "modificators/WaterAdopter.h"
#include "modificators/WaterProxy.h"
#include "modificators/WaterRoutes.h"
#include "modificators/RockPlacer.h"
#include "modificators/RockFiller.h"
#include "modificators/ObstaclePlacer.h"
#include "modificators/RiverPlacer.h"
#include "modificators/TerrainPainter.h"
#include "Functions.h"
#include "CMapGenerator.h"

RmgMap::RmgMap(const CMapGenOptions& mapGenOptions, IGameInfoCallback * cb) :
	mapGenOptions(mapGenOptions), zonesTotal(0)
{
	mapInstance = std::make_unique<CMap>(cb);
	mapProxy = std::make_shared<MapProxy>(*this);
	getEditManager()->getUndoManager().setUndoRedoLimit(0);
}

int RmgMap::getDecorationsPercentage() const
{
	return 15; // arbitrary value to generate more readable map
}

void RmgMap::foreach_neighbour(const int3 & pos, const std::function<void(int3 & pos)> & foo) const
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

void RmgMap::foreachDirectNeighbour(const int3 & pos, const std::function<void(int3 & pos)> & foo) const
{
	for(const int3 &dir : rmg::dirs4)
	{
		int3 n = pos + dir;
		if(mapInstance->isInTheMap(n))
			foo(n);
	}
}

void RmgMap::foreachDiagonalNeighbour(const int3 & pos, const std::function<void(int3 & pos)> & foo) const
{
	for (const int3 &dir : rmg::dirsDiagonal)
	{
		int3 n = pos + dir;
		if (mapInstance->isInTheMap(n))
			foo(n);
	}
}

void RmgMap::initTiles(CMapGenerator & generator, vstd::RNG & rand)
{
	mapInstance->initTerrain();
	
	tiles.resize(boost::extents[mapInstance->width][mapInstance->height][mapInstance->levels()]);
	zoneColouring.resize(boost::extents[mapInstance->width][mapInstance->height][mapInstance->levels()]);
	
	//init native town count with 0
	for (auto faction : LIBRARY->townh->getAllowedFactions())
		zonesPerFaction[faction] = 0;
	
	getEditManager()->clearTerrain(&rand);
	getEditManager()->getTerrainSelection().selectRange(MapRect(int3(0, 0, 0), mapGenOptions.getWidth(), mapGenOptions.getHeight()));
	getEditManager()->drawTerrain(ETerrainId::GRASS, getDecorationsPercentage(), &rand);

	const auto * tmpl = mapGenOptions.getMapTemplate();
	zones.clear();
	for(const auto & option : tmpl->getZones())
	{
		auto zone = std::make_shared<Zone>(*this, generator, rand);
		zone->setOptions(*option.second);
		zones[zone->getId()] = zone;
	}
}

void RmgMap::addWaterZones(CMapGenerator & generator, vstd::RNG & rand)
{
	std::set<int> landZoneLevels;
	TRmgTemplateZoneId nextId = 0;
	for(const auto & zonePair : zones)
	{
		landZoneLevels.insert(zonePair.second->getPos().z);
		if(zonePair.second->getId() > nextId)
			nextId = zonePair.second->getId();
	}

	const auto levels = waterZoneLevels(mapGenOptions.getWaterContent(), mapGenOptions.getLevelMapLayers(), landZoneLevels);
	for(int level : levels)
	{
		++nextId;
		rmg::ZoneOptions options;
		options.setId(nextId);
		options.setType(ETemplateZoneType::WATER);
		auto zone = std::make_shared<Zone>(*this, generator, rand);
		zone->setOptions(options);
		zone->monsterStrength = EMonsterStrength::ZONE_NONE;
		zone->setPos(int3(0, 0, level));
		zone->setCenter(float3(0.f, 0.f, static_cast<float>(level)));
		zones[zone->getId()] = zone;
	}
}

void RmgMap::addModificators()
{
	bool hasObjectDistributor = false;
	bool hasHeroPlacer = false;
	std::set<int> rockFillerLevels;

	for(auto & z : getZones())
	{
		auto zone = z.second;
		
		zone->addModificator<ObjectManager>();
		if (!hasObjectDistributor)
		{
			zone->addModificator<ObjectDistributor>();
			hasObjectDistributor = true;
		}
		if (!hasHeroPlacer)
		{
			zone->addModificator<PrisonHeroPlacer>();
			hasHeroPlacer = true;
		}
		zone->addModificator<TreasurePlacer>();
		zone->addModificator<ObstaclePlacer>();
		zone->addModificator<TerrainPainter>();
		
		if(zone->getType() == ETemplateZoneType::WATER)
		{
			zone->addModificator<WaterProxy>();
			zone->addModificator<WaterRoutes>();
		}
		else
		{
			zone->addModificator<TownPlacer>();
			zone->addModificator<MinePlacer>();
			zone->addModificator<ObjectPlacer>();
			zone->addModificator<QuestArtifactPlacer>();
			zone->addModificator<ConnectionsPlacer>();
			zone->addModificator<RoadPlacer>();
			zone->addModificator<RiverPlacer>();
		}
		
		if(zone->isUnderground())
		{
			zone->addModificator<RockPlacer>();
			int level = zone->getPos().z;
			if (!rockFillerLevels.contains(level))
			{
				zone->addModificator<RockFiller>();
				rockFillerLevels.insert(level);
			}
		}
	}

	std::map<int, TRmgTemplateZoneId> waterZoneByLevel;
	for(const auto & z : getZones())
	{
		if(z.second->getType() == ETemplateZoneType::WATER)
			waterZoneByLevel[z.second->getPos().z] = z.second->getId();
	}

	for(auto & z : getZones())
	{
		if(z.second->getType() == ETemplateZoneType::WATER)
			continue;

		const auto it = waterZoneByLevel.find(z.second->getPos().z);
		if(it == waterZoneByLevel.end())
			continue;

		z.second->addModificator<WaterAdopter>();
		z.second->getModificator<WaterAdopter>()->setWaterZone(it->second);
	}
}

int RmgMap::levels() const
{
	return mapInstance->levels();
}

int RmgMap::width() const
{
	return mapInstance->width;
}

int RmgMap::height() const
{
	return mapInstance->height;
}

PlayerInfo & RmgMap::getPlayer(int playerId)
{
	return mapInstance->players.at(playerId);
}

std::shared_ptr<MapProxy> RmgMap::getMapProxy() const
{
	return mapProxy;
}

CMap& RmgMap::getMap(const CMapGenerator*) const
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
		throw rmgException(boost::str(boost::format("Tile %s is outside the map") % tile.toString()));
}

RmgMap::Zones & RmgMap::getZones()
{
	return zones;
}

RmgMap::Zones RmgMap::getZonesOnLevel(int level) const
{
	Zones zonesOnLevel;
	for(const auto& zonePair : zones)
	{
		if(zonePair.second->getPos().z == level)
		{
			zonesOnLevel.insert(zonePair);
		}
	}
	return zonesOnLevel;
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

void RmgMap::setOccupied(const int3 &tile, ETileType state)
{
	assertOnMap(tile);
	
	tiles[tile.x][tile.y][tile.z].setOccupied(state);
}

void RmgMap::setRoad(const int3& tile, RoadId roadType)
{
	assertOnMap(tile);
	
	tiles[tile.x][tile.y][tile.z].setRoadType(roadType);
}

TileInfo RmgMap::getTileInfo(const int3& tile) const
{
	assertOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z];
}

TerrainTile & RmgMap::getTile(const int3& tile) const
{
	return mapInstance->getTile(tile);
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

void RmgMap::setNearestObjectDistance(const int3 &tile, float value)
{
	assertOnMap(tile);
	
	tiles[tile.x][tile.y][tile.z].setNearestObjectDistance(value);
}

float RmgMap::getNearestObjectDistance(const int3 &tile) const
{
	assertOnMap(tile);
	
	return tiles[tile.x][tile.y][tile.z].getNearestObjectDistance();
}

void RmgMap::registerZone(FactionID faction)
{
	//FIXME: Protect with lock guard?
	zonesPerFaction[faction]++;
	zonesTotal++;
}

ui32 RmgMap::getZoneCount(FactionID faction)
{
	return zonesPerFaction[faction];
}

ui32 RmgMap::getTotalZoneCount() const
{
	return zonesTotal;
}

bool RmgMap::isAllowedSpell(const SpellID & sid) const
{
	assert(sid.getNum() >= 0);
	return mapInstance->allowedSpells.count(sid);
}
