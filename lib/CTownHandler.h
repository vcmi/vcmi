/*
 * CTownHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/Faction.h>
#include <vcmi/FactionService.h>

#include "ConstTransitivePtr.h"
#include "ResourceSet.h"
#include "int3.h"
#include "GameConstants.h"
#include "IHandlerBase.h"
#include "LogicalExpression.h"
#include "battle/BattleHex.h"

class CLegacyConfigParser;
class JsonNode;
class CTown;
class CFaction;
struct BattleHex;

/// a typical building encountered in every castle ;]
/// this is structure available to both client and server
/// contains all mechanics-related data about town structures



class DLL_LINKAGE CBuilding
{

	std::string name;
	std::string description;

public:
	typedef LogicalExpression<BuildingID> TRequired;

	CTown * town; // town this building belongs to
	TResources resources;
	TResources produce;
	TRequired requirements;
	std::string identifier;

	BuildingID bid; //structure ID
	BuildingID upgrade; /// indicates that building "upgrade" can be improved by this, -1 = empty

	enum EBuildMode
	{
		BUILD_NORMAL,  // 0 - normal, default
		BUILD_AUTO,    // 1 - auto - building appears when all requirements are built
		BUILD_SPECIAL, // 2 - special - building can not be built normally
		BUILD_GRAIL    // 3 - grail - building reqires grail to be built
	} mode;

	CBuilding();

	const std::string &Name() const;
	const std::string &Description() const;

	//return base of upgrade(s) or this
	BuildingID getBase() const;

	// returns how many times build has to be upgraded to become build
	si32 getDistance(BuildingID build) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & identifier;
		h & town;
		h & bid;
		h & resources;
		h & produce;
		h & name;
		h & description;
		h & requirements;
		h & upgrade;
		h & mode;
		if(!h.saving)
			deserializeFix();
	}

	friend class CTownHandler;

private:

	void deserializeFix();
};

/// This is structure used only by client
/// Consists of all gui-related data about town structures
/// Should be moved from lib to client
struct DLL_LINKAGE CStructure
{
	CBuilding * building;  // base building. If null - this structure will be always present on screen
	CBuilding * buildable; // building that will be used to determine built building and visible cost. Usually same as "building"

	int3 pos;
	std::string defName, borderName, areaName, identifier;

	bool hiddenUpgrade; // used only if "building" is upgrade, if true - structure on town screen will behave exactly like parent (mouse clicks, hover texts, etc)
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & pos;
		h & defName;
		h & borderName;
		h & areaName;
		h & identifier;
		h & building;
		h & buildable;
		h & hiddenUpgrade;
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
		h & number;
		h & x;
		h & y;
		h & whenUncovered;
		h & filename;
	}
};

class DLL_LINKAGE CFaction : public Faction
{
public:
	std::string name; //town name, by default - from TownName.txt
	std::string identifier;

	TFaction index;

	ETerrainType nativeTerrain;
	EAlignment::EAlignment alignment;

	CTown * town; //NOTE: can be null

	std::string creatureBg120;
	std::string creatureBg130;

	std::vector<SPuzzleInfo> puzzleMap;

	CFaction();
	~CFaction();

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	const std::string & getName() const override;
	const std::string & getJsonKey() const override;
	void registerIcons(const IconRegistar & cb) const override;
	FactionID getId() const override;

	bool hasTown() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name;
		h & identifier;
		h & index;
		h & nativeTerrain;
		h & alignment;
		h & town;
		h & creatureBg120;
		h & creatureBg130;
		h & puzzleMap;
	}
};

class DLL_LINKAGE CTown
{
public:
	CTown();
	~CTown();
	// TODO: remove once save and mod compatability not needed
	static std::vector<BattleHex> defaultMoatHexes();

	std::string getFactionName() const;
	std::string getBuildingScope() const;
	std::set<si32> getAllBuildings() const;

	CFaction * faction;

	std::vector<std::string> names; //names of the town instances

	/// level -> list of creatures on this tier
	// TODO: replace with pointers to CCreature
	std::vector<std::vector<CreatureID> > creatures;

	std::map<BuildingID, ConstTransitivePtr<CBuilding> > buildings;

	std::vector<std::string> dwellings; //defs for adventure map dwellings for new towns, [0] means tier 1 creatures etc.
	std::vector<std::string> dwellingNames;

	// should be removed at least from configs in favor of auto-detection
	std::map<int,int> hordeLvl; //[0] - first horde building creature level; [1] - second horde building (-1 if not present)
	ui32 mageLevel; //max available mage guild level
	ui16 primaryRes;
	ArtifactID warMachine;
	si32 moatDamage;
	std::vector<BattleHex> moatHexes;
	// default chance for hero of specific class to appear in tavern, if field "tavern" was not set
	// resulting chance = sqrt(town.chance * heroClass.chance)
	ui32 defaultTavernChance;

	// Client-only data. Should be moved away from lib
	struct ClientInfo
	{
		struct Point
		{
			si32 x;
			si32 y;

			template <typename Handler> void serialize(Handler &h, const int version)
			{
				h & x;
				h & y;
			}
		};

		//icons [fort is present?][build limit reached?] -> index of icon in def files
		int icons[2][2];
		std::string iconSmall[2][2]; /// icon names used during loading
		std::string iconLarge[2][2];
		std::string tavernVideo;
		std::string musicTheme;
		std::string townBackground;
		std::string guildBackground;
		std::string guildWindow;
		std::string buildingsIcons;
		std::string hallBackground;
		/// vector[row][column] = list of buildings in this slot
		std::vector< std::vector< std::vector<BuildingID> > > hallSlots;

		/// list of town screen structures.
		/// NOTE: index in vector is meaningless. Vector used instead of list for a bit faster access
		std::vector<ConstTransitivePtr<CStructure> > structures;

		std::string siegePrefix;
		std::vector<Point> siegePositions;
		CreatureID siegeShooter; // shooter creature ID

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & icons;
			h & iconSmall;
			h & iconLarge;
			h & tavernVideo;
			h & musicTheme;
			h & townBackground;
			h & guildBackground;
			h & guildWindow;
			h & buildingsIcons;
			h & hallBackground;
			h & hallSlots;
			h & structures;
			h & siegePrefix;
			h & siegePositions;
			h & siegeShooter;
		}
	} clientInfo;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & names;
		h & faction;
		h & creatures;
		h & dwellings;
		h & dwellingNames;
		h & buildings;
		h & hordeLvl;
		h & mageLevel;
		h & primaryRes;
		h & warMachine;
		h & clientInfo;
		h & moatDamage;
		if(version >= 758)
		{
			h & moatHexes;
		}
		else if(!h.saving)
		{
			moatHexes = defaultMoatHexes();
		}
		h & defaultTavernChance;
	}
};

class DLL_LINKAGE CTownHandler : public IHandlerBase, public FactionService
{
	struct BuildingRequirementsHelper
	{
		JsonNode json;
		CBuilding * building;
		CTown * town;
	};

	std::map<CTown *, JsonNode> warMachinesToLoad;
	std::vector<BuildingRequirementsHelper> requirementsToLoad;
	void initializeRequirements();
	void initializeWarMachines();

	/// loads CBuilding's into town
	void loadBuildingRequirements(CBuilding * building, const JsonNode & source);
	void loadBuilding(CTown * town, const std::string & stringID, const JsonNode & source);
	void loadBuildings(CTown * town, const JsonNode & source);

	/// loads CStructure's into town
	void loadStructure(CTown &town, const std::string & stringID, const JsonNode & source);
	void loadStructures(CTown &town, const JsonNode & source);

	/// loads town hall vector (hallSlots)
	void loadTownHall(CTown &town, const JsonNode & source);
	void loadSiegeScreen(CTown &town, const JsonNode & source);

	void loadClientData(CTown &town, const JsonNode & source);

	void loadTown(CTown * town, const JsonNode & source);

	void loadPuzzle(CFaction & faction, const JsonNode & source);

	CFaction * loadFromJson(const JsonNode & data, const std::string & identifier);

	void loadRandomFaction();
public:
	std::vector<ConstTransitivePtr<CFaction> > factions;

	CTown * randomTown;

	CTownHandler(); //c-tor, set pointer in VLC to this
	~CTownHandler();

	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	void loadCustom() override;
	void afterLoadFinalization() override;

	std::vector<bool> getDefaultAllowed() const override;
	std::set<TFaction> getAllowedFactions(bool withTown = true) const;

	const Entity * getBaseByIndex(const int32_t index) const override;

	const Faction * getById(const FactionID & id) const override;
	const Faction * getByIndex(const int32_t index) const override;

	void forEachBase(const std::function<void(const Entity * entity, bool & stop)> & cb) const override;
	void forEach(const std::function<void(const Faction * entity, bool & stop)> & cb) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & factions;

		if(version >= 770)
		{
			h & randomTown;
		}
		else if(!h.saving)
		{
			loadRandomFaction();
		}
	}
};
