
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
#include "CMapDefines.h"

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
		h & heroId & heroName;
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

	std::set<TFaction> allowedFactions;
	bool isFactionRandom;

	si32 mainCustomHeroPortrait; /// The default value is -1.
	std::string mainCustomHeroName;
	si32 mainCustomHeroId; /// ID of custom hero (only if portrait and hero name are set, otherwise unpredicted value), -1 if none (not always -1)

	std::vector<SHeroName> heroesNames; /// list of placed heroes on the map
	bool hasMainTown; /// The default value is false.
	bool generateHeroAtMainTown; /// The default value is false.
	int3 posOfMainTown;
	TeamID team; /// The default value NO_TEAM
	bool hasRandomHero; /// Player has a random hero

	bool generateHero; /// Unused.
	si32 p7; /// Unknown and unused.
	/// Unused. Count of hero placeholders containing hero type.
	/// WARNING: powerPlaceholders sometimes gives false 0 (eg. even if there is one placeholder), maybe different meaning ???
	ui8 powerPlaceholders;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & p7 & hasRandomHero & mainCustomHeroId & canHumanPlay & canComputerPlay & aiTactic & allowedFactions & isFactionRandom &
				mainCustomHeroPortrait & mainCustomHeroName & heroesNames & hasMainTown & generateHeroAtMainTown &
				posOfMainTown & team & generateHero;
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
	EventCondition(EWinLoseType condition, si32 value, si32 objectType, int3 position = int3(-1, -1, -1));

	const CGObjectInstance * object; // object that was at specified position or with instance name on start
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
		if(version > 759)
		{
			h & objectSubtype;
			h & objectInstanceName;
		}
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
		h & type & toOtherMessage;
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
		h & onFulfill & effect;
	}
};

/// The rumor struct consists of a rumor name and text.
struct DLL_LINKAGE Rumor
{
	std::string name;
	std::string text;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & name & text;
	}
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
		h & heroId & portrait & name & players;
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

/// The map header holds information about loss/victory condition,map format, version, players, height, width,...
class DLL_LINKAGE CMapHeader
{
	void setupEvents();
public:
	static const int MAP_SIZE_SMALL;
	static const int MAP_SIZE_MIDDLE;
	static const int MAP_SIZE_LARGE;
	static const int MAP_SIZE_XLARGE;

	CMapHeader();
	virtual ~CMapHeader();

	EMapFormat::EMapFormat version; /// The default value is EMapFormat::SOD.
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
		h & version & name & description & width & height & twoLevel & difficulty & levelLimit & areAnyPlayers;
		h & players & howManyTeams & allowedHeroes & triggeredEvents;
		h & victoryMessage & victoryIconIndex & defeatMessage & defeatIconIndex;
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

	bool checkForVisitableDir( const int3 & src, const TerrainTile *pom, const int3 & dst ) const;
	int3 guardingCreaturePosition (int3 pos) const;

	void addBlockVisTiles(CGObjectInstance * obj);
	void removeBlockVisTiles(CGObjectInstance * obj, bool total = false);
	void calculateGuardingGreaturePositions();

	void addNewArtifactInstance(CArtifactInstance * art);
	void eraseArtifactInstance(CArtifactInstance * art);

	void addNewObject(CGObjectInstance * obj);

	/// Gets object of specified type on requested position
	const CGObjectInstance * getObjectiveObjectFrom(int3 pos, Obj::EObj type);
	CGHeroInstance * getHero(int heroId);

	/// Sets the victory/loss condition objectives ??
	void checkForObjectives();

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

public:
	template <typename Handler>
	void serialize(Handler &h, const int formatVersion)
	{
		h & static_cast<CMapHeader&>(*this);
		h & rumors & allowedSpell & allowedAbilities & allowedArtifact & events & grailPos;
		h & artInstances & quests & allHeroes;
		h & questIdentifierToId;

		//TODO: viccondetails
		int level = twoLevel ? 2 : 1;
		if(h.saving)
		{
			// Save terrain
			for(int i = 0; i < width ; ++i)
			{
				for(int j = 0; j < height ; ++j)
				{
					for(int k = 0; k < level; ++k)
					{
						h & terrain[i][j][k];
						h & guardingCreaturePositions[i][j][k];
					}
				}
			}
		}
		else
		{
			// Load terrain
			terrain = new TerrainTile**[width];
			guardingCreaturePositions = new int3**[width];
			for(int i = 0; i < width; ++i)
			{
				terrain[i] = new TerrainTile*[height];
				guardingCreaturePositions[i] = new int3*[height];
				for(int j = 0; j < height; ++j)
				{
					terrain[i][j] = new TerrainTile[level];
					guardingCreaturePositions[i][j] = new int3[level];
				}
			}
			for(int i = 0; i < width ; ++i)
			{
				for(int j = 0; j < height ; ++j)
				{
					for(int k = 0; k < level; ++k)
					{
						h & terrain[i][j][k];
						h & guardingCreaturePositions[i][j][k];
					}
				}
			}
		}

		h & objects;
		h & heroesOnMap & teleportChannels & towns & artInstances;

		// static members
		h & CGKeys::playerKeyMap;
		h & CGMagi::eyelist;
		h & CGObelisk::obeliskCount & CGObelisk::visited;
		h & CGTownInstance::merchantArtifacts;
		h & CGTownInstance::universitySkills;

		if(formatVersion >= 759)
		{
			h & instanceNames;
		}
	}
};
