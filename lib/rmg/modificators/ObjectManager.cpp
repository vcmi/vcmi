/*
 * ObjectManager.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ObjectManager.h"
#include "../CMapGenerator.h"
#include "../TileInfo.h"
#include "../RmgMap.h"
#include "RoadPlacer.h"
#include "RiverPlacer.h"
#include "WaterAdopter.h"
#include "ConnectionsPlacer.h"
#include "TownPlacer.h"
#include "MinePlacer.h"
#include "QuestArtifactPlacer.h"
#include "../../CCreatureHandler.h"
#include "../../mapObjectConstructors/AObjectTypeHandler.h"
#include "../../mapObjectConstructors/CObjectClassesHandler.h"
#include "../../mapObjects/CGCreature.h"
#include "../../mapping/CMap.h"
#include "../../mapping/CMapEditManager.h"
#include "../Functions.h"
#include "../RmgObject.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

void ObjectManager::process()
{
	zone.fractalize();
	createRequiredObjects();
}

void ObjectManager::init()
{
	DEPENDENCY(WaterAdopter);

	//Monoliths can be placed by other zone, too
	// Consider only connected zones
	auto id = zone.getId();
	std::set<TRmgTemplateZoneId> connectedZones;
	for(auto c : map.getMapGenOptions().getMapTemplate()->getConnectedZoneIds())
	{
		// Only consider connected zones
		if (c.getZoneA() == id || c.getZoneB() == id)
		{
			connectedZones.insert(c.getZoneA());
			connectedZones.insert(c.getZoneB());
		}
	}
	auto zones = map.getZones();
	for (auto zoneId : connectedZones)
	{		
		auto * cp = zones.at(zoneId)->getModificator<ConnectionsPlacer>();
		if (cp)
		{
			dependency(cp);
		}
	}

	DEPENDENCY(TownPlacer); //Only secondary towns
	DEPENDENCY(MinePlacer);
	POSTFUNCTION(RoadPlacer);
	createDistancesPriorityQueue();
}

void ObjectManager::createDistancesPriorityQueue()
{
	const auto tiles = zone.areaPossible()->getTilesVector();

	RecursiveLock lock(externalAccessMutex);
	tilesByDistance.clear();
	for(const auto & tile : tiles)
	{
		tilesByDistance.push(std::make_pair(tile, map.getNearestObjectDistance(tile)));
	}
}

void ObjectManager::addRequiredObject(const RequiredObjectInfo & info)
{
	RecursiveLock lock(externalAccessMutex);
	requiredObjects.emplace_back(info);
}

void ObjectManager::addCloseObject(const RequiredObjectInfo & info)
{
	RecursiveLock lock(externalAccessMutex);
	closeObjects.emplace_back(info);
}

void ObjectManager::addNearbyObject(const RequiredObjectInfo & info)
{
	RecursiveLock lock(externalAccessMutex);
	nearbyObjects.emplace_back(info);
}

void ObjectManager::updateDistances(const rmg::Object & obj)
{
	updateDistances([obj](const int3& tile) -> ui32
	{
		return obj.getArea().distanceSqr(tile); //optimization, only relative distance is interesting
	});
}

void ObjectManager::updateDistances(const int3 & pos)
{
	updateDistances([pos](const int3& tile) -> ui32
	{
		return pos.dist2dSQ(tile); //optimization, only relative distance is interesting
	});
}

void ObjectManager::updateDistances(std::function<ui32(const int3 & tile)> distanceFunction)
{
	// Workaround to avoid deadlock when accessed from other zone
	RecursiveLock lock(zone.areaMutex, boost::try_to_lock);
	if (!lock.owns_lock())
	{
		// Unsolvable problem of mutual access
		return;
	}

	const auto tiles = zone.areaPossible()->getTilesVector();
	//RecursiveLock lock(externalAccessMutex);
	tilesByDistance.clear();

	for (const auto & tile : tiles) //don't need to mark distance for not possible tiles
	{
		ui32 d = distanceFunction(tile);
		map.setNearestObjectDistance(tile, std::min(static_cast<float>(d), map.getNearestObjectDistance(tile)));
		tilesByDistance.push(std::make_pair(tile, map.getNearestObjectDistance(tile)));
	}
}

const rmg::Area & ObjectManager::getVisitableArea() const
{
	RecursiveLock lock(externalAccessMutex);
	return objectsVisitableArea;
}

std::vector<CGObjectInstance*> ObjectManager::getMines() const
{
	std::vector<CGObjectInstance*> mines;

	RecursiveLock lock(externalAccessMutex);
	for(auto * object : objects)
	{
		if (object->ID == Obj::MINE)
		{
			mines.push_back(object);
		}
	}

	return mines;
}

int3 ObjectManager::findPlaceForObject(const rmg::Area & searchArea, rmg::Object & obj, const std::function<float(const int3)> & weightFunction, OptimizeType optimizer) const
{
	float bestWeight = 0.f;
	int3 result(-1, -1, -1);

	//Blocked area might not cover object position if it has an offset from (0,0)
	auto outsideTheMap = [this, &obj]() -> bool
	{
		for (const auto& oi : obj.instances())
		{
			if (!map.isOnMap(oi->getPosition(true)))
			{
				return true;
			}
		}
		return false;
	};
	
	if(optimizer & OptimizeType::DISTANCE)
	{
		// Do not add or remove tiles while we iterate on them
		//RecursiveLock lock(externalAccessMutex);
		auto open = tilesByDistance;

		while(!open.empty())
		{
			auto node = open.top();
			open.pop();
			int3 tile = node.first;
			
			if(!searchArea.contains(tile))
				continue;
			
			obj.setPosition(tile);

			if (obj.getVisibleTop().y < 0)
				continue;
			
			if(!searchArea.contains(obj.getArea()) || !searchArea.overlap(obj.getAccessibleArea()))
				continue;

			if (outsideTheMap())
				continue;
			
			float weight = weightFunction(tile);
			if(weight > bestWeight)
			{
				bestWeight = weight;
				result = tile;
				if(!(optimizer & OptimizeType::WEIGHT))
					break;
			}
		}
	}
	else
	{
		for(const auto & tile : searchArea.getTilesVector())
		{
			obj.setPosition(tile);

			if (obj.getVisibleTop().y < 0)
				continue;
					
			if(!searchArea.contains(obj.getArea()) || !searchArea.overlap(obj.getAccessibleArea()))
				continue;

			if (outsideTheMap())
				continue;
			
			float weight = weightFunction(tile);
			if(weight > bestWeight)
			{
				bestWeight = weight;
				result = tile;
				if(!(optimizer & OptimizeType::WEIGHT))
					break;
			}
		}
	}
	
	if(result.valid())
		obj.setPosition(result);
	return result;
}

int3 ObjectManager::findPlaceForObject(const rmg::Area & searchArea, rmg::Object & obj, si32 min_dist, OptimizeType optimizer) const
{
	return findPlaceForObject(searchArea, obj, [this, min_dist, &obj](const int3 & tile)
	{
		auto ti = map.getTileInfo(tile);
		float dist = ti.getNearestObjectDistance();
		if(dist < min_dist)
			return -1.f;

		for(const auto & t : obj.getArea().getTilesVector())
		{
			auto localDist = map.getTileInfo(t).getNearestObjectDistance();
			if (localDist < min_dist)
			{
				return -1.f;
			}
			else
			{
				vstd::amin(dist, localDist); //Evaluate object tile which will be closest to another object
			}
		}
		
		return dist;
	}, optimizer);
}

rmg::Path ObjectManager::placeAndConnectObject(const rmg::Area & searchArea, rmg::Object & obj, si32 min_dist, bool isGuarded, bool onlyStraight, OptimizeType optimizer) const
{
	return placeAndConnectObject(searchArea, obj, [this, min_dist, &obj](const int3 & tile)
	{
		float bestDistance = 10e9;
		for(const auto & t : obj.getArea().getTilesVector())
		{
			float distance = map.getTileInfo(t).getNearestObjectDistance();
			if(distance < min_dist)
				return -1.f;
			else
				vstd::amin(bestDistance, distance);
		}
		
		rmg::Area perimeter;
		rmg::Area areaToBlock;
		if (obj.isGuarded())
		{
			auto guardedArea = obj.instances().back()->getAccessibleArea();
			guardedArea.add(obj.instances().back()->getVisitablePosition());
			areaToBlock = obj.getAccessibleArea(true);
			areaToBlock.subtract(guardedArea);
		
			if (!areaToBlock.empty())
			{
				perimeter = areaToBlock;
				perimeter.unite(areaToBlock.getBorderOutside());
				//We could have added border around guard
				perimeter.subtract(guardedArea);
			}
		}
		else
		{
			perimeter = obj.getArea();
			perimeter.subtract(obj.getAccessibleArea());
			if (!perimeter.empty())
			{
				perimeter.unite(perimeter.getBorderOutside());
				perimeter.subtract(obj.getAccessibleArea());
			}
		}
		//Check if perimeter of the object intersects with more than one blocked areas

		auto tiles = perimeter.getTiles();
		vstd::erase_if(tiles, [this](const int3& tile) -> bool
		{
			//Out-of-map area also is an obstacle
			if (!map.isOnMap(tile))
				return false;
			return !(map.isBlocked(tile) || map.isUsed(tile));
		});

		if (!tiles.empty())
		{
			rmg::Area border(tiles);
			border.subtract(areaToBlock);
			if (!border.connected())
			{
				//We don't want to connect two blocked areas to create impassable obstacle
				return -1.f;
			}
		}
		
		return bestDistance;
	}, isGuarded, onlyStraight, optimizer);
}

rmg::Path ObjectManager::placeAndConnectObject(const rmg::Area & searchArea, rmg::Object & obj, const std::function<float(const int3)> & weightFunction, bool isGuarded, bool onlyStraight, OptimizeType optimizer) const
{
	int3 pos;
	auto possibleArea = searchArea;
	auto cachedArea = zone.areaPossible() + zone.freePaths();
	while(true)
	{
		pos = findPlaceForObject(possibleArea, obj, weightFunction, optimizer);
		if(!pos.valid())
		{
			return rmg::Path::invalid();
		}
		possibleArea.erase(pos); //do not place again at this point
		auto accessibleArea = obj.getAccessibleArea(isGuarded) * cachedArea;
		//we should exclude tiles which will be covered
		if(isGuarded)
		{
			const auto & guardedArea = obj.instances().back()->getAccessibleArea();
			accessibleArea.intersect(guardedArea);
			accessibleArea.add(obj.instances().back()->getPosition(true));
		}

		rmg::Area subArea;
		if (isGuarded)
		{
			const auto & guardedArea = obj.instances().back()->getAccessibleArea();
			const auto & unguardedArea = obj.getAccessibleArea(isGuarded);
			subArea = cachedArea.getSubarea([guardedArea, unguardedArea, obj](const int3 & t)
			{
				if(unguardedArea.contains(t) && !guardedArea.contains(t))
					return false;
				
				//guard position is always target
				if(obj.instances().back()->getPosition(true) == t)
					return true;

				return !obj.getArea().contains(t);
			});
		}
		else
		{
			subArea = cachedArea.getSubarea([obj](const int3 & t)
			{
				return !obj.getArea().contains(t);
			});
		}
		auto path = zone.searchPath(accessibleArea, onlyStraight, subArea);
		
		if(path.valid())
		{
			return path;
		}
	}
}

bool ObjectManager::createMonoliths()
{
	// Special case for Junction zone only
	logGlobal->trace("Creating Monoliths");
	for(const auto & objInfo : requiredObjects)
	{
		if (objInfo.obj->ID != Obj::MONOLITH_TWO_WAY)
		{
			continue;
		}

		rmg::Object rmgObject(*objInfo.obj);
		rmgObject.setTemplate(zone.getTerrainType(), zone.getRand());
		bool guarded = addGuard(rmgObject, objInfo.guardStrength, true);

		Zone::Lock lock(zone.areaMutex);
		auto path = placeAndConnectObject(zone.areaPossible().get(), rmgObject, 3, guarded, false, OptimizeType::DISTANCE);
		
		if(!path.valid())
		{
			logGlobal->error("Failed to fill zone %d due to lack of space", zone.getId());
			return false;
		}
		
		zone.connectPath(path);
		placeObject(rmgObject, guarded, true, objInfo.createRoad);
	}

	vstd::erase_if(requiredObjects, [](const auto & objInfo)
	{
		return  objInfo.obj->ID == Obj::MONOLITH_TWO_WAY;
	});
	return true;
}

bool ObjectManager::createRequiredObjects()
{
	logGlobal->trace("Creating required objects");
	
	RecursiveLock lock(externalAccessMutex); //In case someone adds more objects
	for(const auto & objInfo : requiredObjects)
	{
		rmg::Object rmgObject(*objInfo.obj);
		rmgObject.setTemplate(zone.getTerrainType(), zone.getRand());
		bool guarded = addGuard(rmgObject, objInfo.guardStrength, (objInfo.obj->ID == Obj::MONOLITH_TWO_WAY));

		Zone::Lock lock(zone.areaMutex);
		auto path = placeAndConnectObject(zone.areaPossible().get(), rmgObject, 3, guarded, false, OptimizeType::DISTANCE);
		
		if(!path.valid())
		{
			logGlobal->error("Failed to fill zone %d due to lack of space", zone.getId());
			return false;
		}
		
		zone.connectPath(path);
		placeObject(rmgObject, guarded, true, objInfo.createRoad);
		
		for(const auto & nearby : nearbyObjects)
		{
			if(nearby.nearbyTarget != nearby.obj)
				continue;
			
			rmg::Object rmgNearObject(*nearby.obj);
			rmg::Area possibleArea(rmgObject.instances().front()->getBlockedArea().getBorderOutside());
			possibleArea.intersect(zone.areaPossible().get());
			if(possibleArea.empty())
			{
				rmgNearObject.clear();
				continue;
			}
			
			rmgNearObject.setPosition(*RandomGeneratorUtil::nextItem(possibleArea.getTiles(), zone.getRand()));
			placeObject(rmgNearObject, false, false, nearby.createRoad);
		}
	}
	
	for(const auto & objInfo : closeObjects)
	{
		Zone::Lock lock(zone.areaMutex);

		rmg::Object rmgObject(*objInfo.obj);
		rmgObject.setTemplate(zone.getTerrainType(), zone.getRand());
		bool guarded = addGuard(rmgObject, objInfo.guardStrength, (objInfo.obj->ID == Obj::MONOLITH_TWO_WAY));
		auto path = placeAndConnectObject(zone.areaPossible().get(), rmgObject,
										  [this, &rmgObject](const int3 & tile)
		{
			float dist = rmgObject.getArea().distanceSqr(zone.getPos());
			dist *= (dist > 12.f * 12.f) ? 10.f : 1.f; //tiles closer 12 are preferable
			dist = 1000000.f - dist; //some big number
			return dist + map.getNearestObjectDistance(tile);
		}, guarded, false, OptimizeType::WEIGHT);
		
		if(!path.valid())
		{
			logGlobal->error("Failed to fill zone %d due to lack of space", zone.getId());
			return false;
		}
		
		zone.connectPath(path);
		placeObject(rmgObject, guarded, true);
	}

	for(const auto & nearby : nearbyObjects)
	{
		auto * targetObject = nearby.nearbyTarget;
		if (!targetObject || !targetObject->appearance)
		{
			continue;
		}

		rmg::Object rmgNearObject(*nearby.obj);
		std::set<int3> blockedArea = targetObject->getBlockedPos();
		rmg::Area areaForObject(rmg::Area(rmg::Tileset(blockedArea.begin(), blockedArea.end())).getBorderOutside());
		areaForObject.intersect(zone.areaPossible().get());
		if(areaForObject.empty())
		{
			rmgNearObject.clear();
			continue;
		}

		rmgNearObject.setPosition(*RandomGeneratorUtil::nextItem(areaForObject.getTiles(), zone.getRand()));
		placeObject(rmgNearObject, false, false);
		auto path = zone.searchPath(rmgNearObject.getVisitablePosition(), false);
		if (path.valid())
		{
			zone.connectPath(path);
		}
		else
		{
			for (auto* instance : rmgNearObject.instances())
			{
				logGlobal->error("Failed to connect nearby object %s at %s",
					instance->object().getObjectName(), instance->getPosition(true).toString());
				mapProxy->removeObject(&instance->object());
			}
		}
	}
	
	//create object on specific positions
	//TODO: implement guards
	for (const auto &objInfo : instantObjects) //Unused ATM
	{
		rmg::Object rmgObject(*objInfo.obj);
		rmgObject.setPosition(objInfo.pos);
		placeObject(rmgObject, false, false);
	}
	
	requiredObjects.clear();
	closeObjects.clear();
	nearbyObjects.clear();
	instantObjects.clear();
	
	return true;
}

void ObjectManager::placeObject(rmg::Object & object, bool guarded, bool updateDistance, bool createRoad/* = false*/)
{	
	if (object.instances().size() == 1 && object.instances().front()->object().ID == Obj::MONSTER)
	{
		//Fix for HoTA offset - lonely guards
		
		auto monster = object.instances().front();
		if (!monster->object().appearance)
		{
			//Needed to determine visitable offset
			monster->setAnyTemplate(zone.getRand());
		}
		object.getPosition();
		auto visitableOffset = monster->object().getVisitableOffset();
		auto fixedPos = monster->getPosition(true) + visitableOffset;

		//Do not place guard outside the map
		vstd::abetween(fixedPos.x, visitableOffset.x, map.width() - 1);
		vstd::abetween(fixedPos.y, visitableOffset.y, map.height() - 1);
		int3 parentOffset = monster->getPosition(true) - monster->getPosition(false);
		monster->setPosition(fixedPos - parentOffset);
	}
	object.finalize(map, zone.getRand());

	{
		Zone::Lock lock(zone.areaMutex);

		zone.areaPossible()->subtract(object.getArea());
		bool keepVisitable = zone.freePaths()->contains(object.getVisitablePosition());
		zone.freePaths()->subtract(object.getArea()); //just to avoid areas overlapping
		if(keepVisitable)
			zone.freePaths()->add(object.getVisitablePosition());
		zone.areaUsed()->unite(object.getArea());
		zone.areaUsed()->erase(object.getVisitablePosition());

		if(guarded) //We assume the monster won't be guarded
		{
			auto guardedArea = object.instances().back()->getAccessibleArea();
			guardedArea.add(object.instances().back()->getVisitablePosition());
			auto areaToBlock = object.getAccessibleArea(true);
			areaToBlock.subtract(guardedArea);
			zone.areaPossible()->subtract(areaToBlock);
			for(const auto & i : areaToBlock.getTilesVector())
				if(map.isOnMap(i) && map.isPossible(i))
					map.setOccupied(i, ETileType::BLOCKED);
		}
	}

	if (updateDistance)
	{
		//Update distances in every adjacent zone (including this one) in case of wide connection

		std::set<TRmgTemplateZoneId> adjacentZones;
		auto objectArea = object.getArea();
		objectArea.unite(objectArea.getBorderOutside());
		
		for (auto tile : objectArea.getTilesVector())
		{
			if (map.isOnMap(tile))
			{
				adjacentZones.insert(map.getZoneID(tile));
			}
		}

		for (auto id : adjacentZones)
		{
			auto otherZone = map.getZones().at(id);
			if ((otherZone->getType() == ETemplateZoneType::WATER) == (zone.getType()	== ETemplateZoneType::WATER))
			{
				// Do not update other zone if only one is water
				auto manager = otherZone->getModificator<ObjectManager>();
				if (manager)
				{
					manager->updateDistances(object);
				}
			}
		}
	}
	
	// TODO: Add multiple tiles in one operation to avoid multiple invalidation
	for(auto * instance : object.instances())
	{
		objectsVisitableArea.add(instance->getVisitablePosition());
		objects.push_back(&instance->object());
		if(auto * rp = zone.getModificator<RoadPlacer>())
		{
			if (instance->object().blockVisit && !instance->object().removable)
			{
				//Cannot be trespassed (Corpse)
				continue;
			}
			else if(instance->object().appearance->isVisitableFromTop())
			{
				//Passable objects
				rp->areaForRoads().add(instance->getVisitablePosition());
			}
			else if(!instance->object().appearance->isVisitableFromTop())
			{
				// Do not route road behind visitable tile
				auto borderAbove = instance->getBorderAbove();
				rp->areaIsolated().unite(borderAbove);
			}

			if (object.isGuarded())
			{
				rp->areaVisitable().add(instance->getVisitablePosition());
			}
		}

		switch (instance->object().ID.toEnum())
		{
			case Obj::RANDOM_TREASURE_ART:
			case Obj::RANDOM_MINOR_ART: //In OH3 quest artifacts have higher value than normal arts
			case Obj::RANDOM_RESOURCE:
			{
				if (auto * qap = zone.getModificator<QuestArtifactPlacer>())
				{
					qap->rememberPotentialArtifactToReplace(&instance->object());
				}
				break;
			}
			default:
				break;
		}
	}

	if (createRoad)
	{
		if (auto* m = zone.getModificator<RoadPlacer>())
			m->addRoadNode(object.instances().front()->getVisitablePosition());
	}

	//TODO: Add road node to these objects:
	/*
	 	case Obj::MONOLITH_ONE_WAY_ENTRANCE:
	 	case Obj::RANDOM_TOWN:
		case Obj::MONOLITH_ONE_WAY_EXIT:
	*/

	switch(object.instances().front()->object().ID.toEnum())
	{
		case Obj::WATER_WHEEL:
			if (auto* m = zone.getModificator<RiverPlacer>())
				m->addRiverNode(object.instances().front()->getVisitablePosition());
			break;

		default:
			break;
	}
}

CGCreature * ObjectManager::chooseGuard(si32 strength, bool zoneGuard)
{
	//precalculate actual (randomized) monster strength based on this post
	//http://forum.vcmi.eu/viewtopic.php?p=12426#12426
	
	if(!zoneGuard && zone.monsterStrength == EMonsterStrength::ZONE_NONE)
		return nullptr; //no guards inside this zone except for zone guards
	
	int mapMonsterStrength = map.getMapGenOptions().getMonsterStrength();
	int monsterStrength = (zoneGuard ? 0 : zone.monsterStrength - EMonsterStrength::ZONE_NORMAL) + mapMonsterStrength - 1; //array index from 0 to 4
	static const std::array<int, 5> value1{2500, 1500, 1000, 500, 0};
	static const std::array<int, 5> value2{7500, 7500, 7500, 5000, 5000};
	static const std::array<float, 5> multiplier1{0.5, 0.75, 1.0, 1.5, 1.5};
	static const std::array<float, 5> multiplier2{0.5, 0.75, 1.0, 1.0, 1.5};
	
	int strength1 = static_cast<int>(std::max(0.f, (strength - value1.at(monsterStrength)) * multiplier1.at(monsterStrength)));
	int strength2 = static_cast<int>(std::max(0.f, (strength - value2.at(monsterStrength)) * multiplier2.at(monsterStrength)));
	
	strength = strength1 + strength2;
	if (strength < generator.getConfig().minGuardStrength)
		return nullptr; //no guard at all
	
	CreatureID creId = CreatureID::NONE;
	int amount = 0;
	std::vector<CreatureID> possibleCreatures;
	for(auto const & cre : VLC->creh->objects)
	{
		if(cre->special)
			continue;
		if(!cre->getAIValue()) //bug #2681
			continue;
		if(!vstd::contains(zone.getMonsterTypes(), cre->getFactionID()))
			continue;
		if((static_cast<si32>(cre->getAIValue() * (cre->ammMin + cre->ammMax) / 2) < strength) && (strength < static_cast<si32>(cre->getAIValue()) * 100)) //at least one full monster. size between average size of given stack and 100
		{
			possibleCreatures.push_back(cre->getId());
		}
	}
	if(!possibleCreatures.empty())
	{
		creId = *RandomGeneratorUtil::nextItem(possibleCreatures, zone.getRand());
		amount = strength / creId.toEntity(VLC)->getAIValue();
		if (amount >= 4)
			amount = static_cast<int>(amount * zone.getRand().nextDouble(0.75, 1.25));
	}
	else //just pick any available creature
	{
		creId = CreatureID::AZURE_DRAGON; //Azure Dragon
		amount = strength / creId.toEntity(VLC)->getAIValue();
	}
	
	auto guardFactory = VLC->objtypeh->getHandlerFor(Obj::MONSTER, creId);

	auto * guard = dynamic_cast<CGCreature *>(guardFactory->create(map.mapInstance->cb, nullptr));
	guard->character = CGCreature::HOSTILE;
	auto * hlp = new CStackInstance(creId, amount);
	//will be set during initialization
	guard->putStack(SlotID(0), hlp);
	return guard;
}

bool ObjectManager::addGuard(rmg::Object & object, si32 strength, bool zoneGuard)
{
	auto * guard = chooseGuard(strength, zoneGuard);
	if(!guard)
		return false;
	
	// Prefer non-blocking tiles, if any
	auto entrableArea = object.getEntrableArea();
	if (entrableArea.empty())
	{
		entrableArea.add(object.getVisitablePosition());
	}

	rmg::Area entrableBorder = entrableArea.getBorderOutside();
	
	auto accessibleArea = object.getAccessibleArea();
	accessibleArea.erase_if([&](const int3 & tile)
	{
		return !entrableBorder.contains(tile);
	});
	
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
	instance.setAnyTemplate(zone.getRand()); //terrain is irrelevant for monsters, but monsters need some template now

	//Fix HoTA monsters with offset template
	auto visitableOffset = instance.object().getVisitableOffset();
	auto fixedPos = guardPos - object.getPosition() + visitableOffset;
	instance.setPosition(fixedPos);
		
	return true;
}

RequiredObjectInfo::RequiredObjectInfo():
	obj(nullptr),
	nearbyTarget(nullptr),
	guardStrength(0),
	createRoad(true)
{}

RequiredObjectInfo::RequiredObjectInfo(CGObjectInstance* obj, ui32 guardStrength, bool createRoad, CGObjectInstance* nearbyTarget):
	obj(obj),
	nearbyTarget(nearbyTarget),
	guardStrength(guardStrength),
	createRoad(createRoad)
{}

VCMI_LIB_NAMESPACE_END


