/*
 * ObstacleProxy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "StdInc.h"
#include "../rmg/RmgObject.h"
#include "CMapEditManager.h"

VCMI_LIB_NAMESPACE_BEGIN

class CMapEditManager;
class CGObjectInstance;
class ObjectTemplate;
class CRandomGenerator;

class DLL_LINKAGE ObstacleProxy
{
    //Base class generating random obstacles for RMG and map editor
public:
	ObstacleProxy() = default;
	virtual ~ObstacleProxy() = default;

	void collectPossibleObstacles(TerrainId terrain);

	void addBlockedTile(const int3 & tile);

	void setBlockedArea(const rmg::Area & area);

	void clearBlockedArea();

	virtual bool isProhibited(const rmg::Area& objArea) const;

	virtual std::pair<bool, bool> verifyCoverage(const int3 & t) const;

	virtual void placeObject(rmg::Object & object, std::set<CGObjectInstance*> & instances);

	virtual std::set<CGObjectInstance*> createObstacles(CRandomGenerator & rand);

	virtual bool isInTheMap(const int3& tile) = 0;
	
	void finalInsertion(CMapEditManager * manager, std::set<CGObjectInstance*> & instances);

	virtual void postProcess(const rmg::Object& object) {};

protected:
	int getWeightedObjects(const int3& tile, CRandomGenerator& rand, std::list<rmg::Object>& allObjects, std::vector<std::pair<rmg::Object*, int3>>& weightedObjects);

	rmg::Area blockedArea;

	using ObstacleVector = std::vector<std::shared_ptr<const ObjectTemplate>>;
	std::map<int, ObstacleVector> obstaclesBySize;
	using ObstaclePair = std::pair<int, ObstacleVector>;
	std::vector<ObstaclePair> possibleObstacles;
};

class DLL_LINKAGE EditorObstaclePlacer : public ObstacleProxy
{
public:
	EditorObstaclePlacer(CMap* map);

	bool isInTheMap(const int3& tile) override;

	void placeObstacles(CRandomGenerator& rand);

private:
	CMap* map;
};

VCMI_LIB_NAMESPACE_END