/*
 * RmgMap.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "../int3.h"
#include "../GameConstants.h"
#include "../mapping/CMap.h"

VCMI_LIB_NAMESPACE_BEGIN

class CMapEditManager;
class TileInfo;
class CMapGenOptions;
class Zone;
class CMapGenerator;

class RmgMap
{
public:
	mutable std::unique_ptr<CMap> mapInstance;
	CMap & map() const;
	
	RmgMap(const CMapGenOptions& mapGenOptions);
	~RmgMap();
	
	CMapEditManager* getEditManager() const;
	const CMapGenOptions& getMapGenOptions() const;
	
	void foreach_neighbour(const int3 &pos, std::function<void(int3& pos)> foo);
	void foreachDirectNeighbour(const int3 &pos, std::function<void(int3& pos)> foo);
	void foreachDiagonalNeighbour(const int3& pos, std::function<void(int3& pos)> foo);
	
	bool isBlocked(const int3 &tile) const;
	bool shouldBeBlocked(const int3 &tile) const;
	bool isPossible(const int3 &tile) const;
	bool isFree(const int3 &tile) const;
	bool isUsed(const int3 &tile) const;
	bool isRoad(const int3 &tile) const;
	bool isOnMap(const int3 & tile) const;
	
	void setOccupied(const int3 &tile, ETileType::ETileType state);
	void setRoad(const int3 &tile, RoadId roadType);
	
	TileInfo getTile(const int3 & tile) const;
		
	float getNearestObjectDistance(const int3 &tile) const;
	void setNearestObjectDistance(int3 &tile, float value);
	
	TRmgTemplateZoneId getZoneID(const int3& tile) const;
	void setZoneID(const int3& tile, TRmgTemplateZoneId zid);
	
	using Zones = std::map<TRmgTemplateZoneId, std::shared_ptr<Zone>>;
	
	Zones & getZones();
	
	void registerZone(TFaction faction);
	ui32 getZoneCount(TFaction faction);
	ui32 getTotalZoneCount() const;
	void initTiles(CMapGenerator & generator);
	void addModificators();
	
	bool isAllowedSpell(SpellID sid) const;
	
	void dump(bool zoneId) const;
	
private:
	void assertOnMap(const int3 &tile) const; //throws
	
private:
	Zones zones;
	std::map<TFaction, ui32> zonesPerFaction;
	ui32 zonesTotal; //zones that have their main town only
	const CMapGenOptions& mapGenOptions;
	boost::multi_array<TileInfo, 3> tiles; //[x][y][z]
	boost::multi_array<TRmgTemplateZoneId, 3> zoneColouring; //[x][y][z]
};

VCMI_LIB_NAMESPACE_END
