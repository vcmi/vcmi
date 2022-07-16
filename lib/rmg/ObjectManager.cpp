/*
 * ObjectManager.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "ObjectManager.h"
#include "CMapGenerator.h"
#include "TileInfo.h"
#include "RmgMap.h"
#include "RoadPlacer.h"
#include "RiverPlacer.h"
#include "WaterAdopter.h"
#include "../CCreatureHandler.h"
#include "../mapObjects/CommonConstructors.h"
#include "../mapObjects/MapObjects.h" //needed to resolve templates for CommonConstructors.h
#include "../mapping/CMap.h"
#include "../mapping/CMapEditManager.h"
#include "Functions.h"
#include "RmgObject.h"

void ObjectManager::process()
{
	zone.fractalize();
	createRequiredObjects();
}

void ObjectManager::init()
{
	dependency(zone.getModificator<WaterAdopter>());
	postfunction(zone.getModificator<RoadPlacer>());
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

void ObjectManager::updateDistances(const rmg::Object & obj)
{
	for (auto tile : zone.areaPossible().getTiles()) //don't need to mark distance for not possible tiles
	{
		ui32 d = obj.getArea().distanceSqr(tile); //optimization, only relative distance is interesting
		map.setNearestObjectDistance(tile, std::min((float)d, map.getNearestObjectDistance(tile)));
	}
}

const rmg::Area & ObjectManager::getVisitableArea() const
{
	return objectsVisitableArea;
}

int3 ObjectManager::findPlaceForObject(const rmg::Area & searchArea, rmg::Object & obj, std::function<float(const int3)> weightFunction, bool optimizer) const
{
	float bestWeight = 0.f;
	int3 result(-1, -1, -1);
	
	for(const auto & tile : searchArea.getTiles())
	{
		obj.setPosition(tile);
		
		if(!searchArea.contains(obj.getArea()) || !searchArea.overlap(obj.getAccessibleArea()))
			continue;
		
		float weight = weightFunction(tile);
		if(weight > bestWeight)
		{
			bestWeight = weight;
			result = tile;
			if(!optimizer)
				break;
		}
	}
	if(result.valid())
		obj.setPosition(result);
	return result;
}

int3 ObjectManager::findPlaceForObject(const rmg::Area & searchArea, rmg::Object & obj, si32 min_dist, bool optimizer) const
{
	return findPlaceForObject(searchArea, obj, [this, min_dist](const int3 & tile)
	{
		auto ti = map.getTile(tile);
		float dist = ti.getNearestObjectDistance();
		if(dist < min_dist)
			return -1.f;
		
		return dist;
	}, optimizer);
}

rmg::Path ObjectManager::placeAndConnectObject(const rmg::Area & searchArea, rmg::Object & obj, si32 min_dist, bool isGuarded, bool onlyStraight, bool optimizer) const
{
	return placeAndConnectObject(searchArea, obj, [this, min_dist](const int3 & tile)
	{
		auto ti = map.getTile(tile);
		float dist = ti.getNearestObjectDistance();
		if(dist < min_dist)
			return -1.f;
		
		return dist;
	}, isGuarded, onlyStraight, optimizer);
}

rmg::Path ObjectManager::placeAndConnectObject(const rmg::Area & searchArea, rmg::Object & obj, std::function<float(const int3)> weightFunction, bool isGuarded, bool onlyStraight, bool optimizer) const
{
	int3 pos;
	auto possibleArea = searchArea;
	while(true)
	{
		pos = findPlaceForObject(possibleArea, obj, weightFunction, optimizer);
		if(!pos.valid())
		{
			return rmg::Path::invalid();
		}
		possibleArea.erase(pos); //do not place again at this point
		auto accessibleArea = obj.getAccessibleArea(isGuarded) * (zone.areaPossible() + zone.freePaths());
		//we should exclude tiles which will be covered
		if(isGuarded)
		{
			auto guardedArea = obj.instances().back()->getAccessibleArea();
			zone.areaPossible().subtract(accessibleArea - guardedArea);
			accessibleArea.intersect(guardedArea);
		}
		
		auto path = zone.searchPath(accessibleArea, onlyStraight, [&obj](const int3 & t)
		{
			return !obj.getArea().contains(t);
		});
		
		if(path.valid())
		{
			return path;
		}
	}
}

bool ObjectManager::createRequiredObjects()
{
	logGlobal->trace("Creating required objects");
	
	for(const auto & object : requiredObjects)
	{
		auto * obj = object.first;
		int3 pos;
		rmg::Object rmgObject(*obj);
		rmgObject.setTemplate(zone.getTerrainType());
		bool guarded = addGuard(rmgObject, object.second, (obj->ID == Obj::MONOLITH_TWO_WAY));
		auto path = placeAndConnectObject(zone.areaPossible(), rmgObject, 3, guarded, false, true);
		
		if(!path.valid())
		{
			logGlobal->error("Failed to fill zone %d due to lack of space", zone.getId());
			return false;
		}
		
		zone.connectPath(path);
		placeObject(rmgObject, guarded, true);
		
		for(const auto & nearby : nearbyObjects)
		{
			if(nearby.second != obj)
				continue;
			
			rmg::Object rmgNearObject(*nearby.first);
			rmg::Area possibleArea(rmgObject.instances().front()->getBlockedArea().getBorderOutside());
			possibleArea.intersect(zone.areaPossible());
			if(possibleArea.empty())
			{
				rmgNearObject.clear();
				continue;
			}
			
			rmgNearObject.setPosition(*RandomGeneratorUtil::nextItem(possibleArea.getTiles(), generator.rand));
			placeObject(rmgNearObject, false, false);
		}
	}
	
	for(const auto & object : closeObjects)
	{
		auto * obj = object.first;
		int3 pos;
		auto possibleArea = zone.areaPossible();
		rmg::Object rmgObject(*obj);
		rmgObject.setTemplate(zone.getTerrainType());
		bool guarded = addGuard(rmgObject, object.second, (obj->ID == Obj::MONOLITH_TWO_WAY));
		auto path = placeAndConnectObject(zone.areaPossible(), rmgObject,
										  [this, &rmgObject](const int3 & tile)
		{
			float dist = rmgObject.getArea().distanceSqr(zone.getPos());
			dist *= (dist > 12.f * 12.f) ? 10.f : 1.f; //tiles closer 12 are preferrable
			dist = 1000000.f - dist; //some big number
			return dist + map.getNearestObjectDistance(tile);
		}, guarded, false, true);
		
		if(!path.valid())
		{
			logGlobal->error("Failed to fill zone %d due to lack of space", zone.getId());
			return false;
		}
		
		zone.connectPath(path);
		placeObject(rmgObject, guarded, true);
		
		for(const auto & nearby : nearbyObjects)
		{
			if(nearby.second != obj)
				continue;
			
			rmg::Object rmgNearObject(*nearby.first);
			rmg::Area possibleArea(rmgObject.instances().front()->getBlockedArea().getBorderOutside());
			possibleArea.intersect(zone.areaPossible());
			if(possibleArea.empty())
			{
				rmgNearObject.clear();
				continue;
			}
			
			rmgNearObject.setPosition(*RandomGeneratorUtil::nextItem(possibleArea.getTiles(), generator.rand));
			placeObject(rmgNearObject, false, false);
		}
	}
	
	//create object on specific positions
	//TODO: implement guards
	for (const auto &obj : instantObjects)
	{
		rmg::Object rmgObject(*obj.first);
		rmgObject.setPosition(obj.second);
		placeObject(rmgObject, false, false);
	}
	
	requiredObjects.clear();
	closeObjects.clear();
	nearbyObjects.clear();
	instantObjects.clear();
	
	return true;
}

void ObjectManager::placeObject(rmg::Object & object, bool guarded, bool updateDistance)
{	
	object.finalize(map);
	zone.areaPossible().subtract(object.getArea());
	bool keepVisitable = zone.freePaths().contains(object.getVisitablePosition());
	zone.freePaths().subtract(object.getArea()); //just to avoid areas overlapping
	if(keepVisitable)
		zone.freePaths().add(object.getVisitablePosition());
	zone.areaUsed().unite(object.getArea());
	zone.areaUsed().erase(object.getVisitablePosition());
	
	if(guarded)
	{
		auto guardedArea = object.instances().back()->getAccessibleArea();
		guardedArea.add(object.instances().back()->getVisitablePosition());
		auto areaToBlock = object.getAccessibleArea(true);
		areaToBlock.subtract(guardedArea);
		zone.areaPossible().subtract(areaToBlock);
		for(auto & i : areaToBlock.getTilesVector())
			if(map.isOnMap(i) && map.isPossible(i))
				map.setOccupied(i, ETileType::BLOCKED);
	}
	
	if(updateDistance)
		updateDistances(object);
	
	for(auto * instance : object.instances())
	{
		objectsVisitableArea.add(instance->getVisitablePosition());
		objects.push_back(&instance->object());
		if(auto * m = zone.getModificator<RoadPlacer>())
		{
			if(instance->object().appearance.isVisitableFromTop())
				m->areaForRoads().add(instance->getVisitablePosition());
			else
			{
				rmg::Area isolated({instance->getVisitablePosition()});
				isolated.assign(isolated.getBorderOutside());
				isolated.subtract(instance->getAccessibleArea());
				m->areaIsolated().unite(isolated);
			}
		}
	}
	
	switch(object.instances().front()->object().ID)
	{
		case Obj::TOWN:
		case Obj::RANDOM_TOWN:
		case Obj::MONOLITH_TWO_WAY:
		case Obj::MONOLITH_ONE_WAY_ENTRANCE:
		case Obj::MONOLITH_ONE_WAY_EXIT:
		case Obj::SUBTERRANEAN_GATE:
		case Obj::SHIPYARD:
			if(auto * m = zone.getModificator<RoadPlacer>())
				m->addRoadNode(object.instances().front()->getVisitablePosition());
			break;
			
		case Obj::WATER_WHEEL:
			if(auto * m = zone.getModificator<RiverPlacer>())
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
	
	int mapMonsterStrength = map.getMapGenOptions().getMonsterStrength();
	int monsterStrength = (zoneGuard ? 0 : zone.zoneMonsterStrength) + mapMonsterStrength - 1; //array index from 0 to 4
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
		creId = *RandomGeneratorUtil::nextItem(possibleCreatures, generator.rand);
		amount = strength / VLC->creh->objects[creId]->AIValue;
		if (amount >= 4)
			amount = static_cast<int>(amount * generator.rand.nextDouble(0.75, 1.25));
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

bool ObjectManager::addGuard(rmg::Object & object, si32 str, bool zoneGuard)
{
	auto * guard = chooseGuard(str, zoneGuard);
	if(!guard)
		return false;
	
	rmg::Area visitablePos({object.getVisitablePosition()});
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
