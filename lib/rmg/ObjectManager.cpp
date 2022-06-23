//
//  ObjectManager.cpp
//  vcmi
//
//  Created by nordsoft on 23.06.2022.
//

#include "ObjectManager.h"
#include "CMapGenerator.h"
#include "TileInfo.h"
#include "../CCreatureHandler.h"
#include "../mapObjects/CommonConstructors.h"
#include "../mapObjects/MapObjects.h" //needed to resolve templates for CommonConstructors.h
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"

ObjectManager::ObjectManager(Zone & zone, CMapGenerator & gen) : gen(gen), zone(zone)
{
	
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
	for (auto tile : zone.getPossibleTiles()) //don't need to mark distance for not possible tiles
	{
		ui32 d = pos.dist2dSQ(tile); //optimization, only relative distance is interesting
		gen.setNearestObjectDistance(tile, std::min((float)d, gen.getNearestObjectDistance(tile)));
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
					if(gen.map->isInTheMap(nearbyPos))
					{
						if(appearance.isVisitableFrom(x, y) && !gen.isBlocked(nearbyPos) && zone.getTileInfo().count(nearbyPos))
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
	
	gen.foreach_neighbour(visitable, [&](int3& pos)
	{
		if(gen.isPossible(pos) || gen.isFree(pos))
		{
			if(!vstd::contains(tilesBlockedByObject, pos))
			{
				if(object->appearance.isVisitableFrom(pos.x - visitable.x, pos.y - visitable.y) && !gen.isBlocked(pos)) //TODO: refactor - info about visitability from absolute coordinates
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
		if(!gen.map->isInTheMap(t) || !gen.isPossible(t) || gen.getZoneID(t) != zone.getId())
		{
			//if at least one tile is not possible, object can't be placed here
			return false;
		}
	}
	return true;
}

bool ObjectManager::findPlaceForObject(CGObjectInstance* obj, si32 min_dist, int3 &pos)
{
	//we need object apperance to deduce free tile
	setTemplateForObject(obj);
	
	int best_distance = 0;
	bool result = false;
	
	auto tilesBlockedByObject = obj->getBlockedOffsets();
	
	for (auto tile : zone.getTileInfo())
	{
		//object must be accessible from at least one surounding tile
		if (!isAccessibleFromSomewhere(obj->appearance, tile))
			continue;
		
		auto ti = gen.getTile(tile);
		auto dist = ti.getNearestObjectDistance();
		//avoid borders
		if (gen.isPossible(tile) && (dist >= min_dist) && (dist > best_distance))
		{
			if (areAllTilesAvailable(obj, tile, tilesBlockedByObject))
			{
				best_distance = static_cast<int>(dist);
				pos = tile;
				result = true;
			}
		}
	}
	if (result)
	{
		gen.setOccupied(pos, ETileType::BLOCKED); //block that tile
	}
	return result;
}

ObjectManager::EObjectPlacingResult ObjectManager::tryToPlaceObjectAndConnectToPath(CGObjectInstance * obj, const int3 & pos)
{
	//check if we can find a path around this object. Tiles will be set to "USED" after object is successfully placed.
	obj->pos = pos;
	gen.setOccupied(obj->visitablePos(), ETileType::BLOCKED);
	for(auto tile : obj->getBlockedPos())
	{
		if(gen.map->isInTheMap(tile))
			gen.setOccupied(tile, ETileType::BLOCKED);
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

bool ObjectManager::createRequiredObjects()
{
	logGlobal->trace("Creating required objects");
	
	for(const auto &object : requiredObjects)
	{
		auto obj = object.first;
		if(!obj->appearance.canBePlacedAt(zone.getTerrainType()))
			continue;
		
		int3 pos;
		while(true)
		{
			if(!findPlaceForObject(obj, 3, pos))
			{
				logGlobal->error("Failed to fill zone %d due to lack of space", zone.getId());
				return false;
			}
			
			if(tryToPlaceObjectAndConnectToPath(obj, pos) == EObjectPlacingResult::SUCCESS)
			{
				//paths to required objects constitute main paths of zone. otherwise they just may lead to middle and create dead zones
				placeObject(obj, pos);
				guardObject(obj, object.second, (obj->ID == Obj::MONOLITH_TWO_WAY), true);
				break;
			}
		}
	}
	
	for(const auto &obj : closeObjects)
	{
		setTemplateForObject(obj.first);
		if (!obj.first->appearance.canBePlacedAt(zone.getTerrainType()))
			continue;
		
		auto tilesBlockedByObject = obj.first->getBlockedOffsets();
		
		bool finished = false;
		bool attempt = true;
		while (!finished && attempt)
		{
			attempt = false;
			
			std::vector<int3> tiles(zone.getPossibleTiles().begin(), zone.getPossibleTiles().end());
			//new tiles vector after each object has been placed, OR misplaced area has been sealed off
			
			boost::remove_if(tiles, [obj, this](int3 &tile)-> bool
			{
				//object must be accessible from at least one surounding tile
				return !this->isAccessibleFromSomewhere(obj.first->appearance, tile);
			});
			
			auto targetPosition = requestedPositions.find(obj.first)!=requestedPositions.end() ? requestedPositions[obj.first] : zone.getPos();
			// smallest distance to zone center, greatest distance to nearest object
			auto isCloser = [this, &targetPosition, &tilesBlockedByObject](const int3 & lhs, const int3 & rhs) -> bool
			{
				float lDist = std::numeric_limits<float>::max();
				float rDist = std::numeric_limits<float>::max();
				for(int3 t : tilesBlockedByObject)
				{
					t += targetPosition;
					lDist = fmin(lDist, static_cast<float>(t.dist2d(lhs)));
					rDist = fmin(rDist, static_cast<float>(t.dist2d(rhs)));
				}
				lDist *= (lDist > 12) ? 10 : 1; //objects within 12 tile radius are preferred (smaller distance rating)
				rDist *= (rDist > 12) ? 10 : 1;
				
				return (lDist * 0.5f - std::sqrt(gen.getNearestObjectDistance(lhs))) < (rDist * 0.5f - std::sqrt(gen.getNearestObjectDistance(rhs)));
			};
			
			boost::sort(tiles, isCloser);
			
			if (tiles.empty())
			{
				logGlobal->error("Failed to fill zone %d due to lack of space", zone.getId());
				return false;
			}
			for (auto tile : tiles)
			{
				//code partially adapted from findPlaceForObject()
				if(!areAllTilesAvailable(obj.first, tile, tilesBlockedByObject))
					continue;
				
				attempt = true;
				
				EObjectPlacingResult result = tryToPlaceObjectAndConnectToPath(obj.first, tile);
				if (result == EObjectPlacingResult::SUCCESS)
				{
					placeObject(obj.first, tile);
					guardObject(obj.first, obj.second, (obj.first->ID == Obj::MONOLITH_TWO_WAY), true);
					finished = true;
					break;
				}
				else if (result == EObjectPlacingResult::CANNOT_FIT)
					continue; // next tile
				else if (result == EObjectPlacingResult::SEALED_OFF)
				{
					break; //tiles expired, pick new ones
				}
				else
					throw (rmgException("Wrong result of tryToPlaceObjectAndConnectToPath()"));
			}
		}
	}
	
	//create nearby objects (e.g. extra resources close to mines)
	for(const auto & object : nearbyObjects)
	{
		auto obj = object.first;
		std::set<int3> possiblePositions;
		for (auto blockedTile : object.second->getBlockedPos())
		{
			gen.foreachDirectNeighbour(blockedTile, [this, &possiblePositions](int3 pos)
										{
				if (!gen.isBlocked(pos) && zone.getTileInfo().count(pos))
				{
					//some resources still could be unaccessible, at least one free cell shall be
					gen.foreach_neighbour(pos, [this, &possiblePositions, &pos](int3 p)
										   {
						if(gen.isFree(p))
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
			auto pos = *RandomGeneratorUtil::nextItem(possiblePositions, gen.rand);
			placeObject(obj, pos);
		}
	}
	
	//create object on specific positions
	//TODO: implement guards
	for (const auto &obj : instantObjects)
	{
		if(tryToPlaceObjectAndConnectToPath(obj.first, obj.second)==EObjectPlacingResult::SUCCESS)
		{
			placeObject(obj.first, obj.second);
			//TODO: guardObject(...)
		}
	}
	
	requiredObjects.clear();
	closeObjects.clear();
	nearbyObjects.clear();
	instantObjects.clear();
	
	return true;
}

void ObjectManager::checkAndPlaceObject(CGObjectInstance* object, const int3 &pos)
{
	if (!gen.map->isInTheMap(pos))
		throw rmgException(boost::to_string(boost::format("Position of object %d at %s is outside the map") % object->id % pos.toString()));
	object->pos = pos;
	
	if (object->isVisitable() && !gen.map->isInTheMap(object->visitablePos()))
		throw rmgException(boost::to_string(boost::format("Visitable tile %s of object %d at %s is outside the map") % object->visitablePos().toString() % object->id % object->pos.toString()));
	for (auto tile : object->getBlockedPos())
	{
		if (!gen.map->isInTheMap(tile))
			throw rmgException(boost::to_string(boost::format("Tile %s of object %d at %s is outside the map") % tile.toString() % object->id % object->pos.toString()));
	}
	
	if (object->appearance.id == Obj::NO_OBJ)
	{
		auto terrainType = gen.map->getTile(pos).terType;
		auto h = VLC->objtypeh->getHandlerFor(object->ID, object->subID);
		auto templates = h->getTemplates(terrainType);
		if (templates.empty())
			throw rmgException(boost::to_string(boost::format("Did not find graphics for object (%d,%d) at %s (terrain %d)") % object->ID % object->subID % pos.toString() % terrainType));
		
		object->appearance = templates.front();
	}
	
	gen.getEditManager()->insertObject(object);
}

void ObjectManager::placeObject(CGObjectInstance* object, const int3 &pos, bool updateDistance)
{
	checkAndPlaceObject (object, pos);
	
	auto points = object->getBlockedPos();
	if (object->isVisitable())
		points.insert(pos + object->getVisitableOffset());
	points.insert(pos);
	for(auto p : points)
	{
		if (gen.map->isInTheMap(p))
		{
			gen.setOccupied(p, ETileType::USED);
		}
	}
	if (updateDistance)
		updateDistances(pos);
	
	switch (object->ID)
	{
		case Obj::TOWN:
		case Obj::RANDOM_TOWN:
		case Obj::MONOLITH_TWO_WAY:
		case Obj::MONOLITH_ONE_WAY_ENTRANCE:
		case Obj::MONOLITH_ONE_WAY_EXIT:
		case Obj::SUBTERRANEAN_GATE:
		case Obj::SHIPYARD:
		{
			//addRoadNode(object->visitablePos());
		}
			break;
			
		default:
			break;
	}
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
			if(gen.isPossible(pos) && gen.getZoneID(zone.getPos()) == zone.getId())
				gen.setOccupied(pos, ETileType::BLOCKED);
		}
		gen.foreach_neighbour(guardTile, [&](int3& pos)
		{
			if(gen.isPossible(pos) && gen.getZoneID(zone.getPos()) == zone.getId())
				gen.setOccupied(pos, ETileType::FREE);
		});
		
		gen.setOccupied (guardTile, ETileType::USED);
	}
	else //allow no guard or other object in front of this object
	{
		for(auto tile : tiles)
			if(gen.isPossible(tile))
				gen.setOccupied(tile, ETileType::FREE);
	}
	
	return true;
}

bool ObjectManager::addMonster(int3 &pos, si32 strength, bool clearSurroundingTiles, bool zoneGuard)
{
	//precalculate actual (randomized) monster strength based on this post
	//http://forum.vcmi.eu/viewtopic.php?p=12426#12426
	
	int mapMonsterStrength = gen.getMapGenOptions().getMonsterStrength();
	int monsterStrength = (zoneGuard ? 0 : zone.zoneMonsterStrength) + mapMonsterStrength - 1; //array index from 0 to 4
	static const int value1[] = {2500, 1500, 1000, 500, 0};
	static const int value2[] = {7500, 7500, 7500, 5000, 5000};
	static const float multiplier1[] = {0.5, 0.75, 1.0, 1.5, 1.5};
	static const float multiplier2[] = {0.5, 0.75, 1.0, 1.0, 1.5};
	
	int strength1 = static_cast<int>(std::max(0.f, (strength - value1[monsterStrength]) * multiplier1[monsterStrength]));
	int strength2 = static_cast<int>(std::max(0.f, (strength - value2[monsterStrength]) * multiplier2[monsterStrength]));
	
	strength = strength1 + strength2;
	if (strength < gen.getConfig().minGuardStrength)
		return false; //no guard at all
	
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
		creId = *RandomGeneratorUtil::nextItem(possibleCreatures, gen.rand);
		amount = strength / VLC->creh->objects[creId]->AIValue;
		if (amount >= 4)
			amount = static_cast<int>(amount * gen.rand.nextDouble(0.75, 1.25));
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
	
	placeObject(guard, pos);
	
	if(clearSurroundingTiles)
	{
		//do not spawn anything near monster
		gen.foreach_neighbour (pos, [this](int3 pos)
		{
			if(gen.isPossible(pos))
				gen.setOccupied(pos, ETileType::FREE);
		});
	}
	
	return true;
}
