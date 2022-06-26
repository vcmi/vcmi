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

class DLL_LINKAGE BattleField
{
public:
	//   1. sand/shore   2. sand/mesas   3. dirt/birches   4. dirt/hills   5. dirt/pines   6. grass/hills   7. grass/pines
	//8. lava   9. magic plains   10. snow/mountains   11. snow/trees   12. subterranean   13. swamp/trees   14. fiery fields
	//15. rock lands   16. magic clouds   17. lucid pools   18. holy ground   19. clover field   20. evil fog
	//21. "favorable winds" text on magic plains background   22. cursed ground   23. rough   24. ship to ship   25. ship
	/*enum EBFieldType {NONE = -1, NONE2, SAND_SHORE, SAND_MESAS, DIRT_BIRCHES, DIRT_HILLS, DIRT_PINES, GRASS_HILLS,
	 GRASS_PINES, LAVA, MAGIC_PLAINS, SNOW_MOUNTAINS, SNOW_TREES, SUBTERRANEAN, SWAMP_TREES, FIERY_FIELDS,
	 ROCKLANDS, MAGIC_CLOUDS, LUCID_POOLS, HOLY_GROUND, CLOVER_FIELD, EVIL_FOG, FAVORABLE_WINDS, CURSED_GROUND,
	 ROUGH, SHIP_TO_SHIP, SHIP
	 };*/
	
	BattleField(const std::string & type = "") : name(type)
	{}
	
	static const BattleField NONE;
	
	DLL_LINKAGE friend bool operator==(const BattleField & l, const BattleField & r);
	DLL_LINKAGE friend bool operator!=(const BattleField & l, const BattleField & r);
	DLL_LINKAGE friend bool operator<(const BattleField & l, const BattleField & r);
	
	operator std::string() const;
	int hash() const;
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name;
	}

	bool isSpecial() const
	{
		return name.find('_') >= 0; // hack for special battlefields, move to JSON
	}
	
protected:
	
	std::string name;
};

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
		int horseSoundId;
		Type type;
		std::vector<BattleField> battleFields;
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
	static Terrain createTerrainByCode(const std::string & typeCode);
	
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
