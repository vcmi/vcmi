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

class DLL_LINKAGE CTerrainType
{
public:
	
	friend class Manager;
	
	class Manager
	{
	public:
		static std::vector<CTerrainType> terrains();
		static const JsonNode & getInfo(const CTerrainType &);
		
	private:
		static Manager & get();
		Manager();
		
		std::map<std::string, JsonNode> terrainInfo;
	};
	
	/*enum EETerrainType
	 {
	 ANY_TERRAIN = -3,
	 WRONG = -2, BORDER = -1, DIRT, SAND, GRASS, SNOW, SWAMP,
	 ROUGH, SUBTERRANEAN, LAVA, WATER, ROCK // ROCK is also intended to be max value.
	 };*/
	
	CTerrainType(const std::string & _type = "");
	static CTerrainType createTerrainTypeH3M(int tId);
	
	int id() const;
	
	CTerrainType& operator=(const CTerrainType & _type);
	CTerrainType& operator=(const std::string & _type);
	
	DLL_LINKAGE friend bool operator==(const CTerrainType & l, const CTerrainType & r);
	DLL_LINKAGE friend bool operator!=(const CTerrainType & l, const CTerrainType & r);
	DLL_LINKAGE friend bool operator<(const CTerrainType & l, const CTerrainType & r);
	
	static const CTerrainType ANY;
	
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

DLL_LINKAGE void loadTerrainTypes();

DLL_LINKAGE std::ostream & operator<<(std::ostream & os, const CTerrainType terrainType);
