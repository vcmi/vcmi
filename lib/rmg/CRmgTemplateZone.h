/*
 * CRmgTemplateZone.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../GameConstants.h"
#include "CMapGenerator.h"
#include "float3.h"
#include "../int3.h"
#include "CRmgTemplate.h"
#include "../mapObjects/ObjectTemplate.h"
#include <boost/heap/priority_queue.hpp> //A*

class CMapGenerator;
class CTileInfo;
class int3;
class CGObjectInstance;
class ObjectTemplate;

namespace EObjectPlacingResult
{
	enum EObjectPlacingResult
	{
		SUCCESS,
		CANNOT_FIT,
		SEALED_OFF
	};
}
class DLL_LINKAGE CTileInfo
{
public:

	CTileInfo();

	float getNearestObjectDistance() const;
	void setNearestObjectDistance(float value);
	bool isBlocked() const;
	bool shouldBeBlocked() const;
	bool isPossible() const;
	bool isFree() const;
	bool isUsed() const;
	bool isRoad() const;
	void setOccupied(ETileType::ETileType value);
	ETerrainType getTerrainType() const;
	ETileType::ETileType getTileType() const;
	void setTerrainType(ETerrainType value);

	void setRoadType(ERoadType::ERoadType value);
private:
	float nearestObjectDistance;
	ETileType::ETileType occupied;
	ETerrainType terrain;
	ERoadType::ERoadType roadType;
};

struct DLL_LINKAGE ObjectInfo
{
	ObjectTemplate templ;
	ui32 value;
	ui16 probability;
	ui32 maxPerZone;
	//ui32 maxPerMap; //unused
	std::function<CGObjectInstance *()> generateObject;

	void setTemplate (si32 type, si32 subtype, ETerrainType terrain);

	ObjectInfo();

	bool operator==(const ObjectInfo& oi) const { return (templ == oi.templ); }
};

struct DLL_LINKAGE CTreasurePileInfo
{
	std::set<int3> visitableFromBottomPositions; //can be visited only from bottom or side
	std::set<int3> visitableFromTopPositions; //they can be visited from any direction
	std::set<int3> blockedPositions;
	std::set<int3> occupiedPositions; //blocked + visitable
	int3 nextTreasurePos;
};

/// The CRmgTemplateZone describes a zone in a template.
class DLL_LINKAGE CRmgTemplateZone : public rmg::ZoneOptions
{
public:
	CRmgTemplateZone(CMapGenerator * Gen);

	void setOptions(const rmg::ZoneOptions & options);
	bool isUnderground() const;

	float3 getCenter() const;
	void setCenter(const float3 &f);
	int3 getPos() const;
	void setPos(const int3 &pos);
	bool isAccessibleFromSomewhere(ObjectTemplate & appearance, const int3 & tile) const;
	int3 getAccessibleOffset(ObjectTemplate & appearance, const int3 & tile) const;

	void addTile (const int3 & pos);
	void removeTile(const int3 & pos);
	void initFreeTiles ();
	std::set<int3> getTileInfo() const;
	std::set<int3> getPossibleTiles() const;
	std::set<int3> collectDistantTiles (float distance) const;
	void clearTiles();

	void addRequiredObject(CGObjectInstance * obj, si32 guardStrength=0);
	void addCloseObject(CGObjectInstance * obj, si32 guardStrength = 0);
	void addNearbyObject(CGObjectInstance * obj, CGObjectInstance * nearbyTarget);
	void addObjectAtPosition(CGObjectInstance * obj, const int3 & position, si32 guardStrength=0);

	void addToConnectLater(const int3& src);
	bool addMonster(int3 &pos, si32 strength, bool clearSurroundingTiles = true, bool zoneGuard = false);
	bool createTreasurePile(int3 &pos, float minDistance, const CTreasureInfo& treasureInfo);
	bool fill ();
	bool placeMines ();
	void initTownType ();
	void paintZoneTerrain (ETerrainType terrainType);
	void randomizeTownType(bool matchUndergroundType = false); //helper function
	void initTerrainType ();
	void createBorder();
	void fractalize();
	void connectLater();
	EObjectPlacingResult::EObjectPlacingResult tryToPlaceObjectAndConnectToPath(CGObjectInstance * obj, const int3 & pos); //return true if the position can be connected
	bool createRequiredObjects();
	bool createShipyard(const int3 & pos, si32 guardStrength=0);
	int3 createShipyard(const std::set<int3> & lake, si32 guardStrength=0);
	bool makeBoat(TRmgTemplateZoneId land, const int3 & coast);
	int3 makeBoat(TRmgTemplateZoneId land, const std::set<int3> & lake);
	void createTreasures();
	
	void createWater(EWaterContent::EWaterContent waterContent, bool debug=false);
	void waterInitFreeTiles();
	void waterConnection(CRmgTemplateZone& dst);
	bool waterKeepConnection(TRmgTemplateZoneId zoneA, TRmgTemplateZoneId zoneB);
	const std::set<int3>& getCoastTiles() const;
	bool isWaterConnected(TRmgTemplateZoneId zone, const int3 & tile) const;
	//void computeCoastTiles();
	
	void createObstacles1();
	void createObstacles2();
	bool crunchPath(const int3 &src, const int3 &dst, bool onlyStraight, std::set<int3>* clearedTiles = nullptr);
	bool connectPath(const int3& src, bool onlyStraight);
	bool connectWithCenter(const int3& src, bool onlyStraight, bool passTroughBlocked = false);
	void updateDistances(const int3 & pos);

	std::vector<int3> getAccessibleOffsets (const CGObjectInstance* object);
	bool areAllTilesAvailable(CGObjectInstance* obj, int3& tile, const std::set<int3>& tilesBlockedByObject) const;

	void setQuestArtZone(std::shared_ptr<CRmgTemplateZone> otherZone);
	std::set<int3>* getFreePaths();
	void addFreePath(const int3 &);

	ObjectInfo getRandomObject (CTreasurePileInfo &info, ui32 desiredValue, ui32 maxValue, ui32 currentValue);

	void placeSubterraneanGate(int3 pos, si32 guardStrength);
	void placeObject(CGObjectInstance* object, const int3 &pos, bool updateDistance = true);
	bool guardObject(CGObjectInstance* object, si32 str, bool zoneGuard = false, bool addToFreePaths = false);
	void placeAndGuardObject(CGObjectInstance* object, const int3 &pos, si32 str, bool zoneGuard = false);
	void addRoadNode(const int3 & node);
	void connectRoads(); //fills "roads" according to "roadNodes"

	//A* priority queue
	typedef std::pair<int3, float> TDistance;
	struct NodeComparer
	{
		bool operator()(const TDistance & lhs, const TDistance & rhs) const
		{
			return (rhs.second < lhs.second);
		}
	};
	boost::heap::priority_queue<TDistance, boost::heap::compare<NodeComparer>> createPriorityQueue();

private:
	
	//subclass to store disconnected parts of water zone
	struct Lake
	{
		std::set<int3> tiles;
		std::set<int3> coast;
		std::map<int3, int> distance;
		std::set<TRmgTemplateZoneId> connectedZones;
		std::set<TRmgTemplateZoneId> keepConnections;
	};
	
	CMapGenerator * gen;
	
	//template info
	si32 townType;
	ETerrainType terrainType;
	std::weak_ptr<CRmgTemplateZone> questArtZone; //artifacts required for Seer Huts will be placed here - or not if null

	std::vector<ObjectInfo> possibleObjects;
	int minGuardedValue;
	
	//content info
	std::vector<std::pair<CGObjectInstance*, ui32>> requiredObjects;
	std::vector<std::pair<CGObjectInstance*, ui32>> closeObjects;
	std::vector<std::pair<CGObjectInstance*, int3>> instantObjects;
	std::vector<std::pair<CGObjectInstance*, CGObjectInstance*>> nearbyObjects;
	std::vector<CGObjectInstance*> objects;
	std::map<CGObjectInstance*, int3> requestedPositions;

	//placement info
	int3 pos;
	float3 center;
	std::set<int3> tileinfo; //irregular area assined to zone
	std::set<int3> possibleTiles; //optimization purposes for treasure generation
	std::set<int3> freePaths; //core paths of free tiles that all other objects will be linked to
	std::set<int3> coastTiles; //tiles bordered to water

	std::set<int3> roadNodes; //tiles to be connected with roads
	std::set<int3> roads; //all tiles with roads
	std::set<int3> tilesToConnectLater; //will be connected after paths are fractalized
	std::vector<Lake> lakes; //disconnected parts of zone. Used to work with water zones
	std::map<int3, int> lakeMap; //map tile on lakeId which is position of lake in lakes array +1
	
	bool createRoad(const int3 &src, const int3 &dst);
	void drawRoads(); //actually updates tiles

	bool pointIsIn(int x, int y);
	void addAllPossibleObjects (); //add objects, including zone-specific, to possibleObjects
	bool findPlaceForObject(CGObjectInstance* obj, si32 min_dist, int3 &pos);
	bool findPlaceForTreasurePile(float min_dist, int3 &pos, int value);
	bool canObstacleBePlacedHere(ObjectTemplate &temp, int3 &pos);
	void setTemplateForObject(CGObjectInstance* obj);
	void checkAndPlaceObject(CGObjectInstance* object, const int3 &pos);
	
	bool isGuardNeededForTreasure(int value);
};
