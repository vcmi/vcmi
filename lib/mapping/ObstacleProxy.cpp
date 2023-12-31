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
#include "../mapObjects/ObjectTemplate.h"

VCMI_LIB_NAMESPACE_BEGIN

void ObstacleProxy::collectPossibleObstacles(TerrainId terrain)
{
	//get all possible obstacles for this terrain
	for(auto primaryID : VLC->objtypeh->knownObjects())
	{
		for(auto secondaryID : VLC->objtypeh->knownSubObjects(primaryID))
		{
			auto handler = VLC->objtypeh->getHandlerFor(primaryID, secondaryID);
			if(handler->isStaticObject())
			{
				for(const auto & temp : handler->getTemplates())
				{
					if(temp->canBePlacedAt(terrain) && temp->getBlockMapOffset().valid())
						obstaclesBySize[temp->getBlockedOffsets().size()].push_back(temp);
				}
			}
		}
	}
	for(const auto & o : obstaclesBySize)
	{
		possibleObstacles.emplace_back(o);
	}
	boost::sort(possibleObstacles, [](const ObstaclePair &p1, const ObstaclePair &p2) -> bool
	{
		return p1.first > p2.first; //bigger obstacles first
	});
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

int ObstacleProxy::getWeightedObjects(const int3 & tile, CRandomGenerator & rand, std::list<rmg::Object> & allObjects, std::vector<std::pair<rmg::Object*, int3>> & weightedObjects)
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
			auto handler = VLC->objtypeh->getHandlerFor(temp->id, temp->subid);
			auto * obj = handler->create(temp);
			allObjects.emplace_back(*obj);
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

std::set<CGObjectInstance*> ObstacleProxy::createObstacles(CRandomGenerator & rand)
{
	//reverse order, since obstacles begin in bottom-right corner, while the map coordinates begin in top-left
	auto blockedTiles = blockedArea.getTilesVector();
	int tilePos = 0;
	std::set<CGObjectInstance*> objs;

	while(!blockedArea.empty() && tilePos < blockedArea.getTilesVector().size())
	{
		auto tile = blockedArea.getTilesVector()[tilePos];

		std::list<rmg::Object> allObjects;
		std::vector<std::pair<rmg::Object*, int3>> weightedObjects;
		int maxWeight = getWeightedObjects(tile, rand, allObjects, weightedObjects);

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

void ObstacleProxy::finalInsertion(CMapEditManager * manager, std::set<CGObjectInstance*> & instances)
{
	manager->insertObjects(instances); //insert as one operation - for undo purposes
}

std::pair<bool, bool> ObstacleProxy::verifyCoverage(const int3 & t) const
{
	return {blockedArea.contains(t), false};
}

void ObstacleProxy::placeObject(rmg::Object & object, std::set<CGObjectInstance*> & instances)
{
	for (auto * instance : object.instances())
	{
		instances.insert(&instance->object());
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

std::set<CGObjectInstance*> EditorObstaclePlacer::placeObstacles(CRandomGenerator & rand)
{
	auto obstacles = createObstacles(rand);
	finalInsertion(map->getEditManager(), obstacles);
	return obstacles;
}

VCMI_LIB_NAMESPACE_END
