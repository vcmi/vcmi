/*
 * CMap.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../ConstTransitivePtr.h"
#include "../mapObjects/MiscObjects.h" // To serialize static props
#include "../mapObjects/CQuest.h" // To serialize static props
#include "../mapObjects/CGTownInstance.h" // To serialize static props
#include "../ResourceSet.h"
#include "../int3.h"
#include "../GameConstants.h"
#include "../LogicalExpression.h"
#include "../CModHandler.h"
#include "CMapDefines.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArtifactInstance;
class CGObjectInstance;
class CGHeroInstance;
class CCommanderInstance;
class CGCreature;
class CQuest;
class CGTownInstance;
class IModableArt;
class IQuestObject;
class CInputStream;
class CMapEditManager;

/// The hero name struct consists of the hero id and the hero name.
struct DLL_LINKAGE SHeroName
{
	SHeroName();

	int heroId;
	std::string heroName;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & heroId;
		h & heroName;
	}
};

/// The player info constains data about which factions are allowed, AI tactical settings,
/// the main hero name, where to generate the hero, whether the faction should be selected randomly,...
struct DLL_LINKAGE PlayerInfo
{
	PlayerInfo();

	/// Gets the default faction id or -1 for a random faction.
	si8 defaultCastle() const;
	/// Gets the default hero id or -1 for a random hero.
	si8 defaultHero() const;
	bool canAnyonePlay() const;
	bool hasCustomMainHero() const;

	bool canHumanPlay;
	bool canComputerPlay;
	EAiTactic::EAiTactic aiTactic; /// The default value is EAiTactic::RANDOM.

	std::set<FactionID> allowedFactions;
	bool isFactionRandom;

	///main hero instance (VCMI maps only)
	std::string mainHeroInstance;
	/// Player has a random main hero
	bool hasRandomHero;
	/// The default value is -1.
	si32 mainCustomHeroPortrait;
	std::string mainCustomHeroName;
	/// ID of custom hero (only if portrait and hero name are set, otherwise unpredicted value), -1 if none (not always -1)
	si32 mainCustomHeroId;

	std::vector<SHeroName> heroesNames; /// list of placed heroes on the map
	bool hasMainTown; /// The default value is false.
	bool generateHeroAtMainTown; /// The default value is false.
	int3 posOfMainTown;
	TeamID team; /// The default value NO_TEAM


	bool generateHero; /// Unused.
	si32 p7; /// Unknown and unused.
	/// Unused. Count of hero placeholders containing hero type.
	/// WARNING: powerPlaceholders sometimes gives false 0 (eg. even if there is one placeholder), maybe different meaning ???
	ui8 powerPlaceholders;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & p7;
		h & hasRandomHero;
		h & mainCustomHeroId;
		h & canHumanPlay;
		h & canComputerPlay;
		h & aiTactic;
		h & allowedFactions;
		h & isFactionRandom;
		h & mainCustomHeroPortrait;
		h & mainCustomHeroName;
		h & heroesNames;
		h & hasMainTown;
		h & generateHeroAtMainTown;
		h & posOfMainTown;
		h & team;
		h & generateHero;
		h & mainHeroInstance;
	}
};

/// The loss condition describes the condition to lose the game. (e.g. lose all own heroes/castles)
struct DLL_LINKAGE EventCondition
{
	enum EWinLoseType {
		//internal use, deprecated
		HAVE_ARTIFACT,     // type - required artifact
		HAVE_CREATURES,    // type - creatures to collect, value - amount to collect
		HAVE_RESOURCES,    // type - resource ID, value - amount to collect
		HAVE_BUILDING,     // position - town, optional, type - building to build
		CONTROL,           // position - position of object, optional, type - type of object
		DESTROY,           // position - position of object, optional, type - type of object
		TRANSPORT,         // position - where artifact should be transported, type - type of artifact

		//map format version pre 1.0
		DAYS_PASSED,       // value - number of days from start of the game
		IS_HUMAN,          // value - 0 = player is AI, 1 = player is human
		DAYS_WITHOUT_TOWN, // value - how long player can live without town, 0=instakill
		STANDARD_WIN,      // normal defeat all enemies condition
		CONST_VALUE,        // condition that always evaluates to "value" (0 = false, 1 = true)

		//map format version 1.0+
		HAVE_0,
		HAVE_BUILDING_0,
		DESTROY_0
	};

	EventCondition(EWinLoseType condition = STANDARD_WIN);
	EventCondition(EWinLoseType condition, si32 value, si32 objectType, const int3 & position = int3(-1, -1, -1));

	const CGObjectInstance * object; // object that was at specified position or with instance name on start
	EMetaclass metaType;
	si32 value;
	si32 objectType;
	si32 objectSubtype;
	std::string objectInstanceName;
	int3 position;
	EWinLoseType condition;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & object;
		h & value;
		h & objectType;
		h & position;
		h & condition;
		h & objectSubtype;
		h & objectInstanceName;
		h & metaType;
	}
};

typedef LogicalExpression<EventCondition> EventExpression;

struct DLL_LINKAGE EventEffect
{
	enum EType
	{
		VICTORY,
		DEFEAT
	};

	/// effect type, using EType enum
	si8 type;

	/// message that will be sent to other players
	std::string toOtherMessage;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & type;
		h & toOtherMessage;
	}
};

struct DLL_LINKAGE TriggeredEvent
{
	/// base condition that must be evaluated
	EventExpression trigger;

	/// string identifier read from config file (e.g. captureKreelah)
	std::string identifier;

	/// string-description, for use in UI (capture town to win)
	std::string description;

	/// Message that will be displayed when this event is triggered (You captured town. You won!)
	std::string onFulfill;

	/// Effect of this event. TODO: refactor into something more flexible
	EventEffect effect;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & identifier;
		h & trigger;
		h & description;
		h & onFulfill;
		h & effect;
	}
};

/// The rumor struct consists of a rumor name and text.
struct DLL_LINKAGE Rumor
{
	std::string name;
	std::string text;

	Rumor() = default;
	~Rumor() = default;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & name;
		h & text;
	}

	void serializeJson(JsonSerializeFormat & handler);
};

/// The disposed hero struct describes which hero can be hired from which player.
struct DLL_LINKAGE DisposedHero
{
	DisposedHero();

	ui32 heroId;
	ui16 portrait; /// The portrait id of the hero, 0xFF is default.
	std::string name;
	ui8 players; /// Who can hire this hero (bitfield).

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & heroId;
		h & portrait;
		h & name;
		h & players;
	}
};

namespace EMapFormat
{
enum EMapFormat: ui8
{
	INVALID = 0,
	//    HEX     DEC
	ROE = 0x0e, // 14
	AB  = 0x15, // 21
	SOD = 0x1c, // 28
// HOTA = 0x1e ... 0x20 // 28 ... 30
	WOG = 0x33,  // 51
	VCMI = 0xF0
};
}

// Inherit from container to enable forward declaration
class ModCompatibilityInfo: public std::map<TModID, CModInfo::Version>
{};

/// The map header holds information about loss/victory condition,map format, version, players, height, width,...
class DLL_LINKAGE CMapHeader
{
	void setupEvents();
public:

	static const int MAP_SIZE_SMALL = 36;
	static const int MAP_SIZE_MIDDLE = 72;
	static const int MAP_SIZE_LARGE = 108;
	static const int MAP_SIZE_XLARGE = 144;
	static const int MAP_SIZE_HUGE = 180;
	static const int MAP_SIZE_XHUGE = 216;
	static const int MAP_SIZE_GIANT = 252;

	CMapHeader();
	virtual ~CMapHeader() = default;

	ui8 levels() const;

	EMapFormat::EMapFormat version; /// The default value is EMapFormat::SOD.
	ModCompatibilityInfo mods; /// set of mods required to play a map
	
	si32 height; /// The default value is 72.
	si32 width; /// The default value is 72.
	bool twoLevel; /// The default value is true.
	std::string name;
	std::string description;
	ui8 difficulty; /// The default value is 1 representing a normal map difficulty.
	/// Specifies the maximum level to reach for a hero. A value of 0 states that there is no
	///	maximum level for heroes. This is the default value.
	ui8 levelLimit;

	std::string victoryMessage;
	std::string defeatMessage;
	ui16 victoryIconIndex;
	ui16 defeatIconIndex;

	std::vector<PlayerInfo> players; /// The default size of the vector is PlayerColor::PLAYER_LIMIT.
	ui8 howManyTeams;
	std::vector<bool> allowedHeroes;

	bool areAnyPlayers; /// Unused. True if there are any playable players on the map.

	/// "main quests" of the map that describe victory and loss conditions
	std::vector<TriggeredEvent> triggeredEvents;

	template <typename Handler>
	void serialize(Handler & h, const int Version)
	{
		h & version;
		if(Version >= 821)
			h & mods;
		h & name;
		h & description;
		h & width;
		h & height;
		h & twoLevel;
		h & difficulty;
		h & levelLimit;
		h & areAnyPlayers;
		h & players;
		h & howManyTeams;
		h & allowedHeroes;
		//Do not serialize triggeredEvents in header as they can contain information about heroes and armies
		h & victoryMessage;
		h & victoryIconIndex;
		h & defeatMessage;
		h & defeatIconIndex;
	}
};

/// The map contains the map header, the tiles of the terrain, objects, heroes, towns, rumors...
class DLL_LINKAGE CMap : public CMapHeader
{
public:
	CMap();
	~CMap();
	void initTerrain();

	CMapEditManager * getEditManager();
	TerrainTile & getTile(const int3 & tile);
	const TerrainTile & getTile(const int3 & tile) const;
	bool isCoastalTile(const int3 & pos) const;
	bool isInTheMap(const int3 & pos) const;
	bool isWaterTile(const int3 & pos) const;

	bool canMoveBetween(const int3 &src, const int3 &dst) const;
	bool checkForVisitableDir(const int3 & src, const TerrainTile * pom, const int3 & dst) const;
	int3 guardingCreaturePosition (int3 pos) const;

	void addBlockVisTiles(CGObjectInstance * obj);
	void removeBlockVisTiles(CGObjectInstance * obj, bool total = false);
	void calculateGuardingGreaturePositions();

	void addNewArtifactInstance(CArtifactInstance * art);
	void eraseArtifactInstance(CArtifactInstance * art);

	void addNewQuestInstance(CQuest * quest);
	void removeQuestInstance(CQuest * quest);

	void setUniqueInstanceName(CGObjectInstance * obj);
	///Use only this method when creating new map object instances
	void addNewObject(CGObjectInstance * obj);
	void moveObject(CGObjectInstance * obj, const int3 & dst);
	void removeObject(CGObjectInstance * obj);


	/// Gets object of specified type on requested position
	const CGObjectInstance * getObjectiveObjectFrom(const int3 & pos, Obj::EObj type);
	CGHeroInstance * getHero(int heroId);

	/// Sets the victory/loss condition objectives ??
	void checkForObjectives();

	void resetStaticData();

	ui32 checksum;
	std::vector<Rumor> rumors;
	std::vector<DisposedHero> disposedHeroes;
	std::vector<ConstTransitivePtr<CGHeroInstance> > predefinedHeroes;
	std::vector<bool> allowedSpell;
	std::vector<bool> allowedArtifact;
	std::vector<bool> allowedAbilities;
	std::list<CMapEvent> events;
	int3 grailPos;
	int grailRadius;

	//Central lists of items in game. Position of item in the vectors below is their (instance) id.
	std::vector< ConstTransitivePtr<CGObjectInstance> > objects;
	std::vector< ConstTransitivePtr<CGTownInstance> > towns;
	std::vector< ConstTransitivePtr<CArtifactInstance> > artInstances;
	std::vector< ConstTransitivePtr<CQuest> > quests;
	std::vector< ConstTransitivePtr<CGHeroInstance> > allHeroes; //indexed by [hero_type_id]; on map, disposed, prisons, etc.

	//Helper lists
	std::vector< ConstTransitivePtr<CGHeroInstance> > heroesOnMap;
	std::map<TeleportChannelID, std::shared_ptr<TeleportChannel> > teleportChannels;

	/// associative list to identify which hero/creature id belongs to which object id(index for objects)
	std::map<si32, ObjectInstanceID> questIdentifierToId;

	std::unique_ptr<CMapEditManager> editManager;

	int3 ***guardingCreaturePositions;

	std::map<std::string, ConstTransitivePtr<CGObjectInstance> > instanceNames;

private:
	/// a 3-dimensional array of terrain tiles, access is as follows: x, y, level. where level=1 is underground
	TerrainTile*** terrain;
	si32 uidCounter; //TODO: initialize when loading an old map

public:
	template <typename Handler>
	void serialize(Handler &h, const int formatVersion)
	{
		h & static_cast<CMapHeader&>(*this);
		h & triggeredEvents; //from CMapHeader
		h & rumors;
		h & allowedSpell;
		h & allowedAbilities;
		h & allowedArtifact;
		h & events;
		h & grailPos;
		h & artInstances;
		h & quests;
		h & allHeroes;
		h & questIdentifierToId;

		//TODO: viccondetails
		const int level = levels();
		if(h.saving)
		{
			// Save terrain
			for(int z = 0; z < level; ++z)
			{
				for(int x = 0; x < width; ++x)
				{
					for(int y = 0; y < height; ++y)
					{
						h & terrain[z][x][y];
						h & guardingCreaturePositions[z][x][y];
					}
				}
			}
		}
		else
		{
			// Load terrain
			terrain = new TerrainTile**[level];
			guardingCreaturePositions = new int3**[level];
			for(int z = 0; z < level; ++z)
			{
				terrain[z] = new TerrainTile*[width];
				guardingCreaturePositions[z] = new int3*[width];
				for(int x = 0; x < width; ++x)
				{
					terrain[z][x] = new TerrainTile[height];
					guardingCreaturePositions[z][x] = new int3[height];
				}
			}
			for(int z = 0; z < level; ++z)
			{
				for(int x = 0; x < width; ++x)
				{
					for(int y = 0; y < height; ++y)
					{

						h & terrain[z][x][y];
						h & guardingCreaturePositions[z][x][y];
					}
				}
			}
		}

		h & objects;
		h & heroesOnMap;
		h & teleportChannels;
		h & towns;
		h & artInstances;

		// static members
		h & CGKeys::playerKeyMap;
		h & CGMagi::eyelist;
		h & CGObelisk::obeliskCount;
		h & CGObelisk::visited;
		h & CGTownInstance::merchantArtifacts;
		h & CGTownInstance::universitySkills;

		h & instanceNames;
	}
};

VCMI_LIB_NAMESPACE_END
