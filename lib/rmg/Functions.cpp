/*
 * Functions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "Functions.h"
#include "CMapGenerator.h"
#include "ObjectManager.h"
#include "RoadPlacer.h"
#include "TreasurePlacer.h"
#include "ConnectionsPlacer.h"
#include "TownPlacer.h"
#include "WaterProxy.h"
#include "WaterRoutes.h"
#include "RmgMap.h"
#include "TileInfo.h"
#include "RmgPath.h"
#include "../CTownHandler.h"
#include "../mapping/CMapEditManager.h"
#include "../mapping/CMap.h"
#include "../mapObjects/CommonConstructors.h"
#include "../mapObjects/MapObjects.h" //needed to resolve templates for CommonConstructors.h
#include "../VCMI_Lib.h"

VCMI_LIB_NAMESPACE_BEGIN

void createModificators(RmgMap & map)
{
	for(auto & z : map.getZones())
	{
		auto & zone = *z.second;
		switch(zone.getType())
		{
			case ETemplateZoneType::WATER:
				zone.addModificator<ObjectManager>();
				zone.addModificator<TreasurePlacer>();
				zone.addModificator<WaterProxy>();
				zone.addModificator<WaterRoutes>();
				break;
				
			default:
				zone.addModificator<TownPlacer>();
				zone.addModificator<ObjectManager>();
				zone.addModificator<ConnectionsPlacer>();
				zone.addModificator<TreasurePlacer>();
				zone.addModificator<RoadPlacer>();
				break;
		}
		
	}
}

rmg::Tileset collectDistantTiles(const Zone& zone, int distance)
{
	int distanceSq = distance * distance;
	auto subarea = zone.getArea().getSubarea([&zone, distanceSq](const int3 & t)
	{
		return t.dist2dSQ(zone.getPos()) > distanceSq;
	});
	return subarea.getTiles();
}

void createBorder(RmgMap & gen, Zone & zone)
{
	rmg::Area borderArea(zone.getArea().getBorder());
	rmg::Area borderOutsideArea(zone.getArea().getBorderOutside());
	auto blockBorder = borderArea.getSubarea([&gen, &borderOutsideArea](const int3 & t)
	{
		auto tile = borderOutsideArea.nearest(t);
		return gen.isOnMap(tile) && gen.getZones()[gen.getZoneID(tile)]->getType() != ETemplateZoneType::WATER;
	});
	
	for(auto & tile : blockBorder.getTilesVector())
	{
		if(gen.isPossible(tile))
		{
			gen.setOccupied(tile, ETileType::BLOCKED);
			zone.areaPossible().erase(tile);
		}
		
		gen.foreachDirectNeighbour(tile, [&gen, &zone](int3 &nearbyPos)
		{
			if(gen.isPossible(nearbyPos) && gen.getZoneID(nearbyPos) == zone.getId())
			{
				gen.setOccupied(nearbyPos, ETileType::BLOCKED);
				zone.areaPossible().erase(nearbyPos);
			}
		});
	}
}

void paintZoneTerrain(const Zone & zone, CRandomGenerator & generator, RmgMap & map, const Terrain & terrainType)
{
	auto v = zone.getArea().getTilesVector();
	map.getEditManager()->getTerrainSelection().setSelection(v);
	map.getEditManager()->drawTerrain(terrainType, &generator);
}

int chooseRandomAppearance(CRandomGenerator & generator, si32 ObjID, const Terrain & terrain)
{
	auto factories = VLC->objtypeh->knownSubObjects(ObjID);
	vstd::erase_if(factories, [ObjID, &terrain](si32 f)
	{
		return VLC->objtypeh->getHandlerFor(ObjID, f)->getTemplates(terrain).empty();
	});
	
	return *RandomGeneratorUtil::nextItem(factories, generator);
}

void initTerrainType(Zone & zone, CMapGenerator & gen)
{
	if(zone.getType()==ETemplateZoneType::WATER)
	{
		//collect all water terrain types
		std::vector<Terrain> waterTerrains;
		for(auto & terrain : Terrain::Manager::terrains())
			if(terrain.isWater())
				waterTerrains.push_back(terrain);
		
		zone.setTerrainType(*RandomGeneratorUtil::nextItem(waterTerrains, gen.rand));
	}
	else
	{
		if(zone.isMatchTerrainToTown() && zone.getTownType() != ETownType::NEUTRAL)
		{
			zone.setTerrainType((*VLC->townh)[zone.getTownType()]->nativeTerrain);
		}
		else
		{
			zone.setTerrainType(*RandomGeneratorUtil::nextItem(zone.getTerrainTypes(), gen.rand));
		}
		
		//TODO: allow new types of terrain?
		{
			if(zone.isUnderground())
			{
				if(!vstd::contains(gen.getConfig().terrainUndergroundAllowed, zone.getTerrainType()))
				{
					//collect all underground terrain types
					std::vector<Terrain> undegroundTerrains;
					for(auto & terrain : Terrain::Manager::terrains())
						if(terrain.isUnderground())
							undegroundTerrains.push_back(terrain);
					
					zone.setTerrainType(*RandomGeneratorUtil::nextItem(undegroundTerrains, gen.rand));
				}
			}
			else
			{
				if(vstd::contains(gen.getConfig().terrainGroundProhibit, zone.getTerrainType()) || zone.getTerrainType().isUnderground())
					zone.setTerrainType(Terrain("dirt"));
			}
		}
	}
}

void createObstaclesCommon2(RmgMap & map, CRandomGenerator & generator)
{
	if(map.map().twoLevel)
	{
		//finally mark rock tiles as occupied, spawn no obstacles there
		for(int x = 0; x < map.map().width; x++)
		{
			for(int y = 0; y < map.map().height; y++)
			{
				int3 tile(x, y, 1);
				if(!map.map().getTile(tile).terType.isPassable())
				{
					map.setOccupied(tile, ETileType::USED);
				}
			}
		}
	}
}

VCMI_LIB_NAMESPACE_END
