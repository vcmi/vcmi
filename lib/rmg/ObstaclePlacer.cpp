/*
 * ObstaclePlacer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "../mapObjects/CObjectClassesHandler.h"
#include "ObstaclePlacer.h"
#include "ObjectManager.h"
#include "TreasurePlacer.h"
#include "RockPlacer.h"
#include "WaterRoutes.h"
#include "WaterProxy.h"
#include "RoadPlacer.h"
#include "RiverPlacer.h"
#include "RmgMap.h"
#include "CMapGenerator.h"
#include "../CRandomGenerator.h"
#include "Functions.h"
#include "../mapping/CMapEditManager.h"

void ObstacleProxy::collectPossibleObstacles(const Terrain & terrain)
{
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
					if(temp.canBePlacedAt(terrain) && temp.getBlockMapOffset().valid())
						obstaclesBySize[temp.getBlockedOffsets().size()].push_back(temp);
				}
			}
		}
	}
	for(auto o : obstaclesBySize)
	{
		possibleObstacles.push_back(o);
	}
	boost::sort(possibleObstacles, [](const ObstaclePair &p1, const ObstaclePair &p2) -> bool
	{
		return p1.first > p2.first; //bigger obstacles first
	});
}

std::pair<bool, bool> ObstacleProxy::verifyCoverage(const int3 & t) const
{
	std::pair<bool, bool> result(false, false);
	if(blockedArea.contains(t))
		result.first = true;
	return result;
}

void ObstacleProxy::placeObject(CMapEditManager * manager, rmg::Object & object)
{
	for(auto * instance : object.instances())
	{
		manager->insertObject(&instance->object());
	}
	//manager->placeObject(*objIter->first, false, false);
}

int ObstacleProxy::getWeightedObjects(const int3 & tile, const CMap * map, CRandomGenerator & rand, std::list<rmg::Object> & allObjects, std::vector<std::pair<rmg::Object*, int3>> & weightedObjects)
{
	int maxWeight = std::numeric_limits<int>::min();
	for(int i = 0; i < possibleObstacles.size(); ++i)
	{
		if(!possibleObstacles[i].first)
			continue;

		auto shuffledObstacles = possibleObstacles[i].second;
		RandomGeneratorUtil::randomShuffle(shuffledObstacles, rand);

		for(auto & temp : shuffledObstacles)
		{
			auto handler = VLC->objtypeh->getHandlerFor(temp.id, temp.subid);
			auto obj = handler->create(temp);
			allObjects.emplace_back(*obj);
			rmg::Object * rmgObject = &allObjects.back();
			for(auto & offset : obj->getBlockedOffsets())
			{
				rmgObject->setPosition(tile - offset);
				if(!map->isInTheMap(rmgObject->getPosition()))
					continue;

				if(!rmgObject->getArea().getSubarea([map](const int3 & t)
				{
					return !map->isInTheMap(t);
				}).empty())
					continue;

				if(isProhibited(rmgObject->getArea()))
					continue;

				int coverageBlocked = 0;
				int coveragePossible = 0;
				//do not use area intersection in optimization purposes
				for(auto & t : rmgObject->getArea().getTilesVector())
				{
					auto coverage = verifyCoverage(t);
					if(coverage.first)
						++coverageBlocked;
					if(coverage.second)
						++coveragePossible;
				}

				int coverageOverlap = possibleObstacles[i].first - coverageBlocked - coveragePossible;
				int weight = possibleObstacles[i].first + coverageBlocked - coverageOverlap * possibleObstacles[i].first;
				assert(coverageOverlap >= 0);

				if(weight > maxWeight)
				{
					weightedObjects.clear();
					maxWeight = weight;
					weightedObjects.emplace_back(rmgObject, rmgObject->getPosition());
					if(weight > 0)
						break;
				}
				else if(weight == maxWeight)
					weightedObjects.emplace_back(rmgObject, rmgObject->getPosition());

			}
		}

		if(maxWeight > 0)
			break;
	}

	return maxWeight;
}

void ObstacleProxy::placeObstacles(CMap * map, CRandomGenerator & rand)
{
	//reverse order, since obstacles begin in bottom-right corner, while the map coordinates begin in top-left
	auto blockedTiles = blockedArea.getTilesVector();
	int tilePos = 0;
	while(!blockedArea.empty() && tilePos < blockedArea.getTilesVector().size())
	{
		auto tile = blockedArea.getTilesVector()[tilePos];

		std::list<rmg::Object> allObjects;
		std::vector<std::pair<rmg::Object*, int3>> weightedObjects;
		int maxWeight = getWeightedObjects(tile, map, rand, allObjects, weightedObjects);

		if(weightedObjects.empty())
		{
			tilePos += 1;
			continue;
		}

		auto objIter = RandomGeneratorUtil::nextItem(weightedObjects, rand);
		objIter->first->setPosition(objIter->second);
		placeObject(map->getEditManager(), *objIter->first);
		blockedArea.subtract(objIter->first->getArea());
		tilePos = 0;

		postProcess(*objIter->first);

		if(maxWeight < 0)
			logGlobal->warn("Placed obstacle with negative weight at %s", objIter->second.toString());

		for(auto & o : allObjects)
		{
			if(&o != objIter->first)
				o.clear();
		}
	}
}

void ObstacleProxy::postProcess(const rmg::Object & object)
{
	//river processing
	/*if(riverManager)
	{
		if(object.instances().front()->object().typeName == "mountain")
			riverManager->riverSource().unite(object.>getArea());
		if(object.instances().front()->object().typeName == "lake")
			riverManager->riverSink().unite(oobject.getArea());
	}*/
}

bool ObstacleProxy::isProhibited(const rmg::Area & objArea) const
{
	/*if(prohibitedArea.overlap(rmgObject->getArea()))
		continue;

	if(!totalArea.contains(rmgObject->getArea()))
		continue;*/
	return false;
}



void ObstaclePlacer::process()
{
	auto * manager = zone.getModificator<ObjectManager>();
	if(!manager)
		return;
	
	auto * riverManager = zone.getModificator<RiverPlacer>();
	
	typedef std::vector<ObjectTemplate> ObstacleVector;
	//obstacleVector possibleObstacles;
	
	std::map<int, ObstacleVector> obstaclesBySize;
	typedef std::pair<int, ObstacleVector> ObstaclePair;
	std::vector<ObstaclePair> possibleObstacles;
	
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
						obstaclesBySize[temp.getBlockedOffsets().size()].push_back(temp);
				}
			}
		}
	}
	for(auto o : obstaclesBySize)
	{
		possibleObstacles.push_back(o);
	}
	boost::sort(possibleObstacles, [](const ObstaclePair &p1, const ObstaclePair &p2) -> bool
	{
		return p1.first > p2.first; //bigger obstacles first
	});
	
	auto blockedArea = zone.area().getSubarea([this](const int3 & t)
	{
		return map.shouldBeBlocked(t);
	});
	blockedArea.subtract(zone.areaUsed());
	zone.areaPossible().subtract(blockedArea);
	
	
	auto prohibitedArea = zone.freePaths() + zone.areaUsed() + manager->getVisitableArea();
	
	//reverse order, since obstacles begin in bottom-right corner, while the map coordinates begin in top-left
	auto blockedTiles = blockedArea.getTilesVector();
	int tilePos = 0;
	while(!blockedArea.empty() && tilePos < blockedArea.getTilesVector().size())
	{
		auto tile = blockedArea.getTilesVector()[tilePos];
		
		std::list<rmg::Object> allObjects;
		std::vector<std::pair<rmg::Object*, int3>> weightedObjects; //obj + position
		int maxWeight = std::numeric_limits<int>::min();
		for(int i = 0; i < possibleObstacles.size(); ++i)
		{
			if(!possibleObstacles[i].first)
				continue;
			
			auto shuffledObstacles = possibleObstacles[i].second;
			RandomGeneratorUtil::randomShuffle(shuffledObstacles, generator.rand);
			
			for(auto & temp : shuffledObstacles)
			{
				auto handler = VLC->objtypeh->getHandlerFor(temp.id, temp.subid);
				auto obj = handler->create(temp);
				allObjects.emplace_back(*obj);
				rmg::Object * rmgObject = &allObjects.back();
				for(auto & offset : obj->getBlockedOffsets())
				{
					rmgObject->setPosition(tile - offset);
					if(!map.isOnMap(rmgObject->getPosition()))
						continue;
					
					if(!rmgObject->getArea().getSubarea([this](const int3 & t)
					{
						return !map.isOnMap(t);
					}).empty())
						continue;
					
					if(prohibitedArea.overlap(rmgObject->getArea()))
						continue;
					
					if(!zone.area().contains(rmgObject->getArea()))
						continue;
					
					int coverageBlocked = 0;
					int coveragePossible = 0;
					//do not use area intersection in optimization purposes
					for(auto & t : rmgObject->getArea().getTilesVector())
					{
						if(map.shouldBeBlocked(t))
							++coverageBlocked;
						if(zone.areaPossible().contains(t))
							++coveragePossible;
					}
					
					int coverageOverlap = possibleObstacles[i].first - coverageBlocked - coveragePossible;
					int weight = possibleObstacles[i].first + coverageBlocked - coverageOverlap * possibleObstacles[i].first;
					assert(coverageOverlap >= 0);
					
					if(weight > maxWeight)
					{
						weightedObjects.clear();
						maxWeight = weight;
						weightedObjects.emplace_back(rmgObject, rmgObject->getPosition());
						if(weight > 0)
							break;
					}
					else if(weight == maxWeight)
						weightedObjects.emplace_back(rmgObject, rmgObject->getPosition());
					
				}
			}
			
			if(maxWeight > 0)
				break;
		}
		
		if(weightedObjects.empty())
		{
			tilePos += 1;
			continue;
		}
		
		auto objIter = RandomGeneratorUtil::nextItem(weightedObjects, generator.rand);
		objIter->first->setPosition(objIter->second);
		manager->placeObject(*objIter->first, false, false);
		blockedArea.subtract(objIter->first->getArea());
		tilePos = 0;
		
		//river processing
		if(riverManager)
		{
			if(objIter->first->instances().front()->object().typeName == "mountain")
				riverManager->riverSource().unite(objIter->first->getArea());
			if(objIter->first->instances().front()->object().typeName == "lake")
				riverManager->riverSink().unite(objIter->first->getArea());
		}
		
		if(maxWeight < 0)
			logGlobal->warn("Placed obstacle with negative weight at %s", objIter->second.toString());
		
		for(auto & o : allObjects)
		{
			if(&o != objIter->first)
				o.clear();
		}
	}
}

void ObstaclePlacer::init()
{
	DEPENDENCY(ObjectManager);
	DEPENDENCY(TreasurePlacer);
	DEPENDENCY(WaterRoutes);
	DEPENDENCY(WaterProxy);
	DEPENDENCY(RoadPlacer);
	DEPENDENCY_ALL(RockPlacer);
}
