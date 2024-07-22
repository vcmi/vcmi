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
#include "../VCMI_Lib.h"
#include "modificators/ObjectManager.h"
#include "modificators/RoadPlacer.h"
#include "modificators/TreasurePlacer.h"
#include "modificators/PrisonHeroPlacer.h"
#include "modificators/QuestArtifactPlacer.h"
#include "modificators/ConnectionsPlacer.h"
#include "modificators/TownPlacer.h"
#include "modificators/MinePlacer.h"
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

VCMI_LIB_NAMESPACE_BEGIN

RmgMap::RmgMap(const CMapGenOptions& mapGenOptions, IGameCallback * cb) :
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
	for (auto faction : VLC->townh->getAllowedFactions())
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
	
	switch(mapGenOptions.getWaterContent())
	{
		case EWaterContent::NORMAL:
		case EWaterContent::ISLANDS:
			TRmgTemplateZoneId waterId = zones.size() + 1;
			rmg::ZoneOptions options;
			options.setId(waterId);
			options.setType(ETemplateZoneType::WATER);
			auto zone = std::make_shared<Zone>(*this, generator, rand);
			zone->setOptions(options);
			//std::set<FactionID> allowedMonsterFactions({FactionID::CASTLE, FactionID::INFERNO}); // example of filling allowed monster factions
			//zone->setMonsterTypes(allowedMonsterFactions); // can be set here, probably from template json, along with the treasure ranges and densities
			zone->monsterStrength = EMonsterStrength::ZONE_NONE; // can be set to other value, probably from a new field in the template json
			zones[zone->getId()] = zone;
			break;
	}
}

void RmgMap::addModificators()
{
	bool hasObjectDistributor = false;
	bool hasHeroPlacer = false;
	bool hasRockFiller = false;

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
			zone->addModificator<MinePlacer>();
			zone->addModificator<QuestArtifactPlacer>();
			zone->addModificator<ConnectionsPlacer>();
			zone->addModificator<RoadPlacer>();
			zone->addModificator<RiverPlacer>();
		}
		
		if(zone->isUnderground())
		{
			zone->addModificator<RockPlacer>();
			if (!hasRockFiller)
			{
				zone->addModificator<RockFiller>();
				hasRockFiller = true;
			}
		}
		
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
		if(zonePair.second->isUnderground() == (bool)level)
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

void RmgMap::dump(bool zoneId) const
{
	static int id = 0;
	std::ofstream out(boost::str(boost::format("zone_%d.txt") % id++));
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
					switch (getTileInfo(int3(i, j, k)).getTileType())
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
