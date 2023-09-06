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
#include "MapProxy.h"

VCMI_LIB_NAMESPACE_BEGIN

class CMap;
class CMapEditManager;
class TileInfo;
class CMapGenOptions;
class Zone;
class CMapGenerator;
class MapProxy;
class playerInfo;

class RmgMap
{
public:
	mutable std::unique_ptr<CMap> mapInstance;
	std::shared_ptr<MapProxy> getMapProxy() const;
	CMap & getMap(const CMapGenerator *) const; //limited access
	
	RmgMap(const CMapGenOptions& mapGenOptions);
	~RmgMap() = default;

	CMapEditManager* getEditManager() const;
	const CMapGenOptions& getMapGenOptions() const;

	void foreach_neighbour(const int3 & pos, const std::function<void(int3 & pos)> & foo) const;
	void foreachDirectNeighbour(const int3 & pos, const std::function<void(int3 & pos)> & foo) const;
	void foreachDiagonalNeighbour(const int3 & pos, const std::function<void(int3 & pos)> & foo) const;

	bool isBlocked(const int3 &tile) const;
	bool shouldBeBlocked(const int3 &tile) const;
	bool isPossible(const int3 &tile) const;
	bool isFree(const int3 &tile) const;
	bool isUsed(const int3 &tile) const;
	bool isRoad(const int3 &tile) const;
	bool isOnMap(const int3 & tile) const;

	int levels() const;
	int width() const;
	int height() const;
	PlayerInfo & getPlayer(int playerId);
	
	void setOccupied(const int3 &tile, ETileType state);
	void setRoad(const int3 &tile, RoadId roadType);
	
	TileInfo getTileInfo(const int3 & tile) const;
	TerrainTile & getTile(const int3 & tile) const;
		
	float getNearestObjectDistance(const int3 &tile) const;
	void setNearestObjectDistance(int3 &tile, float value);
	
	TRmgTemplateZoneId getZoneID(const int3& tile) const;
	void setZoneID(const int3& tile, TRmgTemplateZoneId zid);
	
	using Zones = std::map<TRmgTemplateZoneId, std::shared_ptr<Zone>>;
	using ZonePair = std::pair<TRmgTemplateZoneId, std::shared_ptr<Zone>>;
	using ZoneVector = std::vector<ZonePair>;
	
	Zones & getZones();
	
	void registerZone(FactionID faction);
	ui32 getZoneCount(FactionID faction);
	ui32 getTotalZoneCount() const;
	void initTiles(CMapGenerator & generator, CRandomGenerator & rand);
	void addModificators();

	bool isAllowedSpell(const SpellID & sid) const;

	void dump(bool zoneId) const;
	
private:
	void assertOnMap(const int3 &tile) const; //throws

private:

	std::shared_ptr<MapProxy> mapProxy;

	Zones zones;
	std::map<FactionID, ui32> zonesPerFaction;
	ui32 zonesTotal; //zones that have their main town only
	const CMapGenOptions& mapGenOptions;
	boost::multi_array<TileInfo, 3> tiles; //[x][y][z]
	boost::multi_array<TRmgTemplateZoneId, 3> zoneColouring; //[x][y][z]
};

VCMI_LIB_NAMESPACE_END
