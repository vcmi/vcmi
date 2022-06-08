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
	
	friend class Manager;
	
	class Manager
	{
	public:
		static std::vector<ETerrainType> terrains();
		static const JsonNode & getInfo(const ETerrainType &);
		
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
	
	ETerrainType(const std::string & _type = "");
	static ETerrainType createTerrainTypeH3M(int tId);
	
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

DLL_LINKAGE void loadTerrainTypes();

DLL_LINKAGE std::ostream & operator<<(std::ostream & os, const ETerrainType terrainType);
