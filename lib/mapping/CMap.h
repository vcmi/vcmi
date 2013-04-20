
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
#include "../CObjectHandler.h"
#include "../ResourceSet.h"
#include "../int3.h"
#include "../GameConstants.h"

class CArtifactInstance;
class CGDefInfo;
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

namespace EAiTactic
{
enum EAiTactic
{
	NONE = -1,
	RANDOM,
	WARRIOR,
	BUILDER,
	EXPLORER
};
}

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

	bool canHumanPlay;
	bool canComputerPlay;
	EAiTactic::EAiTactic aiTactic; /// The default value is EAiTactic::RANDOM.
	std::set<TFaction> allowedFactions;
	bool isFactionRandom;
	si32 mainHeroPortrait; /// The default value is -1.
	std::string mainHeroName;
	std::vector<SHeroName> heroesNames; /// List of renamed heroes.
	bool hasMainTown; /// The default value is false.
	bool generateHeroAtMainTown; /// The default value is false.
	int3 posOfMainTown;
	TeamID team; /// The default value is 255 representing that the player belongs to no team.

	bool generateHero; /// Unused.
	si32 p7; /// Unknown and unused.
	bool hasHero; /// Player has a (custom?) hero
	si32 customHeroID; /// ID of custom hero, -1 if none
	/// Unused. Count of hero placeholders containing hero type.
	/// WARNING: powerPlaceholders sometimes gives false 0 (eg. even if there is one placeholder), maybe different meaning ???
	ui8 powerPlaceholders;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & p7 & hasHero & customHeroID & canHumanPlay & canComputerPlay & aiTactic & allowedFactions & isFactionRandom &
				mainHeroPortrait & mainHeroName & heroesNames & hasMainTown & generateHeroAtMainTown &
				posOfMainTown & team & generateHero;
	}
};

/// The loss condition describes the condition to lose the game. (e.g. lose all own heroes/castles)
struct DLL_LINKAGE LossCondition
{
	LossCondition();

	ELossConditionType::ELossConditionType typeOfLossCon;
	int3 pos; /// the position of an object which mustn't be lost
	si32 timeLimit; /// time limit in days, -1 if not used
	const CGObjectInstance * obj;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & typeOfLossCon & pos & timeLimit & obj;
	}
};

/// The victory condition describes the condition to win the game. (e.g. defeat all enemy heroes/castles,
/// receive a specific artifact, ...)
struct DLL_LINKAGE VictoryCondition
{
	VictoryCondition();

	EVictoryConditionType::EVictoryConditionType condition;
	bool allowNormalVictory; /// true if a normal victory is allowed (defeat all enemy towns, heroes)
	bool appliesToAI;
	/// pos of city to upgrade (3); pos of town to build grail, {-1,-1,-1} if not relevant (4); hero pos (5); town pos(6);
	///	monster pos (7); destination pos(8)
	int3 pos;
	/// artifact ID (0); monster ID (1); resource ID (2); needed fort level in upgraded town (3); artifact ID (8)
	si32 objectId;
	/// needed count for creatures (1) / resource (2); upgraded town hall level (3);
	si32 count;
	/// object of specific monster / city / hero instance (NULL if not used); set during map parsing
	const CGObjectInstance * obj;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & condition & allowNormalVictory & appliesToAI & pos & objectId & count & obj;
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

/// The map event is an event which e.g. gives or takes resources of a specific
/// amount to/from players and can appear regularly or once a time.
class DLL_LINKAGE CMapEvent
{
public:
	CMapEvent();

	bool earlierThan(const CMapEvent & other) const;
	bool earlierThanOrEqual(const CMapEvent & other) const;

	std::string name;
	std::string message;
	TResources resources;
	ui8 players; // affected players, bit field?
	ui8 humanAffected;
	ui8 computerAffected;
	ui32 firstOccurence;
	ui32 nextOccurence; /// specifies after how many days the event will occur the next time; 0 if event occurs only one time

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & name & message & resources
				& players & humanAffected & computerAffected & firstOccurence & nextOccurence;
	}
};

/// The castle event builds/adds buildings/creatures for a specific town.
class DLL_LINKAGE CCastleEvent: public CMapEvent
{
public:
	CCastleEvent();

	std::set<BuildingID> buildings;
	std::vector<si32> creatures;
	CGTownInstance * town;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & static_cast<CMapEvent &>(*this);
		h & buildings & creatures;
	}
};

namespace ERiverType
{
enum ERiverType
{
	NO_RIVER, CLEAR_RIVER, ICY_RIVER, MUDDY_RIVER, LAVA_RIVER
};
}

namespace ERoadType
{
enum ERoadType
{
	NO_ROAD, DIRT_ROAD, GRAVEL_ROAD, COBBLESTONE_ROAD
};
}

/// The terrain tile describes the terrain type and the visual representation of the terrain.
/// Furthermore the struct defines whether the tile is visitable or/and blocked and which objects reside in it.
struct DLL_LINKAGE TerrainTile
{
	TerrainTile();

	/// Gets true if the terrain is not a rock. If from is water/land, same type is also required.
	bool entrableTerrain(const TerrainTile * from = NULL) const;
	bool entrableTerrain(bool allowLand, bool allowSea) const;
	/// Checks for blocking objects and terraint type (water / land).
	bool isClear(const TerrainTile * from = NULL) const;
	/// Gets the ID of the top visitable object or -1 if there is none.
	int topVisitableId() const;
	bool isWater() const;
	bool isCoastal() const;
	bool hasFavourableWinds() const;

	ETerrainType terType;
	ui8 terView;
	ERiverType::ERiverType riverType;
	ui8 riverDir;
	ERoadType::ERoadType roadType;
	ui8 roadDir;
	/// first two bits - how to rotate terrain graphic (next two - river graphic, next two - road);
	///	7th bit - whether tile is coastal (allows disembarking if land or block movement if water); 8th bit - Favourable Winds effect
	ui8 extTileFlags;
	bool visitable;
	bool blocked;

	std::vector<CGObjectInstance *> visitableObjects;
	std::vector<CGObjectInstance *> blockingObjects;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & terType & terView & riverType & riverDir & roadType &roadDir & extTileFlags;
		h & visitable & blocked;
		h & visitableObjects & blockingObjects;
	}
};

namespace EMapFormat
{
enum EMapFormat
{
	INVALID, WOG=0x33, AB=0x15, ROE=0x0e, SOD=0x1c
};
}

/// The map header holds information about loss/victory condition,map format, version, players, height, width,...
class DLL_LINKAGE CMapHeader
{
public:
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
	LossCondition lossCondition; /// The default value is lose all your towns and heroes.
	VictoryCondition victoryCondition; /// The default value is defeat all enemies.
	std::vector<PlayerInfo> players; /// The default size of the vector is PlayerColor::PLAYER_LIMIT.
	ui8 howManyTeams;
	std::vector<bool> allowedHeroes;
	std::vector<ui16> placeholdedHeroes;
	bool areAnyPlayers; /// Unused. True if there are any playable players on the map.

	template <typename Handler>
	void serialize(Handler & h, const int Version)
	{
		h & version & name & description & width & height & twoLevel & difficulty & levelLimit & areAnyPlayers;
		h & players & lossCondition & victoryCondition & howManyTeams & allowedHeroes;
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
	bool isInTheMap(const int3 & pos) const;
	bool isWaterTile(const int3 & pos) const;

	void addBlockVisTiles(CGObjectInstance * obj);
	void removeBlockVisTiles(CGObjectInstance * obj, bool total = false);

	void addNewArtifactInstance(CArtifactInstance * art);
	void eraseArtifactInstance(CArtifactInstance * art);
	void addQuest(CGObjectInstance * quest);

	/// Gets the topmost object or the lowermost object depending on the flag lookForHero from the specified position.
	const CGObjectInstance * getObjectiveObjectFrom(int3 pos, bool lookForHero);
	CGHeroInstance * getHero(int heroId);

	/// Sets the victory/loss condition objectives ??
	void checkForObjectives();

	ui32 checksum;
	/// a 3-dimensional array of terrain tiles, access is as follows: x, y, level. where level=1 is underground
	std::vector<Rumor> rumors;
	std::vector<DisposedHero> disposedHeroes;
	std::vector<ConstTransitivePtr<CGHeroInstance> > predefinedHeroes;
	std::vector<ConstTransitivePtr<CGDefInfo> > customDefs;
	std::vector<bool> allowedSpell;
	std::vector<bool> allowedArtifact;
	std::vector<bool> allowedAbilities;
	std::list<CMapEvent> events;
	int3 grailPos;
	int grailRadious;

	std::vector< ConstTransitivePtr<CGObjectInstance> > objects;
	std::vector< ConstTransitivePtr<CGHeroInstance> > heroes;
	std::vector< ConstTransitivePtr<CGTownInstance> > towns;
	std::vector< ConstTransitivePtr<CArtifactInstance> > artInstances;
	std::vector< ConstTransitivePtr<CQuest> > quests;
	/// associative list to identify which hero/creature id belongs to which object id(index for objects)
	bmap<si32, ObjectInstanceID> questIdentifierToId;

	unique_ptr<CMapEditManager> editManager;

private:
	void getTileRangeCheck(const int3 & tile) const;

	TerrainTile*** terrain;

public:
	template <typename Handler>
	void serialize(Handler &h, const int formatVersion)
	{
		h & static_cast<CMapHeader&>(*this);
		h & rumors & allowedSpell & allowedAbilities & allowedArtifact & events & grailPos;
		h & artInstances & quests;
		h & questIdentifierToId;

		//TODO: viccondetails
		if(h.saving)
		{
			// Save terrain
			for(int i = 0; i < width ; ++i)
			{
				for(int j = 0; j < height ; ++j)
				{
					for(int k = 0; k < (twoLevel ? 2 : 1); ++k)
					{
						h & terrain[i][j][k];
					}
				}
			}
		}
		else
		{
			// Load terrain
			terrain = new TerrainTile**[width];
			for(int ii = 0; ii < width; ++ii)
			{
				terrain[ii] = new TerrainTile*[height];
				for(int jj = 0; jj < height; ++jj)
				{
					terrain[ii][jj] = new TerrainTile[twoLevel ? 2 : 1];
				}
			}
			for(int i = 0; i < width ; ++i)
			{
				for(int j = 0; j < height ; ++j)
				{
					for(int k = 0; k < (twoLevel ? 2 : 1); ++k)
					{
						h & terrain[i][j][k];
					}
				}
			}
		}

		h & customDefs & objects;
		h & heroes & towns & artInstances;

		// static members
		h & CGTeleport::objs;
		h & CGTeleport::gates;
		h & CGKeys::playerKeyMap;
		h & CGMagi::eyelist;
		h & CGObelisk::obeliskCount & CGObelisk::visited;
		h & CGTownInstance::merchantArtifacts;
		h & CGTownInstance::universitySkills;
	}
};
