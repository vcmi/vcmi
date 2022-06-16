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
#include "JsonNode.h"

class DLL_LINKAGE Terrain
{
public:
	
	friend class Manager;
	
	class DLL_LINKAGE Manager
	{
	public:
		static std::vector<Terrain> terrains();
		static const JsonNode & getInfo(const Terrain &);
		
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
	
	Terrain(const std::string & _type = "");
	static Terrain createTerrainTypeH3M(int tId);
	
	int id() const; //TODO: has to be complitely removed
	
	Terrain& operator=(const Terrain & _type);
	Terrain& operator=(const std::string & _type);
	
	DLL_LINKAGE friend bool operator==(const Terrain & l, const Terrain & r);
	DLL_LINKAGE friend bool operator!=(const Terrain & l, const Terrain & r);
	DLL_LINKAGE friend bool operator<(const Terrain & l, const Terrain & r);
	
	static const Terrain ANY;
	
	bool isLand() const;
	bool isWater() const;
	bool isPassable() const; //ROCK
	bool isUnderground() const;
	bool isNative() const;
	
	void setWater(bool);
	
	std::string toString() const;
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name;
	}
	
protected:
	
	std::string name;
};

DLL_LINKAGE void loadTerrainTypes();

DLL_LINKAGE std::ostream & operator<<(std::ostream & os, const Terrain terrainType);
