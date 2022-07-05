/*
 * Functions.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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

void createBorder(RmgMap & gen, const Zone & zone)
{
	for(auto & tile : zone.getArea().getBorderOutside())
	{
		if(gen.isOnMap(tile) && gen.getZoneID(tile) != zone.getId() && gen.getZones()[gen.getZoneID(tile)]->getType() != ETemplateZoneType::WATER) //optimization - better than set search
		{
			if(gen.isPossible(tile))
			{
				gen.setOccupied(tile, ETileType::BLOCKED);
				gen.getZones().at(gen.getZoneID(tile))->areaPossible().erase(tile);
			}
			
			gen.foreach_neighbour(tile, [&gen](int3 &nearbyPos)
			{
				if(gen.isPossible(nearbyPos))
				{
					gen.setOccupied(nearbyPos, ETileType::BLOCKED);	
					gen.getZones().at(gen.getZoneID(nearbyPos))->areaPossible().erase(nearbyPos);
				}
			});
		}
	}
}

void paintZoneTerrain(const Zone & zone, CRandomGenerator & generator, RmgMap & map, const Terrain & terrainType)
{
	auto v = zone.getArea().getTilesVector();
	map.getEditManager()->getTerrainSelection().setSelection(v);
	map.getEditManager()->drawTerrain(terrainType, &generator);
}

void createObstacles1(const Zone & zone, RmgMap & map, CRandomGenerator & generator)
{
	if(zone.isUnderground()) //underground
	{
		//now make sure all accessible tiles have no additional rock on them
		std::vector<int3> accessibleTiles;
		for(auto tile : zone.getArea().getTiles())
		{
			if(map.isFree(tile) || map.isUsed(tile))
			{
				accessibleTiles.push_back(tile);
			}
		}
		map.getEditManager()->getTerrainSelection().setSelection(accessibleTiles);
		map.getEditManager()->drawTerrain(zone.getTerrainType(), &generator);
	}
}

void createObstacles2(const Zone & zone, RmgMap & map, CRandomGenerator & generator, ObjectManager & manager)
{
	typedef std::vector<ObjectTemplate> obstacleVector;
	//obstacleVector possibleObstacles;
	
	std::map <ui8, obstacleVector> obstaclesBySize;
	typedef std::pair <ui8, obstacleVector> obstaclePair;
	std::vector<obstaclePair> possibleObstacles;
	
	//get all possible obstacles for this terrain
	for(auto primaryID : VLC->objtypeh->knownObjects())
	{
		for(auto secondaryID : VLC->objtypeh->knownSubObjects(primaryID))
		{
			auto handler = VLC->objtypeh->getHandlerFor(primaryID, secondaryID);
			if(handler->isStaticObject())
			{
				for(auto temp : handler->getTemplates())
				{
					if(temp.canBePlacedAt(zone.getTerrainType()) && temp.getBlockMapOffset().valid())
						obstaclesBySize[(ui8)temp.getBlockedOffsets().size()].push_back(temp);
				}
			}
		}
	}
	for (auto o : obstaclesBySize)
	{
		possibleObstacles.push_back (std::make_pair(o.first, o.second));
	}
	boost::sort (possibleObstacles, [](const obstaclePair &p1, const obstaclePair &p2) -> bool
	{
		return p1.first > p2.first; //bigger obstacles first
	});
	
	auto sel = map.getEditManager()->getTerrainSelection();
	sel.clearSelection();
	
	auto tryToPlaceObstacleHere = [&map, &generator, &manager, &possibleObstacles](int3& tile, int index)-> bool
	{
		auto temp = *RandomGeneratorUtil::nextItem(possibleObstacles[index].second, generator);
		int3 obstaclePos = tile + temp.getBlockMapOffset();
		if(canObstacleBePlacedHere(map, temp, obstaclePos)) //can be placed here
		{
			auto obj = VLC->objtypeh->getHandlerFor(temp.id, temp.subid)->create(temp);
			rmg::Object rmgObject(*obj);
			rmgObject.setPosition(obstaclePos);
			manager.placeObject(rmgObject, false, false);
			return true;
		}
		return false;
	};
	
	//reverse order, since obstacles begin in bottom-right corner, while the map coordinates begin in top-left
	for(auto tile : boost::adaptors::reverse(zone.getArea().getTiles()))
	{
		//fill tiles that should be blocked with obstacles
		if(map.shouldBeBlocked(tile))
		{
			//start from biggets obstacles
			for(int i = 0; i < possibleObstacles.size(); i++)
			{
				if (tryToPlaceObstacleHere(tile, i))
					break;
			}
		}
	}
	//cleanup - remove unused possible tiles to make space for roads
	for(auto tile : zone.getArea().getTiles())
	{
		if(map.isPossible(tile))
		{
			map.setOccupied(tile, ETileType::FREE);
		}
	}
}

bool canObstacleBePlacedHere(const RmgMap & map, ObjectTemplate &temp, int3 &pos)
{
	if (!map.isOnMap(pos)) //blockmap may fit in the map, but botom-right corner does not
		return false;
	
	auto tilesBlockedByObject = temp.getBlockedOffsets();
	
	for (auto blockingTile : tilesBlockedByObject)
	{
		int3 t = pos + blockingTile;
		if(!map.isOnMap(t) || !(/*map.isPossible(t) || */map.shouldBeBlocked(t)) || !temp.canBePlacedAt(map.map().getTile(t).terType))
		{
			return false; //if at least one tile is not possible, object can't be placed here
		}
	}
	return true;
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

void createObstaclesCommon1(RmgMap & map, CRandomGenerator & generator)
{
	if(map.map().twoLevel) //underground
	{
		//negative approach - create rock tiles first, then make sure all accessible tiles have no rock
		std::vector<int3> rockTiles;
		
		for(int x = 0; x < map.map().width; x++)
		{
			for(int y = 0; y < map.map().height; y++)
			{
				int3 tile(x, y, 1);
				if (map.shouldBeBlocked(tile))
				{
					rockTiles.push_back(tile);
				}
			}
		}
		map.getEditManager()->getTerrainSelection().setSelection(rockTiles);
		
		//collect all rock terrain types
		std::vector<Terrain> rockTerrains;
		for(auto & terrain : Terrain::Manager::terrains())
			if(!terrain.isPassable())
				rockTerrains.push_back(terrain);
		auto rockTerrain = *RandomGeneratorUtil::nextItem(rockTerrains, generator);
		
		map.getEditManager()->drawTerrain(rockTerrain, &generator);
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
	
	//tighten obstacles to improve visuals
	
	/*for (int i = 0; i < 3; ++i)
	{
		int blockedTiles = 0;
		int freeTiles = 0;
		
		for (int z = 0; z < (map.map().twoLevel ? 2 : 1); z++)
		{
			for (int x = 0; x < map.map().width; x++)
			{
				for (int y = 0; y < map.map().height; y++)
				{
					int3 tile(x, y, z);
					if (!map.isPossible(tile)) //only possible tiles can change
						continue;
					
					int blockedNeighbours = 0;
					int freeNeighbours = 0;
					map.foreach_neighbour(tile, [&map, &blockedNeighbours, &freeNeighbours](int3 &pos)
					{
						if (map.isBlocked(pos))
							blockedNeighbours++;
						if (map.isFree(pos))
							freeNeighbours++;
					});
					if (blockedNeighbours > 4)
					{
						map.setOccupied(tile, ETileType::BLOCKED);
						blockedTiles++;
					}
					else if (freeNeighbours > 4)
					{
						map.setOccupied(tile, ETileType::FREE);
						freeTiles++;
					}
				}
			}
		}
		logGlobal->trace("Set %d tiles to BLOCKED and %d tiles to FREE", blockedTiles, freeTiles);
	}*/
}
