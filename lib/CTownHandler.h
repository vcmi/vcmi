#pragma once

#include "ConstTransitivePtr.h"
#include "ResourceSet.h"
#include "int3.h"
#include "GameConstants.h"
#include "IHandlerBase.h"

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
class CTown;
class CFaction;

/// a typical building encountered in every castle ;]
/// this is structure available to both client and server
/// contains all mechanics-related data about town structures
class DLL_LINKAGE CBuilding
{

	std::string name;
	std::string description;

public:
	CTown * town; // town this building belongs to
	BuildingID bid; //structure ID
	TResources resources;

	std::set<BuildingID> requirements; /// set of required buildings, includes upgradeOf;
	BuildingID upgrade; /// indicates that building "upgrade" can be improved by this, -1 = empty

	enum EBuildMode
	{
		BUILD_NORMAL,  // 0 - normal, default
		BUILD_AUTO,    // 1 - auto - building appears when all requirements are built
		BUILD_SPECIAL, // 2 - special - building can not be built normally
		BUILD_GRAIL    // 3 - grail - building reqires grail to be built
	} mode;

	const std::string &Name() const;
	const std::string &Description() const;

	//return base of upgrade(s) or this
	BuildingID getBase() const;

	// returns how many times build has to be upgraded to become build
	si32 getDistance(BuildingID build) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & town & bid & resources & name & description & requirements & upgrade & mode;
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
		h & pos & defName & borderName & areaName & building & buildable & hiddenUpgrade;
	}
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

class DLL_LINKAGE CFaction
{
public:
	CFaction();
	~CFaction();

	std::string name; //town name, by default - from TownName.txt

	TFaction index;

	ETerrainType nativeTerrain;
	EAlignment::EAlignment alignment;

	CreatureID commander;

	CTown * town; //NOTE: can be null

	std::string creatureBg120;
	std::string creatureBg130;

	std::vector<SPuzzleInfo> puzzleMap;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name & index & nativeTerrain & alignment & commander & town & creatureBg120 & creatureBg130 & puzzleMap;
	}
};

class DLL_LINKAGE CTown
{
public:
	CTown();
	~CTown();

	CFaction * faction;

	std::vector<std::string> names; //names of the town instances

	/// level -> list of creatures on this tier
	// TODO: replace with pointers to CCreature
	std::vector<std::vector<CreatureID> > creatures;

	bmap<BuildingID, ConstTransitivePtr<CBuilding> > buildings;

	std::vector<std::string> dwellings; //defs for adventure map dwellings for new towns, [0] means tier 1 creatures etc.
	std::vector<std::string> dwellingNames;

	// should be removed at least from configs in favor of auto-detection
	std::map<int,int> hordeLvl; //[0] - first horde building creature level; [1] - second horde building (-1 if not present)
	ui32 mageLevel; //max available mage guild level
	ui16 primaryRes;
	ArtifactID warMachine;
	si32 moatDamage;

	// Client-only data. Should be moved away from lib
	struct ClientInfo
	{
		struct Point
		{
			si32 x;
			si32 y;

			template <typename Handler> void serialize(Handler &h, const int version)
			{ h & x & y; }
		};

		//icons [fort is present?][build limit reached?] -> index of icon in def files
		int icons[2][2];
		std::string iconSmall[2][2]; /// icon names used during loading
		std::string iconLarge[2][2];

		std::string musicTheme;
		std::string townBackground;
		std::string guildWindow;
		std::string buildingsIcons;
		std::string hallBackground;
		/// vector[row][column] = list of buildings in this slot
		std::vector< std::vector< std::vector<BuildingID> > > hallSlots;

		/// list of town screen structures.
		/// NOTE: index in vector is meaningless. Vector used instead of list for a bit faster access
		std::vector<ConstTransitivePtr<CStructure> > structures;

		std::string advMapVillage;
		std::string advMapCastle;
		std::string advMapCapitol;

		std::string siegePrefix;
		std::vector<Point> siegePositions;
		CreatureID siegeShooter; // shooter creature ID

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & icons & iconSmall & iconLarge & musicTheme & townBackground & guildWindow & buildingsIcons & hallBackground;
			h & advMapVillage & advMapCastle & advMapCapitol & hallSlots & structures;
			h & siegePrefix & siegePositions & siegeShooter;
		}
	} clientInfo;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & names & faction & creatures & dwellings & dwellingNames & buildings & hordeLvl & mageLevel
			& primaryRes & warMachine & clientInfo & moatDamage;
	}
};

class DLL_LINKAGE CTownHandler : public IHandlerBase
{
	/// loads CBuilding's into town
	void loadBuilding(CTown &town, const JsonNode & source);
	void loadBuildings(CTown &town, const JsonNode & source);

	/// loads CStructure's into town
	void loadStructure(CTown &town, const JsonNode & source);
	void loadStructures(CTown &town, const JsonNode & source);

	/// loads town hall vector (hallSlots)
	void loadTownHall(CTown &town, const JsonNode & source);
	void loadSiegeScreen(CTown &town, const JsonNode & source);

	void loadClientData(CTown &town, const JsonNode & source);

	void loadTown(CTown &town, const JsonNode & source);

	void loadPuzzle(CFaction & faction, const JsonNode & source);

	CFaction * loadFromJson(const JsonNode & data);

public:
	std::vector<ConstTransitivePtr<CFaction> > factions;

	CTownHandler(); //c-tor, set pointer in VLC to this
	~CTownHandler();

	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	std::vector<bool> getDefaultAllowed() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & factions;
	}
};
