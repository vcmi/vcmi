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
class RiverPlacer;
class ObjectManager;
class DLL_LINKAGE ObstacleProxy
{
public:
	ObstacleProxy() = default;
	virtual ~ObstacleProxy() = default;

	rmg::Area blockedArea;

	void collectPossibleObstacles(TTerrainId terrain);

	void placeObstacles(CMap * map, CRandomGenerator & rand);

	virtual std::pair<bool, bool> verifyCoverage(const int3 & t) const;

	virtual void placeObject(rmg::Object & object, std::set<CGObjectInstance*> & instances);

	virtual void postProcess(const rmg::Object & object);

	virtual bool isProhibited(const rmg::Area & objArea) const;
	
	virtual void finalInsertion(CMapEditManager * manager, std::set<CGObjectInstance*> & instances);

protected:
	int getWeightedObjects(const int3 & tile, const CMap * map, CRandomGenerator & rand, std::list<rmg::Object> & allObjects, std::vector<std::pair<rmg::Object*, int3>> & weightedObjects);

	typedef std::vector<std::shared_ptr<const ObjectTemplate>> ObstacleVector;
	std::map<int, ObstacleVector> obstaclesBySize;
	typedef std::pair<int, ObstacleVector> ObstaclePair;
	std::vector<ObstaclePair> possibleObstacles;
};

class ObstaclePlacer: public Modificator, public ObstacleProxy
{
public:
	MODIFICATOR(ObstaclePlacer);
	
	void process() override;
	void init() override;
	
	std::pair<bool, bool> verifyCoverage(const int3 & t) const override;
	
	void placeObject(rmg::Object & object, std::set<CGObjectInstance*> & instances) override;
	
	void postProcess(const rmg::Object & object) override;
	
	bool isProhibited(const rmg::Area & objArea) const override;
	
	void finalInsertion(CMapEditManager * manager, std::set<CGObjectInstance*> & instances) override;
	
private:
	rmg::Area prohibitedArea;
	RiverPlacer * riverManager;
	ObjectManager * manager;
};
