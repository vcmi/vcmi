/*
 * ObstaclePlacer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "Zone.h"

class CMap;
class CMapEditManager;
class DLL_LINKAGE ObstacleProxy
{
public:
	ObstacleProxy() = default;
	virtual ~ObstacleProxy() = default;

	rmg::Area blockedArea;

	void collectPossibleObstacles(const Terrain & terrain);

	void placeObstacles(CMap * map, CRandomGenerator & rand);

	virtual std::pair<bool, bool> verifyCoverage(const int3 & t) const;

	virtual void placeObject(CMapEditManager * manager, rmg::Object & object);

	virtual void postProcess(const rmg::Object & object);

	virtual bool isProhibited(const rmg::Area & objArea) const;

protected:
	int getWeightedObjects(const int3 & tile, const CMap * map, CRandomGenerator & rand, std::list<rmg::Object> & allObjects, std::vector<std::pair<rmg::Object*, int3>> & weightedObjects);

	typedef std::vector<std::shared_ptr<const ObjectTemplate>> ObstacleVector;
	std::map<int, ObstacleVector> obstaclesBySize;
	typedef std::pair<int, ObstacleVector> ObstaclePair;
	std::vector<ObstaclePair> possibleObstacles;
};

class ObstaclePlacer: public Modificator
{
public:
	MODIFICATOR(ObstaclePlacer);
	
	void process() override;
	void init() override;
};
