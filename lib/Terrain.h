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
	
	enum class PassabilityType
	{
		LAND,
		WATER,
		SUBTERRANEAN,
		ROCK
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
	std::string rockTerrain;
	std::string river;

	TTerrain id;
	int moveCost;
	int horseSoundId;
	PassabilityType passabilityType;
	bool transitionRequired;
	
	TerrainType(const std::string & _type = "");
	
	TerrainType& operator=(const TerrainType & _type);
	
	bool operator==(const TerrainType & other);
	bool operator!=(const TerrainType & other);
	bool operator<(const TerrainType & other);
	
	bool isLand() const;
	bool isWater() const;
	bool isPassable() const; //ROCK
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

DLL_LINKAGE std::ostream & operator<<(std::ostream & os, const TerrainType & terrainType);

class DLL_LINKAGE TerrainTypeHandler //TODO: handlerBase
{
public:

	TerrainTypeHandler();

	const std::vector<TerrainType *> & terrains();
	const TerrainType * getInfoByName(const std::string & terrainName) const;
	const TerrainType * getInfoByCode(const std::string & terrainName) const;
	const TerrainType * getInfoById(TTerrain id) const;

	//TODO: road, river types?

private:

	std::vector<TerrainType *> objects;

	std::unordered_map<std::string, const TerrainType*> terrainInfoByName;
	std::unordered_map<std::string, const TerrainType*> terrainInfoByCode;
	std::unordered_map<TTerrain, const TerrainType*> terrainInfoById;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name;

		if (!saving)
		{
			//TODO: recreate at load
		}
		/*
		h & terrainInfoByName;
		h & terrainInfoByCode;
		h & terrainInfoById;
		*/
	}
};
