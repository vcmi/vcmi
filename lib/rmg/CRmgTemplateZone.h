
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
#include "../ResourceSet.h" //for TResource (?)
#include "../mapObjects/ObjectTemplate.h"

class CMapGenerator;
class CTileInfo;
class int3;
class CGObjectInstance;
class ObjectTemplate;

namespace ETemplateZoneType
{
	enum ETemplateZoneType
	{
		PLAYER_START,
		CPU_START,
		TREASURE,
		JUNCTION
	};
}
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

class DLL_LINKAGE CTreasureInfo
{
public:
	ui32 min;
	ui32 max;
	ui16 density;
};

struct DLL_LINKAGE ObjectInfo
{
	ObjectTemplate templ;
	ui32 value;
	ui16 probability;
	ui32 maxPerZone;
	ui32 maxPerMap;
	std::function<CGObjectInstance *()> generateObject;

	void setTemplate (si32 type, si32 subtype, ETerrainType terrain);

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
class DLL_LINKAGE CRmgTemplateZone
{
public:
	class DLL_LINKAGE CTownInfo
	{
	public:
		CTownInfo();

		int getTownCount() const; /// Default: 0
		void setTownCount(int value);
		int getCastleCount() const; /// Default: 0
		void setCastleCount(int value);
		int getTownDensity() const; /// Default: 0
		void setTownDensity(int value);
		int getCastleDensity() const; /// Default: 0
		void setCastleDensity(int value);

	private:
		int townCount, castleCount, townDensity, castleDensity;
	};

	CRmgTemplateZone();

	TRmgTemplateZoneId getId() const; /// Default: 0
	void setId(TRmgTemplateZoneId value);
	ETemplateZoneType::ETemplateZoneType getType() const; /// Default: ETemplateZoneType::PLAYER_START
	void setType(ETemplateZoneType::ETemplateZoneType value);
	int getSize() const; /// Default: 1
	void setSize(int value);
	boost::optional<int> getOwner() const;
	void setOwner(boost::optional<int> value);
	const CTownInfo & getPlayerTowns() const;
	void setPlayerTowns(const CTownInfo & value);
	const CTownInfo & getNeutralTowns() const;
	void setNeutralTowns(const CTownInfo & value);
	bool getTownsAreSameType() const; /// Default: false
	void setTownsAreSameType(bool value);
	const std::set<TFaction> & getTownTypes() const; /// Default: all
	void setTownTypes(const std::set<TFaction> & value);
	void setMonsterTypes(const std::set<TFaction> & value);
	std::set<TFaction> getDefaultTownTypes() const;
	bool getMatchTerrainToTown() const; /// Default: true
	void setMatchTerrainToTown(bool value);
	const std::set<ETerrainType> & getTerrainTypes() const; /// Default: all
	void setTerrainTypes(const std::set<ETerrainType> & value);
	std::set<ETerrainType> getDefaultTerrainTypes() const;
	void setMinesAmount (TResource res, ui16 amount);
	std::map<TResource, ui16> getMinesInfo() const;
	void setMonsterStrength (EMonsterStrength::EMonsterStrength val);

	float3 getCenter() const;
	void setCenter(const float3 &f);
	int3 getPos() const;
	void setPos(const int3 &pos);
	bool isAccessibleFromAnywhere(CMapGenerator* gen, ObjectTemplate &appearance, int3 &tile) const;
	int3 getAccessibleOffset(CMapGenerator* gen, ObjectTemplate &appearance, int3 &tile) const;

	void addTile (const int3 &pos);
	void initFreeTiles (CMapGenerator* gen);
	std::set<int3> getTileInfo () const;
	void discardDistantTiles (CMapGenerator* gen, float distance);
	void clearTiles();

	void addRequiredObject(CGObjectInstance * obj, si32 guardStrength=0);
	void addCloseObject(CGObjectInstance * obj, si32 guardStrength = 0);
	void addToConnectLater(const int3& src);
	bool addMonster(CMapGenerator* gen, int3 &pos, si32 strength, bool clearSurroundingTiles = true, bool zoneGuard = false);
	bool createTreasurePile(CMapGenerator* gen, int3 &pos, float minDistance, const CTreasureInfo& treasureInfo);
	bool fill (CMapGenerator* gen);
	bool placeMines (CMapGenerator* gen);
	void initTownType (CMapGenerator* gen);
	void paintZoneTerrain (CMapGenerator* gen, ETerrainType terrainType);
	void randomizeTownType(CMapGenerator* gen); //helper function
	void initTerrainType (CMapGenerator* gen);
	void createBorder(CMapGenerator* gen);
	void fractalize(CMapGenerator* gen);
	void connectLater(CMapGenerator* gen);
	EObjectPlacingResult::EObjectPlacingResult tryToPlaceObjectAndConnectToPath(CMapGenerator* gen, CGObjectInstance *obj, int3 &pos); //return true if the position cna be connected
	bool createRequiredObjects(CMapGenerator* gen);
	void createTreasures(CMapGenerator* gen);
	void createObstacles1(CMapGenerator* gen);
	void createObstacles2(CMapGenerator* gen);
	bool crunchPath(CMapGenerator* gen, const int3 &src, const int3 &dst, bool onlyStraight, std::set<int3>* clearedTiles = nullptr);
	bool connectPath(CMapGenerator* gen, const int3& src, bool onlyStraight);
	bool connectWithCenter(CMapGenerator* gen, const int3& src, bool onlyStraight);

	std::vector<int3> getAccessibleOffsets (CMapGenerator* gen, CGObjectInstance* object);

	void addConnection(TRmgTemplateZoneId otherZone);
	void setQuestArtZone(CRmgTemplateZone * otherZone);
	std::vector<TRmgTemplateZoneId> getConnections() const;
	void addTreasureInfo(CTreasureInfo & info);
	std::vector<CTreasureInfo> getTreasureInfo();
	std::set<int3>* getFreePaths();

	ObjectInfo getRandomObject (CMapGenerator* gen, CTreasurePileInfo &info, ui32 desiredValue, ui32 maxValue, ui32 currentValue);

	void placeSubterraneanGate(CMapGenerator* gen, int3 pos, si32 guardStrength);
	void placeObject(CMapGenerator* gen, CGObjectInstance* object, const int3 &pos, bool updateDistance = true);
	bool guardObject(CMapGenerator* gen, CGObjectInstance* object, si32 str, bool zoneGuard = false, bool addToFreePaths = false);
	void placeAndGuardObject(CMapGenerator* gen, CGObjectInstance* object, const int3 &pos, si32 str, bool zoneGuard = false);
	void addRoadNode(const int3 & node);
	void connectRoads(CMapGenerator * gen); //fills "roads" according to "roadNodes"

private:
	//template info
	TRmgTemplateZoneId id;
	ETemplateZoneType::ETemplateZoneType type;
	int size;
	boost::optional<int> owner;
	CTownInfo playerTowns, neutralTowns;
	bool townsAreSameType;
	std::set<TFaction> townTypes;
	std::set<TFaction> monsterTypes;
	bool matchTerrainToTown;
	std::set<ETerrainType> terrainTypes;
	std::map<TResource, ui16> mines; //obligatory mines to spawn in this zone

	si32 townType;
	ETerrainType terrainType;
	CRmgTemplateZone * questArtZone; //artifacts required for Seer Huts will be placed here - or not if null

	EMonsterStrength::EMonsterStrength zoneMonsterStrength;
	std::vector<CTreasureInfo> treasureInfo;
	std::vector<ObjectInfo> possibleObjects;
	int minGuardedValue;

	//content info
	std::vector<std::pair<CGObjectInstance*, ui32>> requiredObjects;
	std::vector<std::pair<CGObjectInstance*, ui32>> closeObjects;
	std::vector<CGObjectInstance*> objects;

	//placement info
	int3 pos;
	float3 center;
	std::set<int3> tileinfo; //irregular area assined to zone
	std::set<int3> possibleTiles; //optimization purposes for treasure generation
	std::vector<TRmgTemplateZoneId> connections; //list of adjacent zones
	std::set<int3> freePaths; //core paths of free tiles that all other objects will be linked to

	std::set<int3> roadNodes; //tiles to be connected with roads
	std::set<int3> roads; //all tiles with roads
	std::set<int3> tilesToConnectLater; //will be connected after paths are fractalized

	bool createRoad(CMapGenerator* gen, const int3 &src, const int3 &dst);
	void drawRoads(CMapGenerator * gen); //actually updates tiles

	bool pointIsIn(int x, int y);
	void addAllPossibleObjects (CMapGenerator* gen); //add objects, including zone-specific, to possibleObjects
	bool findPlaceForObject(CMapGenerator* gen, CGObjectInstance* obj, si32 min_dist, int3 &pos);
	bool findPlaceForTreasurePile(CMapGenerator* gen, float min_dist, int3 &pos, int value);
	bool canObstacleBePlacedHere(CMapGenerator* gen, ObjectTemplate &temp, int3 &pos);
	void setTemplateForObject(CMapGenerator* gen, CGObjectInstance* obj);
	bool areAllTilesAvailable(CMapGenerator* gen, CGObjectInstance* obj, int3& tile, std::set<int3>& tilesBlockedByObject) const;
	void checkAndPlaceObject(CMapGenerator* gen, CGObjectInstance* object, const int3 &pos);
};
