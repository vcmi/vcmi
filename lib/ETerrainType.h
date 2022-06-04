/*
 * ETerrainType.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ConstTransitivePtr.h"
#include "JsonNode.h"

class DLL_LINKAGE ETerrainType
{
public:
	/*enum EETerrainType
	 {
	 ANY_TERRAIN = -3,
	 WRONG = -2, BORDER = -1, DIRT, SAND, GRASS, SNOW, SWAMP,
	 ROUGH, SUBTERRANEAN, LAVA, WATER, ROCK // ROCK is also intended to be max value.
	 };*/
	static std::vector<ETerrainType> & terrains();
	static const std::set<ETerrainType> DEFAULT_TERRAIN_TYPES;
	
	ETerrainType(const std::string & _type = "");
	ETerrainType(int _typeId);
	
	int id() const;
	
	ETerrainType& operator=(const ETerrainType & _type);
	ETerrainType& operator=(const std::string & _type);
	
	DLL_LINKAGE friend bool operator==(const ETerrainType & l, const ETerrainType & r);
	DLL_LINKAGE friend bool operator!=(const ETerrainType & l, const ETerrainType & r);
	DLL_LINKAGE friend bool operator<(const ETerrainType & l, const ETerrainType & r);
	
	static const ETerrainType ANY;
	
	bool isLand() const;
	bool isWater() const;
	bool isPassable() const; //ROCK
	bool isUnderground() const;
	
	void setWater(bool);
	
	std::string toString() const;
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & type;
	}
	
protected:
	
	std::string type;
};

DLL_LINKAGE std::ostream & operator<<(std::ostream & os, const ETerrainType terrainType);
