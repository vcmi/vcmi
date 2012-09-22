#pragma once

#include "ConstTransitivePtr.h"
#include "ResourceSet.h"
#include "int3.h"
//#include "GameConstants.h"

/*
 * CTownHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CLegacyConfigParser;
class JsonNode;

/// a typical building encountered in every castle ;]
/// this is structure available to both client and server
/// contains all mechanics-related data about town structures
class DLL_LINKAGE CBuilding
{
	typedef si32 BuildingType;//TODO: replace int with pointer?

	std::string name;
	std::string description;

public:
	si32 tid, bid; //town ID and structure ID
	TResources resources;

	std::set<BuildingType> requirements; /// set of required buildings, includes upgradeOf;
	BuildingType upgrade; /// indicates that building "upgrade" can be improved by this, -1 = empty

	enum EBuildMode
	{
		BUILD_NORMAL,  // 0 - normal, default
		BUILD_AUTO,    // 1 - auto - building appears when all requirements are built
		BUILD_SPECIAL, // 2 - special - building can not be built normally
		BUILD_GRAIL    // 3 - grail - building reqires grail to be built
	};
	ui32 mode;

	const std::string &Name() const;
	const std::string &Description() const;

	//return base of upgrade(s) or this
	BuildingType getBase() const;

	// returns how many times build has to be upgraded to become build
	si32 getDistance(BuildingType build) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid & bid & resources & name & description & requirements & upgrade & mode;
	}

	friend class CTownHandler;
};

/// This is structure used only by client
/// Consists of all gui-related data about town structures
/// Should be moved from lib to client
struct DLL_LINKAGE CStructure
{
	CBuilding * building;  // base building. If null - this structure will be always present on screen
	CBuilding * buildable; // building that will be used to determine built building and visible cost. Usually same as "building"

	bool hiddenUpgrade; // used only if "building" is upgrade, if true - structure on town screen will behave exactly like parent (mouse clicks, hover texts, etc)

	int3 pos;
	std::string defName, borderName, areaName;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & pos & defName & borderName & areaName & building & buildable;
	}
};

class DLL_LINKAGE CTown
{
	std::string name;
	std::string description;

	std::vector<std::string> names; //names of the town instances

public:
	ui32 typeID;//also works as factionID

	/// level -> list of creatures on this tier
	// TODO: replace with pointers to CCreature
	std::vector<std::vector<si32> > creatures;

	bmap<int, ConstTransitivePtr<CBuilding> > buildings;

	// should be removed at least from configs in favour of auto-detection
	std::map<int,int> hordeLvl; //[0] - first horde building creature level; [1] - second horde building (-1 if not present)
	ui32 mageLevel; //max available mage guild level
	int bonus; //pic number
	ui16 primaryRes, warMachine;

	// Client-only data. Should be moved away from lib
	struct ClientInfo
	{
		//icons [fort is present?][build limit reached?] -> index of icon in def files
		int icons[2][2];

		std::string musicTheme;
		std::string townBackground;
		std::string guildWindow;
		std::string buildingsIcons;
		std::string hallBackground;
		/// vector[row][column] = list of buildings in this slot
		std::vector< std::vector< std::vector<int> > > hallSlots;

		/// list of town screen structures.
		/// NOTE: index in vector is meaningless. Vector used instead of list for a bit faster access
		std::vector<ConstTransitivePtr<CStructure> > structures;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & icons & musicTheme & townBackground & guildWindow & buildingsIcons & hallBackground & hallSlots & structures;
		}
	} clientInfo;

	const std::vector<std::string> & Names() const;
	const std::string & Name() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name & names & typeID & creatures & buildings & hordeLvl & mageLevel & bonus
			& primaryRes & warMachine & clientInfo;
	}

	friend class CTownHandler;
};

struct DLL_LINKAGE SPuzzleInfo
{
	ui16 number; //type of puzzle
	si16 x, y; //position
	ui16 whenUncovered; //determines the sequnce of discovering (the lesser it is the sooner puzzle will be discovered)
	std::string filename; //file with graphic of this puzzle

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & number & x & y & whenUncovered & filename;
	}
};

class CFaction
{
public:
	std::string name; //reference name, usually lower case
	ui32 factionID;

	ui32 nativeTerrain;

	std::string creatureBg120;
	std::string creatureBg130;

	std::vector<SPuzzleInfo> puzzleMap;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name & factionID & nativeTerrain & creatureBg120 & creatureBg130 & puzzleMap;
	}
};

class DLL_LINKAGE CTownHandler
{
	/// loads CBuilding's into town
	void loadBuilding(CTown &town, const JsonNode & source);
	void loadBuildings(CTown &town, const JsonNode & source);

	/// loads CStructure's into town
	void loadStructure(CTown &town, const JsonNode & source);
	void loadStructures(CTown &town, const JsonNode & source);

	/// loads town hall vector (hallSlots)
	void loadTownHall(CTown &town, const JsonNode & source);

	void loadClientData(CTown &town, const JsonNode & source);

	void loadTown(CTown &town, const JsonNode & source);

	void loadPuzzle(CFaction & faction, const JsonNode & source);

	/// main loading function, accepts merged JSON source and add all entries from it into game
	/// all entries in JSON should be checked for validness before using this function
	void loadFactions(const JsonNode & source);

	/// load all available data from h3 txt(s) into json structure using format similar to vcmi configs
	/// returns 2d array [townID] [buildID] of buildings
	void loadLegacyData(JsonNode & dest);

public:
	std::map<ui32, CTown> towns;
	std::map<ui32, CFaction> factions;

	CTownHandler(); //c-tor, set pointer in VLC to this

	/// "entry point" for towns loading.
	/// reads legacy txt's from H3 + vcmi json, merges them
	/// and loads resulting structure to game using loadTowns method
	/// in future may require loaded Creature Handler
	void load();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & towns & factions;
	}
};
