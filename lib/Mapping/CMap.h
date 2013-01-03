
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

/**
 * The hero name struct consists of the hero id and the hero name.
 */
struct DLL_LINKAGE SHeroName
{
	/**
	 * Default c-tor.
	 */
	SHeroName();

	/** the id of the hero */
	int heroId;

	/** the name of the hero */
	std::string heroName;

	/**
	 * Serialize method.
	 */
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

/**
 * The player info constains data about which factions are allowed, AI tactical settings,
 * the main hero name, where to generate the hero, whether the faction should be selected randomly,...
 */
struct DLL_LINKAGE PlayerInfo
{
	/**
	 * Default constructor.
	 */
	PlayerInfo();

	/**
	 * Gets the default faction id or -1 for a random faction.
	 *
	 * @return the default faction id or -1 for a random faction
	 */
	si8 defaultCastle() const;

	/**
	 * Gets -1 for random hero.
	 *
	 * @return -1 for random hero
	 */
	si8 defaultHero() const;

	/** True if the player can be played by a human. */
	bool canHumanPlay;

	/** True if th player can be played by the computer */
	bool canComputerPlay;

	/** Defines the tactical setting of the AI. The default value is EAiTactic::RANDOM. */
	EAiTactic::EAiTactic aiTactic;

	/** A list of unique IDs of allowed factions. */
	std::set<TFaction> allowedFactions;

	/** Unused. True if the faction should be chosen randomly. */
	bool isFactionRandom;

	/** Specifies the ID of the main hero with chosen portrait. The default value is -1. */
	si32 mainHeroPortrait;

	/** The name of the main hero. */
	std::string mainHeroName;

	/** The list of renamed heroes. */
	std::vector<SHeroName> heroesNames;

	/** True if the player has a main town. The default value is true. */
	bool hasMainTown;

	/** True if the main hero should be generated at the main town. The default value is true. */
	bool generateHeroAtMainTown;

	/** The position of the main town. */
	int3 posOfMainTown;

	/** The team id to which the player belongs to. The default value is 255 representing that the player belongs to no team. */
	ui8 team;

	/** Unused. True if a hero should be generated. */
	bool generateHero;

	/** Unknown and unused. */
	si32 p7;

	/** Player has a (custom?) hero */
	bool hasHero;

	/** ID of custom hero, -1 if none */
	si32 customHeroID;

	/**
	 * Unused. Count of hero placeholders containing hero type.
	 * WARNING: powerPlaceholders sometimes gives false 0 (eg. even if there is one placeholder),
	 * maybe different meaning ???
	 */
	ui8 powerPlaceholders;

	/**
	 * Serialize method.
	 */
	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & p7 & hasHero & customHeroID & canHumanPlay & canComputerPlay & aiTactic & allowedFactions & isFactionRandom &
			mainHeroPortrait & mainHeroName & heroesNames & hasMainTown & generateHeroAtMainTown &
			posOfMainTown & team & generateHero;
	}
};

/**
 * The loss condition describes the condition to lose the game. (e.g. lose all own heroes/castles)
 */
struct DLL_LINKAGE LossCondition
{
	/**
	 * Default constructor.
	 */
	LossCondition();

	/** specifies the condition type */
	ELossConditionType::ELossConditionType typeOfLossCon;

	/** the position of an object which mustn't be lost */
	int3 pos;

	/** time limit in days, -1 if not used */
	si32 timeLimit;

	/** set during map parsing: hero/town (depending on typeOfLossCon); nullptr if not used */
	const CGObjectInstance * obj;

	/**
	 * Serialize method.
	 */
	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & typeOfLossCon & pos & timeLimit & obj;
	}
};

/**
 * The victory condition describes the condition to win the game. (e.g. defeat all enemy heroes/castles,
 * receive a specific artifact, ...)
 */
struct DLL_LINKAGE VictoryCondition
{
	/**
	 * Default constructor.
	 */
	VictoryCondition();

	/** specifies the condition type */
	EVictoryConditionType::EVictoryConditionType condition;

	/** true if a normal victory is allowed (defeat all enemy towns, heroes) */
	bool allowNormalVictory;

	/** true if this victory condition also applies to the AI */
	bool appliesToAI;

	/** pos of city to upgrade (3); pos of town to build grail, {-1,-1,-1} if not relevant (4); hero pos (5); town pos(6); monster pos (7); destination pos(8) */
	int3 pos;

	/** artifact ID (0); monster ID (1); resource ID (2); needed fort level in upgraded town (3); artifact ID (8) */
	si32 objectId;

	/** needed count for creatures (1) / resource (2); upgraded town hall level (3);  */
	si32 count;

	/** object of specific monster / city / hero instance (NULL if not used); set during map parsing */
	const CGObjectInstance * obj;

	/**
	 * Serialize method.
	 */
	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & condition & allowNormalVictory & appliesToAI & pos & objectId & count & obj;
	}
};

/**
 * The rumor struct consists of a rumor name and text.
 */
struct DLL_LINKAGE Rumor
{
	/** the name of the rumor */
	std::string name;

	/** the content of the rumor */
	std::string text;

	/**
	 * Serialize method.
	 */
	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & name & text;
	}
};

/**
 * The disposed hero struct describes which hero can be hired from which player.
 */
struct DLL_LINKAGE DisposedHero
{
	/**
	 * Default c-tor.
	 */
	DisposedHero();

	/** the id of the hero */
	ui32 heroId;

	/** the portrait id of the hero, 0xFF is default */
	ui16 portrait;

	/** the name of the hero */
	std::string name;

	/** who can hire this hero (bitfield) */
	ui8 players;

	/**
	 * Serialize method.
	 */
	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & heroId & portrait & name & players;
	}
};

/// Class which manages map events.

/**
 * The map event is an event which gives or takes resources for a specific
 * amount of players and can appear regularly or once a time.
 */
class DLL_LINKAGE CMapEvent
{
public:
	/**
	 * Default c-tor.
	 */
	CMapEvent();

	/**
	 * Returns true if this map event occurs earlier than the other map event for the first time.
	 *
	 * @param other the other map event to compare with
	 * @return true if this event occurs earlier than the other map event, false if not
	 */
	bool earlierThan(const CMapEvent & other) const;

	/**
	 * Returns true if this map event occurs earlier than or at the same day than the other map event for the first time.
	 *
	 * @param other the other map event to compare with
	 * @return true if this event occurs earlier than or at the same day than the other map event, false if not
	 */
	bool earlierThanOrEqual(const CMapEvent & other) const;

	/** the name of the event */
	std::string name;

	/** the message to display */
	std::string message;

	/** gained or taken resources */
	TResources resources;

	/** affected players */
	ui8 players;

	/** affected humans */
	ui8 humanAffected;

	/** affacted computer players */
	ui8 computerAffected;

	/** the day counted continously when the event happens */
	ui32 firstOccurence;

	/** specifies after how many days the event will occur the next time; 0 if event occurs only one time */
	ui32 nextOccurence;

	/**
	 * Serialize method.
	 */
	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & name & message & resources
			& players & humanAffected & computerAffected & firstOccurence & nextOccurence;
	}
};

/**
 * The castle event builds/adds buildings/creatures for a specific town.
 */
class DLL_LINKAGE CCastleEvent: public CMapEvent
{
public:
	/**
	 * Default c-tor.
	 */
	CCastleEvent();

	/** build specific buildings */
	std::set<si32> buildings;

	/** additional creatures in i-th level dwelling */
	std::vector<si32> creatures;

	/** owner of this event */
	CGTownInstance * town;

	/**
	 * Serialize method.
	 */
	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & static_cast<CMapEvent &>(*this);
		h & buildings & creatures;
	}
};

namespace ETerrainType
{
	enum ETerrainType
	{
		BORDER = -1, DIRT, SAND, GRASS, SNOW, SWAMP,
		ROUGH, SUBTERRANEAN, LAVA, WATER, ROCK
	};
}

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

/**
 * The terrain tile describes the terrain type and the visual representation of the terrain.
 * Furthermore the struct defines whether the tile is visitable or/and blocked and which objects reside in it.
 */
struct DLL_LINKAGE TerrainTile
{
	/**
	 * Default c-tor.
	 */
	TerrainTile();

	/**
	 * Gets true if the terrain is not a rock. If from is water/land, same type is also required.
	 *
	 * @param from
	 * @return
	 */
	bool entrableTerrain(const TerrainTile * from = NULL) const;

	/**
	 * Gets true if the terrain is not a rock. If from is water/land, same type is also required.
	 *
	 * @param allowLand
	 * @param allowSea
	 * @return
	 */
	bool entrableTerrain(bool allowLand, bool allowSea) const;

	/**
	 * Checks for blocking objects and terraint type (water / land).
	 *
	 * @param from
	 * @return
	 */
	bool isClear(const TerrainTile * from = NULL) const;

	/**
	 * Gets the ID of the top visitable object or -1 if there is none.
	 *
	 * @return the ID of the top visitable object or -1 if there is none
	 */
	int topVisitableId() const;

	/**
	 * Gets true if the terrain type is water.
	 *
	 * @return true if the terrain type is water
	 */
	bool isWater() const;

	/**
	 * Gets true if the terrain tile is coastal.
	 *
	 * @return true if the terrain tile is coastal
	 */
	bool isCoastal() const;

	/**
	 * Gets true if the terrain tile has favourable winds.
	 *
	 * @return true if the terrain tile has favourable winds
	 */
	bool hasFavourableWinds() const;

	/** the type of terrain */
	ETerrainType::ETerrainType terType;

	/** the visual representation of the terrain */
	ui8 terView;

	/** the type of the river. 0 if there is no river */
	ERiverType::ERiverType riverType;

	/** the direction of the river */
	ui8 riverDir;

	/** the type of the road. 0 if there is no river */
	ERoadType::ERoadType roadType;

	/** the direction of the road */
	ui8 roadDir;

	/**
	 * first two bits - how to rotate terrain graphic (next two - river graphic, next two - road);
	 * 7th bit - whether tile is coastal (allows disembarking if land or block movement if water); 8th bit - Favourable Winds effect
	 */
	ui8 extTileFlags;

	/** true if it is visitable, false if not */
	bool visitable;

	/** true if it is blocked, false if not */
	bool blocked;

	/** pointers to objects which the hero can visit while being on this tile */
	std::vector<CGObjectInstance *> visitableObjects;

	/** pointers to objects that are blocking this tile */
	std::vector<CGObjectInstance *> blockingObjects;

	/**
	 * Serialize method.
	 */
	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & terType & terView & riverType & riverDir & roadType &roadDir & extTileFlags & blocked;

		if(!h.saving)
		{
			visitable = false;
			//these flags (and obj vectors) will be restored in map serialization
		}
	}
};

namespace EMapFormat
{
	enum EMapFormat
	{
		INVALID, WOG=0x33, AB=0x15, ROE=0x0e, SOD=0x1c
	};
}

/**
 * The map header holds information about loss/victory condition,
 * map format, version, players, height, width,...
 */
class DLL_LINKAGE CMapHeader
{
public:
	/**
	 * Default constructor.
	 */
	CMapHeader();

	/**
	 * D-tor.
	 */
	virtual ~CMapHeader();

	/** The version of the map. The default value is EMapFormat::SOD. */
	EMapFormat::EMapFormat version;

	/** The height of the map. The default value is 72. */
	si32 height;

	/** The width of the map. The default value is 72. */
	si32 width;

	/** Specifies if the map has two levels. The default value is true. */
	bool twoLevel;

	/** The name of the map. */
	std::string name;

	/** The description of the map. */
	std::string description;

	/**
	 * Specifies the difficulty of the map ranging from 0 easy to 4 impossible.
	 * The default value is 1 representing a normal map difficulty.
	 */
	ui8 difficulty;

	/**
	 * Specifies the maximum level to reach for a hero. A value of 0 states that there is no
	 * maximum level for heroes.
	 */
	ui8 levelLimit;

	/** Specifies the loss condition. The default value is lose all your towns and heroes. */
	LossCondition lossCondition;

	/** Specifies the victory condition. The default value is defeat all enemies. */
	VictoryCondition victoryCondition;

	/** A list containing information about players. */
	std::vector<PlayerInfo> players;

	/** The number of teams. */
	ui8 howManyTeams;

	/**
	 * A list of allowed heroes. The index is the hero id and the value is either 0 for not allowed or 1 for allowed.
	 * The default value is a list of default allowed heroes. See CHeroHandler::getDefaultAllowedHeroes for more info.
	 */
	std::vector<ui8> allowedHeroes;

	/** A list of placeholded heroes. The index is the id of a hero type. */
	std::vector<ui16> placeholdedHeroes;

	/** Unused. True if there are any playable players on the map. */
	bool areAnyPlayers;

	/**
	 * Serialize method.
	 */
	template <typename Handler>
	void serialize(Handler & h, const int Version)
	{
		h & version & name & description & width & height & twoLevel & difficulty & levelLimit & areAnyPlayers;
		h & players & lossCondition & victoryCondition & howManyTeams & allowedHeroes;
	}
};

/**
 * The map contains the map header, the tiles of the terrain, objects,
 * heroes, towns, rumors...
 */
class DLL_LINKAGE CMap : public CMapHeader
{
public:
	/**
	 * Default constructor.
	 */
	CMap();

	/**
	 * Destructor.
	 */
	~CMap();

	/**
	 * Erases an artifact instance.
	 *
	 * @param art the artifact to erase
	 */
	void eraseArtifactInstance(CArtifactInstance * art);

	/**
	 * Gets the topmost object or the lowermost object depending on the flag
	 * lookForHero from the specified position.
	 *
	 * @param pos the position of the tile
	 * @param lookForHero true if you want to get the lowermost object, false if
	 *        you want to get the topmost object
	 * @return the object at the given position and level
	 */
	const CGObjectInstance * getObjectiveObjectFrom(int3 pos, bool lookForHero);

	/**
	 * Sets the victory/loss condition objectives.
	 */
	void checkForObjectives();

	/**
	 * Adds an visitable/blocking object to a terrain tile.
	 *
	 * @param obj the visitable/blocking object to add to a tile
	 */
	void addBlockVisTiles(CGObjectInstance * obj);

	/**
	 * Removes an visitable/blocking object from a terrain tile.
	 *
	 * @param obj the visitable/blocking object to remove from a tile
	 * @param total
	 */
	void removeBlockVisTiles(CGObjectInstance * obj, bool total = false);

	/**
	 * Gets the terrain tile of the specified position.
	 *
	 * @param tile the position of the tile
	 * @return the terrain tile of the specified position
	 */
	TerrainTile & getTile(const int3 & tile);

	/**
	 * Gets the terrain tile as a const of the specified position.
	 *
	 * @param tile the position of the tile
	 * @return the terrain tile as a const of the specified position
	 */
	const TerrainTile & getTile(const int3 & tile) const;

	/**
	 * Gets the hero with the given id.
	 * @param heroId the hero id
	 * @return the hero with the given id
	 */
	CGHeroInstance * getHero(int heroId);

	/**
	 * Validates if the position is in the bounds of the map.
	 *
	 * @param pos the position to test
	 * @return true if the position is in the bounds of the map
	 */
	bool isInTheMap(const int3 & pos) const;

	/**
	 * Validates if the tile at the given position is a water terrain type.
	 *
	 * @param pos the position to test
	 * @return true if the tile at the given position is a water terrain type
	 */
	bool isWaterTile(const int3 & pos) const;

	/**
	 * Adds the specified artifact instance to the list of artifacts of this map.
	 *
	 * @param art the artifact which should be added to the list of artifacts
	 */
	void addNewArtifactInstance(CArtifactInstance * art);

	/** the checksum of the map */
	ui32 checksum;

	/** a 3-dimensional array of terrain tiles, access is as follows: x, y, level */
	TerrainTile*** terrain;

	/** list of rumors */
	std::vector<Rumor> rumors;

	/** list of disposed heroes */
	std::vector<DisposedHero> disposedHeroes;

	/** list of predefined heroes */
	std::vector<ConstTransitivePtr<CGHeroInstance> > predefinedHeroes;

	/** list of .def files with definitions from .h3m (may be custom) */
	std::vector<ConstTransitivePtr<CGDefInfo> > customDefs;

	/** list of allowed spells, index is the spell id */
	std::vector<ui8> allowedSpell;

	/** list of allowed artifacts, index is the artifact id */
	std::vector<ui8> allowedArtifact;

	/** list of allowed abilities, index is the ability id */
	std::vector<ui8> allowedAbilities;

	/** list of map events */
	std::list<ConstTransitivePtr<CMapEvent> > events;

	/** specifies the position of the grail */
	int3 grailPos;

	/** specifies the radius of the grail */
	int grailRadious;

	/** list of objects */
	std::vector< ConstTransitivePtr<CGObjectInstance> > objects;

	/** list of heroes */
	std::vector< ConstTransitivePtr<CGHeroInstance> > heroes;

	/** list of towns */
	std::vector< ConstTransitivePtr<CGTownInstance> > towns;

	/** list of artifacts */
	std::vector< ConstTransitivePtr<CArtifactInstance> > artInstances;

	/** list of quests */
	std::vector< ConstTransitivePtr<CQuest> > quests;

	/** associative list to identify which hero/creature id belongs to which object id(index for objects) */
	bmap<si32, si32> questIdentifierToId;

	/**
	 * Serialize method.
	 */
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

		// static members
		h & CGTeleport::objs;
		h & CGTeleport::gates;
		h & CGKeys::playerKeyMap;
		h & CGMagi::eyelist;
		h & CGObelisk::obeliskCount & CGObelisk::visited;
		h & CGTownInstance::merchantArtifacts;
		h & CGTownInstance::universitySkills;

		if(!h.saving)
		{
			for(ui32 i = 0; i < objects.size(); ++i)
			{
				if(!objects[i]) continue;

				switch (objects[i]->ID)
				{
					case Obj::HERO:
						heroes.push_back (static_cast<CGHeroInstance*>(+objects[i]));
						break;
					case Obj::TOWN:
						towns.push_back (static_cast<CGTownInstance*>(+objects[i]));
						break;
				}

				// recreate blockvis map
				addBlockVisTiles(objects[i]);
			}

			// if hero is visiting/garrisoned in town set appropriate pointers
			for(ui32 i = 0; i < heroes.size(); ++i)
			{
				int3 vistile = heroes[i]->pos;
				vistile.x++;
				for(ui32 j = 0; j < towns.size(); ++j)
				{
					// hero stands on the town entrance
					if(vistile == towns[j]->pos)
					{
						if(heroes[i]->inTownGarrison)
						{
							towns[j]->garrisonHero = heroes[i];
							removeBlockVisTiles(heroes[i]);
						}
						else
						{
							towns[j]->visitingHero = heroes[i];
						}

						heroes[i]->visitedTown = towns[j];
						break;
					}
				}

				vistile.x -= 2; //manifest pos
				const TerrainTile & t = getTile(vistile);
				if(t.terType != ETerrainType::WATER) continue;

				//hero stands on the water - he must be in the boat
				for(ui32 j = 0; j < t.visitableObjects.size(); ++j)
				{
					if(t.visitableObjects[j]->ID == Obj::BOAT)
					{
						CGBoat * b = static_cast<CGBoat *>(t.visitableObjects[j]);
						heroes[i]->boat = b;
						b->hero = heroes[i];
						removeBlockVisTiles(b);
						break;
					}
				}
			}
		}
	}
};
