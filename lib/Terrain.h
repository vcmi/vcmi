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
	
	struct Info
	{
		enum class Type
		{
			Land, Water, Subterranean, Rock
		};
		
		int moveCost;
		std::array<int, 3> minimapBlocked;
		std::array<int, 3> minimapUnblocked;
		std::string musicFilename;
		std::string tilesFilename;
		int horseSoundId;
		Type type;
	};
	
	class DLL_LINKAGE Manager
	{
	public:
		static std::vector<Terrain> terrains();
		static const Info & getInfo(const Terrain &);
		
	private:
		static Manager & get();
		Manager();
		
		std::map<Terrain, Info> terrainInfo;
	};
	
	/*enum EETerrainType
	 {
	 ANY_TERRAIN = -3,
	 WRONG = -2, BORDER = -1, DIRT, SAND, GRASS, SNOW, SWAMP,
	 ROUGH, SUBTERRANEAN, LAVA, WATER, ROCK // ROCK is also intended to be max value.
	 };*/
	
	Terrain(const std::string & _type = "");
	static Terrain createTerrainTypeH3M(int tId);
	
	int id() const; //TODO: has to be completely removed
	
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
		
	operator std::string() const;
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name;
	}
	
protected:
	
	std::string name;
};

DLL_LINKAGE std::ostream & operator<<(std::ostream & os, const Terrain terrainType);
