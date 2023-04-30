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
#include "HeroBonus.h"
#include "Point.h"
#include "mapObjects/CRewardableConstructor.h"

VCMI_LIB_NAMESPACE_BEGIN

class CLegacyConfigParser;
class JsonNode;
class CTown;
class CFaction;
struct BattleHex;
class JsonSerializeFormat;

/// a typical building encountered in every castle ;]
/// this is structure available to both client and server
/// contains all mechanics-related data about town structures



class DLL_LINKAGE CBuilding
{
	std::string modScope;
	std::string identifier;

public:
	typedef LogicalExpression<BuildingID> TRequired;

	CTown * town; // town this building belongs to
	TResources resources;
	TResources produce;
	TRequired requirements;

	BuildingID bid; //structure ID
	BuildingID upgrade; /// indicates that building "upgrade" can be improved by this, -1 = empty
	BuildingSubID::EBuildingSubID subId; /// subtype for special buildings, -1 = the building is not special
	std::set<BuildingID> overrideBids; /// the building which bonuses should be overridden with bonuses of the current building
	BonusList buildingBonuses;
	BonusList onVisitBonuses;
	
	Rewardable::Info rewardableObjectInfo; ///configurable rewards for special buildings

	enum EBuildMode
	{
		BUILD_NORMAL,  // 0 - normal, default
		BUILD_AUTO,    // 1 - auto - building appears when all requirements are built
		BUILD_SPECIAL, // 2 - special - building can not be built normally
		BUILD_GRAIL    // 3 - grail - building reqires grail to be built
	} mode;

	enum ETowerHeight // for lookup towers and some grails
	{
		HEIGHT_NO_TOWER = 5, // building has not 'lookout tower' ability
		HEIGHT_LOW = 10,     // low lookout tower, but castle without lookout tower gives radius 5
		HEIGHT_AVERAGE = 15,
		HEIGHT_HIGH = 20,    // such tower is in the Tower town
		HEIGHT_SKYSHIP = std::numeric_limits<int>::max()  // grail, open entire map
	} height;

	static const std::map<std::string, CBuilding::EBuildMode> MODES;
	static const std::map<std::string, CBuilding::ETowerHeight> TOWER_TYPES;

	CBuilding() : town(nullptr), mode(BUILD_NORMAL) {};

	std::string getJsonKey() const;

	std::string getNameTranslated() const;
	std::string getDescriptionTranslated() const;

	std::string getNameTextID() const;
	std::string getDescriptionTextID() const;

	//return base of upgrade(s) or this
	BuildingID getBase() const;

	// returns how many times build has to be upgraded to become build
	si32 getDistance(const BuildingID & build) const;

	STRONG_INLINE
	bool IsTradeBuilding() const
	{
		return bid == BuildingID::MARKETPLACE || subId == BuildingSubID::ARTIFACT_MERCHANT || subId == BuildingSubID::FREELANCERS_GUILD;
	}

	STRONG_INLINE
	bool IsWeekBonus() const
	{
		return subId == BuildingSubID::STABLES || subId == BuildingSubID::MANA_VORTEX;
	}

	STRONG_INLINE
	bool IsVisitingBonus() const
	{
		return subId == BuildingSubID::ATTACK_VISITING_BONUS ||
			subId == BuildingSubID::DEFENSE_VISITING_BONUS ||
			subId == BuildingSubID::SPELL_POWER_VISITING_BONUS ||
			subId == BuildingSubID::KNOWLEDGE_VISITING_BONUS ||
			subId == BuildingSubID::EXPERIENCE_VISITING_BONUS ||
			subId == BuildingSubID::CUSTOM_VISITING_BONUS;
	}

	void addNewBonus(const std::shared_ptr<Bonus> & b, BonusList & bonusList) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & modScope;
		h & identifier;
		h & town;
		h & bid;
		h & resources;
		h & produce;
		h & requirements;
		h & upgrade;
		h & mode;
		h & subId;
		h & height;
		h & overrideBids;
		h & buildingBonuses;
		h & onVisitBonuses;
		h & rewardableObjectInfo;
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
	friend class CTownHandler;
	friend class CBuilding;
	friend class CTown;

	std::string modScope;
	std::string identifier;

	FactionID index = FactionID::NEUTRAL;

	FactionID getFaction() const override; //This function should not be used

public:
	TerrainId nativeTerrain;
	EAlignment alignment = EAlignment::NEUTRAL;
	bool preferUndergroundPlacement = false;

	CTown * town = nullptr; //NOTE: can be null

	std::string creatureBg120;
	std::string creatureBg130;

	std::vector<SPuzzleInfo> puzzleMap;

	CFaction() = default;
	~CFaction();

	int32_t getIndex() const override;
	int32_t getIconIndex() const override;
	std::string getJsonKey() const override;
	void registerIcons(const IconRegistar & cb) const override;
	FactionID getId() const override;

	std::string getNameTranslated() const override;
	std::string getNameTextID() const override;

	bool hasTown() const override;
	TerrainId getNativeTerrain() const override;
	EAlignment getAlignment() const override;

	void updateFrom(const JsonNode & data);
	void serializeJson(JsonSerializeFormat & handler);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & modScope;
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
	friend class CTownHandler;
	size_t namesCount = 0;

public:
	CTown();
	~CTown();

	std::string getBuildingScope() const;
	std::set<si32> getAllBuildings() const;
	const CBuilding * getSpecialBuilding(BuildingSubID::EBuildingSubID subID) const;
	std::string getGreeting(BuildingSubID::EBuildingSubID subID) const;
	void setGreeting(BuildingSubID::EBuildingSubID subID, const std::string & message) const; //may affect only mutable field
	BuildingID::EBuildingID getBuildingType(BuildingSubID::EBuildingSubID subID) const;

	std::string getRandomNameTranslated(size_t index) const;
	std::string getRandomNameTextID(size_t index) const;
	size_t getRandomNamesCount() const;

	CFaction * faction;

	/// level -> list of creatures on this tier
	// TODO: replace with pointers to CCreature
	std::vector<std::vector<CreatureID> > creatures;

	std::map<BuildingID, ConstTransitivePtr<CBuilding> > buildings;

	std::vector<std::string> dwellings; //defs for adventure map dwellings for new towns, [0] means tier 1 creatures etc.
	std::vector<std::string> dwellingNames;

	// should be removed at least from configs in favor of auto-detection
	std::map<int,int> hordeLvl; //[0] - first horde building creature level; [1] - second horde building (-1 if not present)
	ui32 mageLevel; //max available mage guild level
	GameResID primaryRes;
	ArtifactID warMachine;
	SpellID moatAbility;
	// default chance for hero of specific class to appear in tavern, if field "tavern" was not set
	// resulting chance = sqrt(town.chance * heroClass.chance)
	ui32 defaultTavernChance;

	// Client-only data. Should be moved away from lib
	struct ClientInfo
	{
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
		std::string towerIconSmall;
		std::string towerIconLarge;

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
			h & towerIconSmall;
			h & towerIconLarge;
		}
	} clientInfo;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & namesCount;
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
		h & moatAbility;
		h & defaultTavernChance;
	}
	
private:
	///generated bonusing buildings messages for all towns of this type.
	mutable std::map<BuildingSubID::EBuildingSubID, const std::string> specialMessages; //may be changed by CGTownBuilding::getVisitingBonusGreeting() const
};

class DLL_LINKAGE CTownHandler : public CHandlerBase<FactionID, Faction, CFaction, FactionService>
{
	struct BuildingRequirementsHelper
	{
		JsonNode json;
		CBuilding * building;
		CTown * town;
	};

	std::map<CTown *, JsonNode> warMachinesToLoad;
	std::vector<BuildingRequirementsHelper> requirementsToLoad;
	std::vector<BuildingRequirementsHelper> overriddenBidsToLoad; //list of buildings, which bonuses should be overridden.

	static TPropagatorPtr & emptyPropagator();

	void initializeRequirements();
	void initializeOverridden();
	void initializeWarMachines();

	/// loads CBuilding's into town
	void loadBuildingRequirements(CBuilding * building, const JsonNode & source, std::vector<BuildingRequirementsHelper> & bidsToLoad) const;
	void loadBuilding(CTown * town, const std::string & stringID, const JsonNode & source);
	void loadBuildings(CTown * town, const JsonNode & source);

	std::shared_ptr<Bonus> createBonus(CBuilding * build, Bonus::BonusType type, int val, int subtype = -1) const;
	std::shared_ptr<Bonus> createBonus(CBuilding * build, Bonus::BonusType type, int val, TPropagatorPtr & prop, int subtype = -1) const;
	std::shared_ptr<Bonus> createBonusImpl(const BuildingID & building,
												  Bonus::BonusType type,
												  int val,
												  TPropagatorPtr & prop,
												  const std::string & description,
												  int subtype = -1) const;

	/// loads CStructure's into town
	void loadStructure(CTown & town, const std::string & stringID, const JsonNode & source) const;
	void loadStructures(CTown & town, const JsonNode & source) const;

	/// loads town hall vector (hallSlots)
	void loadTownHall(CTown & town, const JsonNode & source) const;
	void loadSiegeScreen(CTown & town, const JsonNode & source) const;

	void loadClientData(CTown & town, const JsonNode & source) const;

	void loadTown(CTown * town, const JsonNode & source);

	void loadPuzzle(CFaction & faction, const JsonNode & source) const;

	void loadRandomFaction();


public:
	template<typename R, typename K>
	static R getMappedValue(const K key, const R defval, const std::map<K, R> & map, bool required = true);
	template<typename R>
	static R getMappedValue(const JsonNode & node, const R defval, const std::map<std::string, R> & map, bool required = true);

	CTown * randomTown;
	CFaction * randomFaction;

	CTownHandler();
	~CTownHandler();

	std::vector<JsonNode> loadLegacyData() override;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;
	void addBonusesForVanilaBuilding(CBuilding * building) const;

	void loadCustom() override;
	void afterLoadFinalization() override;

	std::vector<bool> getDefaultAllowed() const override;
	std::set<FactionID> getAllowedFactions(bool withTown = true) const;

	static void loadSpecialBuildingBonuses(const JsonNode & source, BonusList & bonusList, CBuilding * building);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objects;
		h & randomTown;
	}

protected:
	const std::vector<std::string> & getTypeNames() const override;
	CFaction * loadFromJson(const std::string & scope, const JsonNode & data, const std::string & identifier, size_t index) override;
};

VCMI_LIB_NAMESPACE_END
