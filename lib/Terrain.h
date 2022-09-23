/*
 * Terrain.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ConstTransitivePtr.h"
#include "GameConstants.h"
#include "JsonNode.h"


class DLL_LINKAGE TerrainType
{
public:
	
	enum PassabilityType : ui8
	{
		LAND = 1,
		WATER = 2,
		SURFACE = 4,
		SUBTERRANEAN = 8,
		ROCK = 16
	};
	
	std::vector<std::string> battleFields;
	std::vector<TTerrain> prohibitTransitions;
	std::array<int, 3> minimapBlocked;
	std::array<int, 3> minimapUnblocked;
	std::string name;
	std::string musicFilename;
	std::string tilesFilename;
	std::string terrainText;
	std::string typeCode;
	std::string terrainViewPatterns;
	TRiver river;

	TTerrain id;
	TTerrain rockTerrain;
	int moveCost;
	int horseSoundId;
	ui8 passabilityType;
	bool transitionRequired;
	
	TerrainType(const std::string & name = "");

	TerrainType& operator=(const TerrainType & _type);
	
	bool operator==(const TerrainType & other);
	bool operator!=(const TerrainType & other);
	bool operator<(const TerrainType & other);
	
	bool isLand() const;
	bool isWater() const;
	bool isPassable() const;
	bool isSurface() const;
	bool isUnderground() const;
	bool isTransitionRequired() const;
		
	operator std::string() const;
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & battleFields;
		h & prohibitTransitions;
		h & minimapBlocked;
		h & minimapUnblocked;
		h & name;
		h & musicFilename;
		h & tilesFilename;
		h & terrainText;
		h & typeCode;
		h & terrainViewPatterns;
		h & rockTerrain;
		h & river;

		h & id;
		h & moveCost;
		h & horseSoundId;
		h & passabilityType;
		h & transitionRequired;
	}
};

class DLL_LINKAGE RiverType
{
public:

	std::string fileName;
	std::string code;
	std::string deltaName;
	TRiver id;

	RiverType(const std::string & fileName = "", const std::string& code = "", TRiver id = River::NO_RIVER);

	template <typename Handler> void serialize(Handler& h, const int version)
	{
		h & fileName;
		h & code;
		h & deltaName;
		h & id;
	}
};

class DLL_LINKAGE RoadType
{
public:
	std::string fileName;
	std::string code;
	TRoad id;
	ui8 movementCost;

	RoadType(const std::string & fileName = "", const std::string& code = "", TRoad id = Road::NO_ROAD);

	template <typename Handler> void serialize(Handler& h, const int version)
	{
		h & fileName;
		h & code;
		h & id;
		h & movementCost;
	}
};

DLL_LINKAGE std::ostream & operator<<(std::ostream & os, const TerrainType & terrainType);

class DLL_LINKAGE TerrainTypeHandler //TODO: public IHandlerBase ?
{
public:

	TerrainTypeHandler();
	void initRivers();
	void initRoads();

	const std::vector<TerrainType *> & terrains() const;
	const TerrainType * getInfoByName(const std::string & terrainName) const;
	const TerrainType * getInfoByCode(const std::string & terrainCode) const;
	const TerrainType * getInfoById(TTerrain id) const;

	const std::vector<RiverType *> & rivers() const;
	const RiverType * getRiverByName(const std::string & riverName) const;
	const RiverType * getRiverByCode(const std::string & riverCode) const;
	const RiverType * getRiverById(TRiver id) const;

	const std::vector<RoadType *> & roads() const;
	const RoadType * getRoadByName(const std::string & roadName) const;
	const RoadType * getRoadByCode(const std::string & roadCode) const;
	const RoadType * getRoadById(TRoad id) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objects;
		h & riverTypes;
		h & roadTypes;

		if (!h.saving)
		{
			recreateTerrainMaps();
			recreateRiverMaps();
			recreateRoadMaps();
		}
	}

private:

	std::vector<TerrainType *> objects;
	std::vector<RiverType *> riverTypes;
	std::vector<RoadType *> roadTypes;

	std::unordered_map<std::string, const TerrainType*> terrainInfoByName;
	std::unordered_map<std::string, const TerrainType*> terrainInfoByCode;
	std::unordered_map<TTerrain, const TerrainType*> terrainInfoById;

	std::unordered_map<std::string, const RiverType*> riverInfoByName;
	std::unordered_map<std::string, const RiverType*> riverInfoByCode;
	std::unordered_map<TRiver, const RiverType*> riverInfoById;

	std::unordered_map<std::string, const RoadType*> roadInfoByName;
	std::unordered_map<std::string, const RoadType*> roadInfoByCode;
	std::unordered_map<TRoad, const RoadType*> roadInfoById;

	void recreateTerrainMaps();
	void recreateRiverMaps();
	void recreateRoadMaps();

};
