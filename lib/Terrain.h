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

VCMI_LIB_NAMESPACE_BEGIN

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
		bool transitionRequired;
		std::array<int, 3> minimapBlocked;
		std::array<int, 3> minimapUnblocked;
		std::string musicFilename;
		std::string tilesFilename;
		std::string terrainText;
		std::string typeCode;
		std::string terrainViewPatterns;
		std::string rockTerrain;
		std::string river;
		int horseSoundId;
		Type type;
		std::vector<std::string> battleFields;
		std::vector<Terrain> prohibitTransitions;
	};
	
	class DLL_LINKAGE Manager
	{
	public:
		static const std::vector<Terrain> & terrains();
		static const Info & getInfo(const Terrain &);
		static int id(const Terrain &);
		
	private:
		static Manager & get();
		Manager();
		
		std::unordered_map<std::string, Info> terrainInfo;
		std::vector<Terrain> terrainVault;
		std::map<Terrain, int> terrainId;
	};
	
	/*enum EETerrainType
	 {
	 ANY_TERRAIN = -3,
	 WRONG = -2, BORDER = -1, DIRT, SAND, GRASS, SNOW, SWAMP,
	 ROUGH, SUBTERRANEAN, LAVA, WATER, ROCK // ROCK is also intended to be max value.
	 };*/
	
	Terrain(const std::string & _type = "");
	static Terrain createTerrainTypeH3M(int tId);
	static Terrain createTerrainByCode(const std::string & typeCode);
	
	int id() const; //TODO: has to be completely removed
	
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
	bool isTransitionRequired() const;
	
		
	operator std::string() const;
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name;
	}
	
protected:
	
	std::string name;
};

DLL_LINKAGE std::ostream & operator<<(std::ostream & os, const Terrain terrainType);

VCMI_LIB_NAMESPACE_END
