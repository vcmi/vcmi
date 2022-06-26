//
//  ObjectManager.cpp
//  vcmi
//
//  Created by nordsoft on 23.06.2022.
//

#include "ObjectManager.h"
#include "CMapGenerator.h"
#include "TileInfo.h"
#include "RmgMap.h"
#include "RoadPlacer.h"
#include "../CCreatureHandler.h"
#include "../mapObjects/CommonConstructors.h"
#include "../mapObjects/MapObjects.h" //needed to resolve templates for CommonConstructors.h
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "Functions.h"
#include "CRmgObject.h"

ObjectManager::ObjectManager(Zone & zone, RmgMap & map, CRandomGenerator & generator) : zone(zone), map(map), generator(generator)
{
	
}

void ObjectManager::process()
{
	createRequiredObjects();
}

void ObjectManager::addRequiredObject(CGObjectInstance * obj, si32 strength)
{
	requiredObjects.push_back(std::make_pair(obj, strength));
}

void ObjectManager::addCloseObject(CGObjectInstance * obj, si32 strength)
{
	closeObjects.push_back(std::make_pair(obj, strength));
}

void ObjectManager::addNearbyObject(CGObjectInstance * obj, CGObjectInstance * nearbyTarget)
{
	nearbyObjects.push_back(std::make_pair(obj, nearbyTarget));
}

void ObjectManager::addObjectAtPosition(CGObjectInstance * obj, const int3 & position, si32 strength)
{
	//TODO: use strength
	instantObjects.push_back(std::make_pair(obj, position));
}

void ObjectManager::setTemplateForObject(CGObjectInstance* obj)
{
	if(obj->appearance.id == Obj::NO_OBJ)
	{
		auto templates = VLC->objtypeh->getHandlerFor(obj->ID, obj->subID)->getTemplates(zone.getTerrainType());
		if(templates.empty())
			throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s") % obj->ID % obj->subID % zone.getTerrainType()));
		
		obj->appearance = templates.front();
	}
}

void ObjectManager::updateDistances(const int3 & pos)
{
	for (auto tile : zone.areaPossible().getTiles()) //don't need to mark distance for not possible tiles
	{
		ui32 d = pos.dist2dSQ(tile); //optimization, only relative distance is interesting
		map.setNearestObjectDistance(tile, std::min((float)d, map.getNearestObjectDistance(tile)));
	}
}


bool ObjectManager::isAccessibleFromSomewhere(ObjectTemplate & appearance, const int3 & tile) const
{
	return getAccessibleOffset(appearance, tile).valid();
}

int3 ObjectManager::getAccessibleOffset(ObjectTemplate & appearance, const int3 & tile) const
{
	auto tilesBlockedByObject = appearance.getBlockedOffsets();
	
	int3 ret(-1, -1, -1);
	for(int x = -1; x < 2; x++)
	{
		for(int y = -1; y <2; y++)
		{
			if(x && y) //check only if object is visitable from another tile
			{
				int3 offset = int3(x, y, 0) - appearance.getVisitableOffset();
				if (!vstd::contains(tilesBlockedByObject, offset))
				{
					int3 nearbyPos = tile + offset;
					if(map.isOnMap(nearbyPos))
					{
						if(appearance.isVisitableFrom(x, y) && !map.isBlocked(nearbyPos)/* && zone.getTileInfo().count(nearbyPos)*/)
							ret = nearbyPos;
					}
				}
			}
		}
	}
	return ret;
}

std::vector<int3> ObjectManager::getAccessibleOffsets(const CGObjectInstance* object) const
{
	//get all tiles from which this object can be accessed
	int3 visitable = object->visitablePos();
	std::vector<int3> tiles;
	
	auto tilesBlockedByObject = object->getBlockedPos(); //absolue value, as object is already placed
	
	map.foreach_neighbour(visitable, [&](int3& pos)
	{
		if(map.isPossible(pos) || map.isFree(pos))
		{
			if(!vstd::contains(tilesBlockedByObject, pos))
			{
				if(object->appearance.isVisitableFrom(pos.x - visitable.x, pos.y - visitable.y) && !map.isBlocked(pos)) //TODO: refactor - info about visitability from absolute coordinates
				{
					tiles.push_back(pos);
				}
			}
			
		};
	});
	
	return tiles;
}

bool ObjectManager::areAllTilesAvailable(CGObjectInstance* obj, int3& tile, const std::set<int3>& tilesBlockedByObject) const
{
	for(auto blockingTile : tilesBlockedByObject)
	{
		int3 t = tile + blockingTile;
		if(!map.isOnMap(t) || !map.isPossible(t) || map.getZoneID(t) != zone.getId())
		{
			//if at least one tile is not possible, object can't be placed here
			return false;
		}
	}
	return true;
}

int3 ObjectManager::findPlaceForObject(const Rmg::Area & searchArea, Rmg::Object & obj, std::function<float(const int3)> weightFunction) const
{
	float bestWeight = 0.f;
	int3 result(-1, -1, -1);
	auto usedTilesBorder = zone.areaUsed().getBorderOutside();
	
	auto appropriateArea = searchArea.getSubarea([&obj, &searchArea, &usedTilesBorder](const int3 & tile)
	{
		obj.setPosition(tile);
		if(usedTilesBorder.count(tile))
			return false;
		
		return searchArea.contains(obj.getArea()) && searchArea.overlap(obj.getAccessibleArea());
	});
	
	for (auto tile : appropriateArea.getTiles())
	{
		obj.setPosition(tile);
		float weight = weightFunction(tile);
		if(weight > bestWeight)
		{
			bestWeight = weight;
			result = tile;
		}
	}
	if(result.valid())
		obj.setPosition(result);
	return result;
}

int3 ObjectManager::findPlaceForObject(const Rmg::Area & searchArea, Rmg::Object & obj, si32 min_dist) const
{
	return findPlaceForObject(searchArea, obj, [this, min_dist](const int3 & tile)
	{
		auto ti = map.getTile(tile);
		auto dist = ti.getNearestObjectDistance();
		//avoid borders
		if(dist >= min_dist)
		{
			return dist;
		}
		return -1.f;
	});
}

ObjectManager::EObjectPlacingResult ObjectManager::tryToPlaceObjectAndConnectToPath(CGObjectInstance * obj, const int3 & pos)
{
	//check if we can find a path around this object. Tiles will be set to "USED" after object is successfully placed.
	obj->pos = pos;
	map.setOccupied(obj->visitablePos(), ETileType::BLOCKED);
	for(auto tile : obj->getBlockedPos())
	{
		if(map.isOnMap(tile))
			map.setOccupied(tile, ETileType::BLOCKED);
	}
	int3 accessibleOffset = getAccessibleOffset(obj->appearance, pos);
	if (!accessibleOffset.valid())
	{
		logGlobal->warn("Cannot access required object at position %s, retrying", pos.toString());
		return EObjectPlacingResult::CANNOT_FIT;
	}
	if(!zone.connectPath(accessibleOffset, true))
	{
		logGlobal->trace("Failed to create path to required object at position %s, retrying", pos.toString());
		return EObjectPlacingResult::SEALED_OFF;
	}
	else
		return EObjectPlacingResult::SUCCESS;
}

bool ObjectManager::placeAndConnectObject(const Rmg::Area & searchArea, Rmg::Object & obj, si32 min_dist, bool isGuarded, bool onlyStraight) const
{
	return placeAndConnectObject(searchArea, obj, [this, min_dist](const int3 & tile)
	{
		auto ti = map.getTile(tile);
		auto dist = ti.getNearestObjectDistance();
		//avoid borders
		if(dist >= min_dist)
		{
			return dist;
		}
		return -1.f;
	}, isGuarded, onlyStraight);
}

bool ObjectManager::placeAndConnectObject(const Rmg::Area & searchArea, Rmg::Object & obj, std::function<float(const int3)> weightFunction, bool isGuarded, bool onlyStraight) const
{
	int3 pos;
	auto possibleArea = searchArea;
	while(true)
	{
		pos = findPlaceForObject(possibleArea, obj, weightFunction);
		if(!pos.valid())
		{
			return false;
		}
		possibleArea.erase(pos); //do not place again at this point
		auto possibleAreaTemp = zone.areaPossible();
		zone.areaPossible().subtract(obj.getArea());
		if(zone.connectPath(obj.getAccessibleArea(isGuarded), onlyStraight))
		{
			zone.areaPossible() = possibleAreaTemp;
			return true;
		}
		zone.areaPossible() = possibleAreaTemp;
	}
}

bool ObjectManager::createRequiredObjects()
{
	logGlobal->trace("Creating required objects");
	
	for(const auto &object : requiredObjects)
	{
		auto * obj = object.first;
		int3 pos;
		Rmg::Object rmgObject(*obj);
		rmgObject.setTemplate(zone.getTerrainType());
		bool guarded = addGuard(rmgObject, object.second, (obj->ID == Obj::MONOLITH_TWO_WAY));
		
		if(!placeAndConnectObject(zone.areaPossible(), rmgObject, 3, guarded, false))
		{
			logGlobal->error("Failed to fill zone %d due to lack of space", zone.getId());
			return false;
		}
		
		placeObject(rmgObject, guarded, true);
	}
	
	for(const auto &object : closeObjects)
	{
		auto * obj = object.first;
		int3 pos;
		auto possibleArea = zone.areaPossible();
		Rmg::Object rmgObject(*obj);
		rmgObject.setTemplate(zone.getTerrainType());
		bool guarded = addGuard(rmgObject, object.second, (obj->ID == Obj::MONOLITH_TWO_WAY));
		
		if(!placeAndConnectObject(zone.areaPossible(), rmgObject,
								[this, &rmgObject](const int3 & tile)
								{
									float dist = rmgObject.getArea().distanceSqr(zone.getPos());
									dist *= (dist > 12.f * 12.f) ? 10.f : 1.f; //tiles closer 12 are preferrable
									dist = 1000000.f - dist; //some big number
									return dist + map.getNearestObjectDistance(tile);
								}, guarded, false))
		{
			logGlobal->error("Failed to fill zone %d due to lack of space", zone.getId());
			return false;
		}
		
		placeObject(rmgObject, guarded, true);
	}
	
	//create nearby objects (e.g. extra resources close to mines)
	for(const auto & object : nearbyObjects)
	{
		/*auto * obj = object.first;
		int3 pos;
		auto possibleArea = obj.second;
		Rmg::Object rmgObject(*obj);
		rmgObject.setTemplate(zone.getTerrainType());
		pos = findPlaceForObject(possibleArea, rmgObject, [this, &rmgObject](const int3 & tile)
		{
			float dist = rmgObject.getArea().distanceSqr(zone.getPos());
			dist *= (dist > 12) ? 10 : 1;
			return dist * 0.5f - std::sqrt(map.getNearestObjectDistance(tile));
		});
		if(!pos.valid())
		{
			logGlobal->error("Failed to fill zone %d due to lack of space", zone.getId());
			return false;
		}
		possibleArea.erase(pos); //do not place again at this point
		auto possibleAreaTemp = zone.areaPossible();
		zone.areaPossible().subtract(rmgObject.getArea());
		rmgObject.finalize(map);
		updateDistances(pos);

		auto obj = object.first;
		std::set<int3> possiblePositions;
		for (auto blockedTile : object.second->getBlockedPos())
		{
			map.foreachDirectNeighbour(blockedTile, [this, &possiblePositions](int3 pos)
			{
				if (!map.isBlocked(pos) && zone.getArea().contains(pos))
				{
					//some resources still could be unaccessible, at least one free cell shall be
					map.foreach_neighbour(pos, [this, &possiblePositions, &pos](int3 p)
					{
						if(map.isFree(p))
							possiblePositions.insert(pos);
					});
				}
			});
		}
		
		if(possiblePositions.empty())
		{
			delete obj; //is it correct way to prevent leak?
		}
		else
		{
			auto pos = *RandomGeneratorUtil::nextItem(possiblePositions, generator);
			placeObject(obj, pos);
		}*/
	}
	
	//create object on specific positions
	//TODO: implement guards
	for (const auto &obj : instantObjects)
	{
		Rmg::Object rmgObject(*obj.first);
		rmgObject.setPosition(obj.second);
		placeObject(rmgObject, false, false);
	}
	
	requiredObjects.clear();
	closeObjects.clear();
	nearbyObjects.clear();
	instantObjects.clear();
	
	return true;
}

void ObjectManager::checkAndPlaceObject(CGObjectInstance* object, const int3 &pos)
{
	if (!map.isOnMap(pos))
		throw rmgException(boost::to_string(boost::format("Position of object %d at %s is outside the map") % object->id % pos.toString()));
	object->pos = pos;
	
	if (object->isVisitable() && !map.isOnMap(object->visitablePos()))
		throw rmgException(boost::to_string(boost::format("Visitable tile %s of object %d at %s is outside the map") % object->visitablePos().toString() % object->id % object->pos.toString()));
	for (auto tile : object->getBlockedPos())
	{
		if (!map.isOnMap(tile))
			throw rmgException(boost::to_string(boost::format("Tile %s of object %d at %s is outside the map") % tile.toString() % object->id % object->pos.toString()));
	}
	
	if (object->appearance.id == Obj::NO_OBJ)
	{
		auto terrainType = map.map().getTile(pos).terType;
		auto h = VLC->objtypeh->getHandlerFor(object->ID, object->subID);
		auto templates = h->getTemplates(terrainType);
		if (templates.empty())
			throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s (terrain %d)") % object->ID % object->subID % pos.toString() % terrainType));
		
		object->appearance = templates.front();
	}
	
	map.getEditManager()->insertObject(object);
}

void ObjectManager::placeObject(Rmg::Object & object, bool guarded, bool updateDistance)
{
	object.finalize(map);
	zone.areaPossible().subtract(object.getArea());
	zone.areaUsed().unite(object.getArea());
	zone.areaUsed().erase(object.getVisitablePosition());
	
	if(guarded)
	{
		auto guardedArea = object.instances().back()->getAccessibleArea();
		auto areaToBlock = object.getAccessibleArea(true);
		areaToBlock.subtract(guardedArea);
		zone.areaPossible().subtract(areaToBlock);
		for(auto & i : areaToBlock.getTiles())
			if(map.isOnMap(i) && map.isPossible(i))
				map.setOccupied(i, ETileType::BLOCKED);
	}
	
	if(updateDistance)
		updateDistances(object.getPosition());
	
	//zone.getModificator<RoadPlacer>()->addRoadNode(object.getVisitablePosition());
	//return;
	
	switch(object.instances().front()->object().ID)
	{
		case Obj::TOWN:
		case Obj::RANDOM_TOWN:
		case Obj::MONOLITH_TWO_WAY:
		case Obj::MONOLITH_ONE_WAY_ENTRANCE:
		case Obj::MONOLITH_ONE_WAY_EXIT:
		case Obj::SUBTERRANEAN_GATE:
		case Obj::SHIPYARD:
			zone.getModificator<RoadPlacer>()->addRoadNode(object.getVisitablePosition());
			break;
			
		default:
			break;
	}
}

void ObjectManager::placeObject(CGObjectInstance* object, const int3 &pos, bool updateDistance)
{
	Rmg::Object rmgObject(*object);
	rmgObject.setPosition(pos);
	rmgObject.finalize(map);
	zone.areaPossible().subtract(rmgObject.getArea());
	zone.areaUsed().unite(rmgObject.getArea());
	zone.areaUsed().erase(rmgObject.getVisitablePosition());
	
	if(updateDistance)
		updateDistances(pos);
	
	//return;
	
	switch(object->ID)
	{
		case Obj::TOWN:
		case Obj::RANDOM_TOWN:
		case Obj::MONOLITH_TWO_WAY:
		case Obj::MONOLITH_ONE_WAY_ENTRANCE:
		case Obj::MONOLITH_ONE_WAY_EXIT:
		case Obj::SUBTERRANEAN_GATE:
		case Obj::SHIPYARD:
		{
			zone.getModificator<RoadPlacer>()->addRoadNode(object->visitablePos());
		}
			break;
			
		default:
			break;
	}
}

CGCreature * ObjectManager::chooseGuard(si32 strength, bool zoneGuard)
{
	//precalculate actual (randomized) monster strength based on this post
	//http://forum.vcmi.eu/viewtopic.php?p=12426#12426
	
	int mapMonsterStrength = map.getMapGenOptions().getMonsterStrength();
	int monsterStrength = (zoneGuard ? 0 : zone.zoneMonsterStrength) + mapMonsterStrength - 1; //array index from 0 to 4
	static const int value1[] = {2500, 1500, 1000, 500, 0};
	static const int value2[] = {7500, 7500, 7500, 5000, 5000};
	static const float multiplier1[] = {0.5, 0.75, 1.0, 1.5, 1.5};
	static const float multiplier2[] = {0.5, 0.75, 1.0, 1.0, 1.5};
	
	int strength1 = static_cast<int>(std::max(0.f, (strength - value1[monsterStrength]) * multiplier1[monsterStrength]));
	int strength2 = static_cast<int>(std::max(0.f, (strength - value2[monsterStrength]) * multiplier2[monsterStrength]));
	
	strength = strength1 + strength2;
	if (strength < 100/*gen.getConfig().minGuardStrength*/) //TODO: use config
		return nullptr; //no guard at all
	
	CreatureID creId = CreatureID::NONE;
	int amount = 0;
	std::vector<CreatureID> possibleCreatures;
	for(auto cre : VLC->creh->objects)
	{
		if(cre->special)
			continue;
		if(!cre->AIValue) //bug #2681
			continue;
		if(!vstd::contains(zone.getMonsterTypes(), cre->faction))
			continue;
		if(((si32)(cre->AIValue * (cre->ammMin + cre->ammMax) / 2) < strength) && (strength < (si32)cre->AIValue * 100)) //at least one full monster. size between average size of given stack and 100
		{
			possibleCreatures.push_back(cre->idNumber);
		}
	}
	if(possibleCreatures.size())
	{
		creId = *RandomGeneratorUtil::nextItem(possibleCreatures, generator);
		amount = strength / VLC->creh->objects[creId]->AIValue;
		if (amount >= 4)
			amount = static_cast<int>(amount * generator.nextDouble(0.75, 1.25));
	}
	else //just pick any available creature
	{
		creId = CreatureID(132); //Azure Dragon
		amount = strength / VLC->creh->objects[creId]->AIValue;
	}
	
	auto guardFactory = VLC->objtypeh->getHandlerFor(Obj::MONSTER, creId);
	
	auto guard = (CGCreature *) guardFactory->create(ObjectTemplate());
	guard->character = CGCreature::HOSTILE;
	auto  hlp = new CStackInstance(creId, amount);
	//will be set during initialization
	guard->putStack(SlotID(0), hlp);
	return guard;
}

bool ObjectManager::addGuard(Rmg::Object & object, si32 str, bool zoneGuard)
{
	auto * guard = chooseGuard(str, zoneGuard);
	if(!guard)
		return false;
	
	Rmg::Area visitablePos({object.getVisitablePosition()});
	visitablePos.unite(visitablePos.getBorderOutside());
	
	auto accessibleArea = object.getAccessibleArea();
	accessibleArea.intersect(visitablePos);
	if(accessibleArea.empty())
	{
		delete guard;
		return false;
	}
	auto guardTiles = accessibleArea.getTilesVector();
	auto guardPos = *std::min_element(guardTiles.begin(), guardTiles.end(), [&object](const int3 & l, const int3 & r)
	{
		auto p = object.getVisitablePosition();
		if(l.y > r.y)
			return true;
		
		if(l.y == r.y)
			return abs(l.x - p.x) < abs(r.x - p.x);
		
		return false;
	});
	
	auto & instance = object.addInstance(*guard);
	instance.setPosition(guardPos - object.getPosition());
		
	return true;
}

bool ObjectManager::guardObject(CGObjectInstance* object, si32 str, bool zoneGuard, bool addToFreePaths)
{
	std::vector<int3> tiles = getAccessibleOffsets(object);
	
	int3 guardTile(-1, -1, -1);
	
	if(tiles.size())
	{
		//guardTile = tiles.front();
		guardTile = getAccessibleOffset(object->appearance, object->pos);
		logGlobal->trace("Guard object at %s", object->pos.toString());
	}
	else
	{
		logGlobal->error("Failed to guard object at %s", object->pos.toString());
		return false;
	}
	
	if(addMonster(guardTile, str, false, zoneGuard)) //do not place obstacles around unguarded object
	{
		for(auto pos : tiles)
		{
			if(map.isPossible(pos) && map.getZoneID(zone.getPos()) == zone.getId())
				map.setOccupied(pos, ETileType::BLOCKED);
		}
		map.foreach_neighbour(guardTile, [&](int3& pos)
		{
			if(map.isPossible(pos) && map.getZoneID(zone.getPos()) == zone.getId())
				map.setOccupied(pos, ETileType::FREE);
		});
		
		map.setOccupied (guardTile, ETileType::USED);
	}
	else //allow no guard or other object in front of this object
	{
		for(auto tile : tiles)
			if(map.isPossible(tile))
				map.setOccupied(tile, ETileType::FREE);
	}
	
	return true;
}

bool ObjectManager::addMonster(int3 &pos, si32 strength, bool clearSurroundingTiles, bool zoneGuard)
{
	//precalculate actual (randomized) monster strength based on this post
	//http://forum.vcmi.eu/viewtopic.php?p=12426#12426
	
	int mapMonsterStrength = map.getMapGenOptions().getMonsterStrength();
	int monsterStrength = (zoneGuard ? 0 : zone.zoneMonsterStrength) + mapMonsterStrength - 1; //array index from 0 to 4
	static const int value1[] = {2500, 1500, 1000, 500, 0};
	static const int value2[] = {7500, 7500, 7500, 5000, 5000};
	static const float multiplier1[] = {0.5, 0.75, 1.0, 1.5, 1.5};
	static const float multiplier2[] = {0.5, 0.75, 1.0, 1.0, 1.5};
	
	int strength1 = static_cast<int>(std::max(0.f, (strength - value1[monsterStrength]) * multiplier1[monsterStrength]));
	int strength2 = static_cast<int>(std::max(0.f, (strength - value2[monsterStrength]) * multiplier2[monsterStrength]));
	
	strength = strength1 + strength2;
	//if (strength < gen.getConfig().minGuardStrength)
	//	return false; //no guard at all
	
	CreatureID creId = CreatureID::NONE;
	int amount = 0;
	std::vector<CreatureID> possibleCreatures;
	for(auto cre : VLC->creh->objects)
	{
		if(cre->special)
			continue;
		if(!cre->AIValue) //bug #2681
			continue;
		if(!vstd::contains(zone.getMonsterTypes(), cre->faction))
			continue;
		if(((si32)(cre->AIValue * (cre->ammMin + cre->ammMax) / 2) < strength) && (strength < (si32)cre->AIValue * 100)) //at least one full monster. size between average size of given stack and 100
		{
			possibleCreatures.push_back(cre->idNumber);
		}
	}
	if(possibleCreatures.size())
	{
		creId = *RandomGeneratorUtil::nextItem(possibleCreatures, generator);
		amount = strength / VLC->creh->objects[creId]->AIValue;
		if (amount >= 4)
			amount = static_cast<int>(amount * generator.nextDouble(0.75, 1.25));
	}
	else //just pick any available creature
	{
		creId = CreatureID(132); //Azure Dragon
		amount = strength / VLC->creh->objects[creId]->AIValue;
	}
	
	auto guardFactory = VLC->objtypeh->getHandlerFor(Obj::MONSTER, creId);
	
	auto guard = (CGCreature *) guardFactory->create(ObjectTemplate());
	guard->character = CGCreature::HOSTILE;
	auto  hlp = new CStackInstance(creId, amount);
	//will be set during initialization
	guard->putStack(SlotID(0), hlp);
	
	placeObject(guard, pos, true);
	
	if(clearSurroundingTiles)
	{
		//do not spawn anything near monster
		map.foreach_neighbour(pos, [this](int3 pos)
		{
			if(map.isPossible(pos))
				map.setOccupied(pos, ETileType::FREE);
		});
	}
	
	return true;
}
