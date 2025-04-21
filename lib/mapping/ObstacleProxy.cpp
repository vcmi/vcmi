/*
 * ObstacleProxy.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ObstacleProxy.h"
#include "../mapping/CMap.h"
#include "../mapObjectConstructors/AObjectTypeHandler.h"
#include "../mapObjectConstructors/CObjectClassesHandler.h"
#include "../mapObjects/CGObjectInstance.h"
#include "../mapObjects/ObjectTemplate.h"
#include "../mapObjects/ObstacleSetHandler.h"
#include "../GameLibrary.h"

#include <vstd/RNG.h>

VCMI_LIB_NAMESPACE_BEGIN

void ObstacleProxy::collectPossibleObstacles(TerrainId terrain)
{
	//get all possible obstacles for this terrain
	for(auto primaryID : LIBRARY->objtypeh->knownObjects())
	{
		for(auto secondaryID : LIBRARY->objtypeh->knownSubObjects(primaryID))
		{
			auto handler = LIBRARY->objtypeh->getHandlerFor(primaryID, secondaryID);
			if(handler->isStaticObject())
			{
				for(const auto & temp : handler->getTemplates())
				{
					if(temp->canBePlacedAt(terrain) && temp->getBlockMapOffset().isValid())
						obstaclesBySize[temp->getBlockedOffsets().size()].push_back(temp);
				}
			}
		}
	}
	sortObstacles();
}

void ObstacleProxy::sortObstacles()
{
	for(const auto & o : obstaclesBySize)
	{
		possibleObstacles.emplace_back(o);
	}
	boost::sort(possibleObstacles, [](const ObstaclePair &p1, const ObstaclePair &p2) -> bool
	{
		return p1.first > p2.first; //bigger obstacles first
	});
}

bool ObstacleProxy::prepareBiome(const ObstacleSetFilter & filter, vstd::RNG & rand)
{
	possibleObstacles.clear();

	std::vector<std::shared_ptr<ObstacleSet>> obstacleSets;

	size_t selectedSets = 0;
	const size_t MINIMUM_SETS = 3; // Original Lava has only 4 types of sets
	const size_t MAXIMUM_SETS = 9;
	const size_t MIN_SMALL_SETS = 3;
	const size_t MAX_SMALL_SETS = 5;

	auto terrain = filter.getTerrain();
	auto localFilter = filter;
	localFilter.setType(ObstacleSet::EObstacleType::MOUNTAINS);

	TObstacleTypes mountainSets = LIBRARY->biomeHandler->getObstacles(localFilter);

	if (!mountainSets.empty())
	{
		obstacleSets.push_back(*RandomGeneratorUtil::nextItem(mountainSets, rand));
		selectedSets++;
		logGlobal->info("Mountain set added");
	}
	else
	{
		logGlobal->warn("No mountain sets found for terrain %s", TerrainId::encode(terrain.getNum()));
		// FIXME: Do we ever want to generate obstacles without any mountains?
	}

	localFilter.setType(ObstacleSet::EObstacleType::TREES);
	TObstacleTypes treeSets = LIBRARY->biomeHandler->getObstacles(localFilter);

	// 1 or 2 tree sets
	size_t treeSetsCount = std::min<size_t>(treeSets.size(), rand.nextInt(1, 2));
	for (size_t i = 0; i < treeSetsCount; i++)
	{
		obstacleSets.push_back(*RandomGeneratorUtil::nextItem(treeSets, rand));
		selectedSets++;
	}
	logGlobal->info("Added %d tree sets", treeSetsCount);

	// Some obstacle types may be completely missing from water, but it's not a problem
	localFilter.setTypes({ObstacleSet::EObstacleType::LAKES, ObstacleSet::EObstacleType::CRATERS});
	TObstacleTypes largeSets = LIBRARY->biomeHandler->getObstacles(localFilter);

	// We probably don't want to have lakes and craters at the same time, choose one of them

	if (!largeSets.empty())
	{
		obstacleSets.push_back(*RandomGeneratorUtil::nextItem(largeSets, rand));
		selectedSets++;

		// TODO: Convert to string
		logGlobal->info("Added large set of type %s", obstacleSets.back()->getType());
	}

	localFilter.setType(ObstacleSet::EObstacleType::ROCKS);
	TObstacleTypes rockSets = LIBRARY->biomeHandler->getObstacles(localFilter);

	size_t rockSetsCount = std::min<size_t>(rockSets.size(), rand.nextInt(1, 2));
	for (size_t i = 0; i < rockSetsCount; i++)
	{
		obstacleSets.push_back(*RandomGeneratorUtil::nextItem(rockSets, rand));
		selectedSets++;
	}
	logGlobal->info("Added %d rock sets", rockSetsCount);

	localFilter.setType(ObstacleSet::EObstacleType::PLANTS);
	TObstacleTypes plantSets = LIBRARY->biomeHandler->getObstacles(localFilter);

	// 1 or 2 sets (3 - rock sets)
	size_t plantSetsCount = std::min<size_t>(plantSets.size(), rand.nextInt(1, std::max<size_t>(3 - rockSetsCount, 2)));
	for (size_t i = 0; i < plantSetsCount; i++)
	{
		{
			obstacleSets.push_back(*RandomGeneratorUtil::nextItem(plantSets, rand));
			selectedSets++;
		}
	}
	logGlobal->info("Added %d plant sets", plantSetsCount);

	//3 to 5 of total small sets (rocks, plants, structures, animals and others)
	//This gives total of 6 to 9 different sets

	size_t maxSmallSets = std::min<size_t>(MAX_SMALL_SETS, std::max(MIN_SMALL_SETS, MAXIMUM_SETS - selectedSets));

	size_t smallSets = rand.nextInt(MIN_SMALL_SETS, maxSmallSets);

	localFilter.setTypes({ObstacleSet::EObstacleType::STRUCTURES, ObstacleSet::EObstacleType::ANIMALS});
	TObstacleTypes smallObstacleSets = LIBRARY->biomeHandler->getObstacles(localFilter);
	RandomGeneratorUtil::randomShuffle(smallObstacleSets, rand);

	localFilter.setType(ObstacleSet::EObstacleType::OTHER);
	TObstacleTypes otherSets = LIBRARY->biomeHandler->getObstacles(localFilter);
	RandomGeneratorUtil::randomShuffle(otherSets, rand);

	while (smallSets > 0)
	{
		if (!smallObstacleSets.empty())
		{
			obstacleSets.push_back(smallObstacleSets.back());
			smallObstacleSets.pop_back();
			selectedSets++;
			smallSets--;
			logGlobal->info("Added small set of type %s", obstacleSets.back()->getType());
		}
		else if(otherSets.empty())
		{
			logGlobal->warn("No other sets found for terrain %s", terrain.encode(terrain.getNum()));
			break;
		}

		if (smallSets > 0)
		{
			// Fill with whatever's left
			if (!otherSets.empty())
			{
				obstacleSets.push_back(otherSets.back());
				otherSets.pop_back();
				selectedSets++;
				smallSets--;

				logGlobal->info("Added set of other obstacles");
			}
		}
	}

	// Copy this set to our possible obstacles

	if (selectedSets >= MINIMUM_SETS ||
		(terrain == TerrainId::WATER && selectedSets > 0))
	{
		obstaclesBySize.clear();
		for (const auto & os : obstacleSets)
		{
			for (const auto & temp : os->getObstacles())
			{
				if(temp->getBlockMapOffset().isValid())
				{
					obstaclesBySize[temp->getBlockedOffsets().size()].push_back(temp);
				}
			}
		}

		sortObstacles();

		return true;
	}
	else
	{
		return false; // Proceed with old method
	}
}

void ObstacleProxy::addBlockedTile(const int3& tile)
{
	blockedArea.add(tile);
}

void ObstacleProxy::setBlockedArea(const rmg::Area& area)
{
	blockedArea = area;
}

void ObstacleProxy::clearBlockedArea()
{
	blockedArea.clear();
}

bool ObstacleProxy::isProhibited(const rmg::Area& objArea) const
{
	return false;
};

int ObstacleProxy::getWeightedObjects(const int3 & tile, vstd::RNG & rand, IGameCallback * cb, std::list<rmg::Object> & allObjects, std::vector<std::pair<rmg::Object*, int3>> & weightedObjects)
{
	int maxWeight = std::numeric_limits<int>::min();
	for(auto & possibleObstacle : possibleObstacles)
	{
		if(!possibleObstacle.first)
			continue;

		auto shuffledObstacles = possibleObstacle.second;
		RandomGeneratorUtil::randomShuffle(shuffledObstacles, rand);

		for(const auto & temp : shuffledObstacles)
		{
			auto handler = LIBRARY->objtypeh->getHandlerFor(temp->id, temp->subid);
			auto obj = handler->create(cb, temp);
			allObjects.emplace_back(obj);
			rmg::Object * rmgObject = &allObjects.back();
			for(const auto & offset : obj->getBlockedOffsets())
			{
				auto newPos = tile - offset;

				if(!isInTheMap(newPos))
					continue;

				rmgObject->setPosition(newPos);

				bool isInTheMapEntirely = true;
				for (const auto & t : rmgObject->getArea().getTiles())
				{
					if (!isInTheMap(t))
					{
						isInTheMapEntirely = false;
						break;
					}

				}
				if (!isInTheMapEntirely)
				{
					continue;
				}

				if(isProhibited(rmgObject->getArea()))
					continue;

				int coverageBlocked = 0;
				int coveragePossible = 0;
				//do not use area intersection in optimization purposes
				for(const auto & t : rmgObject->getArea().getTilesVector())
				{
					auto coverage = verifyCoverage(t);
					if(coverage.first)
						++coverageBlocked;
					if(coverage.second)
						++coveragePossible;
				}

				int coverageOverlap = possibleObstacle.first - coverageBlocked - coveragePossible;
				int weight = possibleObstacle.first + coverageBlocked - coverageOverlap * possibleObstacle.first;
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

std::set<std::shared_ptr<CGObjectInstance>> ObstacleProxy::createObstacles(vstd::RNG & rand, IGameCallback * cb)
{
	//reverse order, since obstacles begin in bottom-right corner, while the map coordinates begin in top-left
	auto blockedTiles = blockedArea.getTilesVector();
	int tilePos = 0;
	std::set<std::shared_ptr<CGObjectInstance>> objs;

	while(!blockedArea.empty() && tilePos < blockedArea.getTilesVector().size())
	{
		auto tile = blockedArea.getTilesVector()[tilePos];

		std::list<rmg::Object> allObjects;
		std::vector<std::pair<rmg::Object*, int3>> weightedObjects;
		int maxWeight = getWeightedObjects(tile, rand, cb, allObjects, weightedObjects);

		if(weightedObjects.empty())
		{
			tilePos += 1;
			continue;
		}

		auto objIter = RandomGeneratorUtil::nextItem(weightedObjects, rand);
		objIter->first->setPosition(objIter->second);
		placeObject(*objIter->first, objs);

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

	return objs;
}

//FIXME: Only editor placer obstacles directly

void ObstacleProxy::finalInsertion(CMapEditManager * manager, const std::set<std::shared_ptr<CGObjectInstance>> & instances)
{
	manager->insertObjects(instances); //insert as one operation - for undo purposes
}

std::pair<bool, bool> ObstacleProxy::verifyCoverage(const int3 & t) const
{
	return {blockedArea.contains(t), false};
}

void ObstacleProxy::placeObject(rmg::Object & object, std::set<std::shared_ptr<CGObjectInstance>> & instances)
{
	for (const auto * instance : object.instances())
	{
		instances.insert(instance->pointer());
	}
}

EditorObstaclePlacer::EditorObstaclePlacer(CMap* map):
	map(map)
{
}

bool EditorObstaclePlacer::isInTheMap(const int3& tile)
{
	return map->isInTheMap(tile);
}

std::set<std::shared_ptr<CGObjectInstance>> EditorObstaclePlacer::placeObstacles(vstd::RNG & rand)
{
	auto obstacles = createObstacles(rand, map->cb);
	finalInsertion(map->getEditManager(), obstacles);
	return obstacles;
}

VCMI_LIB_NAMESPACE_END
