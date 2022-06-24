//
//  RmgMap.hpp
//  vcmi
//
//  Created by nordsoft on 23.06.2022.
//

#pragma once
#include "../int3.h"
#include "../GameConstants.h"
#include "../mapping/CMap.h"

class CMapEditManager;
class CTileInfo;
class CMapGenOptions;
class Zone;

class rmgException : public std::exception
{
	std::string msg;
public:
	explicit rmgException(const std::string& _Message) : msg(_Message)
	{
	}
	
	virtual ~rmgException() throw ()
	{
	};
	
	const char *what() const throw () override
	{
		return msg.c_str();
	}
};

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
	void setRoad(const int3 &tile, const std::string & roadType);
	
	CTileInfo getTile(const int3 & tile) const;
		
	float getNearestObjectDistance(const int3 &tile) const;
	void setNearestObjectDistance(int3 &tile, float value);
	
	TRmgTemplateZoneId getZoneID(const int3& tile) const;
	void setZoneID(const int3& tile, TRmgTemplateZoneId zid);
	
	using Zones = std::map<TRmgTemplateZoneId, std::shared_ptr<Zone>>;
	
	Zones & getZones();
	
	void registerZone(TFaction faction);
	ui32 getZoneCount(TFaction faction);
	ui32 getTotalZoneCount() const;
	void initTiles(CRandomGenerator & generator);
	
	bool isAllowedSpell(SpellID sid) const;
	
	void dump(bool zoneId) const;
	
private:
	void checkIsOnMap(const int3 &tile) const; //throws
	
private:
	Zones zones;
	std::map<TFaction, ui32> zonesPerFaction;
	ui32 zonesTotal; //zones that have their main town only
	const CMapGenOptions& mapGenOptions;
	CTileInfo*** tiles;
	boost::multi_array<TRmgTemplateZoneId, 3> zoneColouring; //[z][x][y]
};
