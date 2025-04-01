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

#include "../rmg/RmgObject.h"
#include "CMapEditManager.h"

VCMI_LIB_NAMESPACE_BEGIN

class CMapEditManager;
class CGObjectInstance;
class ObjectTemplate;
class IGameCallback;
class ObstacleSetFilter;

class DLL_LINKAGE ObstacleProxy
{
    //Base class generating random obstacles for RMG and map editor
public:
	ObstacleProxy() = default;
	virtual ~ObstacleProxy() = default;

	void collectPossibleObstacles(TerrainId terrain);
	bool prepareBiome(const ObstacleSetFilter & filter, vstd::RNG & rand);

	void addBlockedTile(const int3 & tile);

	void setBlockedArea(const rmg::Area & area);

	void clearBlockedArea();

	virtual bool isProhibited(const rmg::Area& objArea) const;

	virtual std::pair<bool, bool> verifyCoverage(const int3 & t) const;

	virtual void placeObject(rmg::Object & object, std::set<std::shared_ptr<CGObjectInstance>> & instances);

	virtual std::set<std::shared_ptr<CGObjectInstance>> createObstacles(vstd::RNG & rand, IGameCallback * cb);

	virtual bool isInTheMap(const int3& tile) = 0;
	
	void finalInsertion(CMapEditManager * manager, const std::set<std::shared_ptr<CGObjectInstance>> & instances);

	virtual void postProcess(const rmg::Object& object) {};

protected:
	int getWeightedObjects(const int3& tile, vstd::RNG& rand, IGameCallback * cb, std::list<rmg::Object>& allObjects, std::vector<std::pair<rmg::Object*, int3>>& weightedObjects);
	void sortObstacles();

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

	std::set<std::shared_ptr<CGObjectInstance>> placeObstacles(vstd::RNG& rand);

private:
	CMap* map;
};

VCMI_LIB_NAMESPACE_END
