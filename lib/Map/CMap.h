
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

#ifndef _MSC_VER
#include "../CObjectHandler.h"
#include "../CDefObjInfoHandler.h"
#endif

#include "../ConstTransitivePtr.h"
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
        NO_RIVER = 0, CLEAR_RIVER, ICY_RIVER, MUDDY_RIVER, LAVA_RIVER
    };
}

namespace ERoadType
{
    enum ERoadType
    {
        DIRT_ROAD = 1, GRAVEL_ROAD, COBBLESTONE_ROAD
    };
}

/**
 * The terrain tile describes the terrain type and the visual representation of the terrain.
 * Furthermore the struct defines whether the tile is visitable or/and blocked and which objects reside in it.
 */
struct DLL_LINKAGE TerrainTile
{
    /** the type of terrain */
    ETerrainType::ETerrainType tertype;

    /** the visual representation of the terrain */
    ui8 terview;

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
    std::vector <CGObjectInstance *> visitableObjects;

    /** pointers to objects that are blocking this tile */
    std::vector <CGObjectInstance *> blockingObjects;

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
    int topVisitableID() const;

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

    /**
     * Serialize method.
     */
    template <typename Handler>
    void serialize(Handler & h, const int version)
    {
        h & tertype & terview & riverType & riverDir & roadType &roadDir & extTileFlags & blocked;

        if(!h.saving)
        {
            visitable = false;
            //these flags (and obj vectors) will be restored in map serialization
        }
    }
};

/**
 * The hero name struct consists of the hero id and name.
 */
struct DLL_LINKAGE SheroName
{
    /** the id of the hero */
    int heroID;

    /** the name of the hero */
    std::string heroName;

    /**
     * Serialize method.
     */
    template <typename Handler>
    void serialize(Handler & h, const int version)
    {
        h & heroID & heroName;
    }
};

/**
 * The player info constains data about which factions are allowed, AI tactical settings,
 * main hero name, where to generate the hero, whether faction is random,...
 */
struct DLL_LINKAGE PlayerInfo
{
    /** unknown, unused */
    si32 p7;

    /** TODO ? */
    si32 p8;

    /** TODO ? */
    si32 p9;

    /** TODO unused, q-ty of hero placeholders containing hero type, WARNING: powerPlacehodlers sometimes gives false 0 (eg. even if there is one placeholder), maybe different meaning??? */
    ui8 powerPlacehodlers;

    /** player can be played by a human */
    ui8 canHumanPlay;

    /** player can be played by the computer */
    ui8 canComputerPlay;

    /** defines the tactical setting of the AI: 0 - random, 1 -  warrior, 2 - builder, 3 - explorer */
    ui32 AITactic;

    /** IDs of allowed factions */
    std::set<ui32> allowedFactions;

    /** unused. is the faction random */
    ui8 isFactionRandom;

    /** specifies the ID of hero with chosen portrait; 255 if standard */
    ui32 mainHeroPortrait;

    /** the name of the main hero */
    std::string mainHeroName;

    /** list of available heroes */
    std::vector<SheroName> heroesNames;

    /** has the player a main town */
    ui8 hasMainTown;

    /** generates the hero at the main town */
    ui8 generateHeroAtMainTown;

    /** the position of the main town */
    int3 posOfMainTown;

    /** the team id to which the player belongs to */
    ui8 team;

    /** unused. generates a hero */
    ui8 generateHero;

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

    /**
     * Serialize method.
     */
    template <typename Handler>
    void serialize(Handler & h, const int version)
    {
        h & p7 & p8 & p9 & canHumanPlay & canComputerPlay & AITactic & allowedFactions & isFactionRandom &
            mainHeroPortrait & mainHeroName & heroesNames & hasMainTown & generateHeroAtMainTown &
            posOfMainTown & team & generateHero;
    }
};

/**
 * The loss condition describes the condition to lose the game. (e.g. lose all own heroes/castles)
 */
struct DLL_LINKAGE LossCondition
{
    /** specifies the condition type */
    ELossConditionType::ELossConditionType typeOfLossCon;

    /** the position of an object which mustn't be lost */
    int3 pos;

    /** time limit in days, -1 if not used */
    si32 timeLimit;

    /** set during map parsing: hero/town (depending on typeOfLossCon); nullptr if not used */
    const CGObjectInstance * obj;

    /**
     * Default constructor.
     */
    LossCondition();

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
    /** specifies the condition type */
    EVictoryConditionType::EVictoryConditionType condition;

    /** true if a normal victory is allowed (defeat all enemy towns, heroes) */
    ui8 allowNormalVictory;

    /** true if this victory condition also applies to the AI */
    ui8 appliesToAI;

    /** pos of city to upgrade (3); pos of town to build grail, {-1,-1,-1} if not relevant (4); hero pos (5); town pos(6); monster pos (7); destination pos(8) */
    int3 pos;

    /** artifact ID (0); monster ID (1); resource ID (2); needed fort level in upgraded town (3); artifact ID (8) */
    si32 ID;

    /** needed count for creatures (1) / resource (2); upgraded town hall level (3);  */
    si32 count;

    /** object of specific monster / city / hero instance (NULL if not used); set during map parsing */
    const CGObjectInstance * obj;

    /**
     * Default constructor.
     */
    VictoryCondition();

    /**
     * Serialize method.
     */
    template <typename Handler>
    void serialize(Handler & h, const int version)
    {
        h & condition & allowNormalVictory & appliesToAI & pos & ID & count & obj;
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
    /** the id of the hero */
    ui32 ID;

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
        h & ID & portrait & name & players;
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

    /** the day counted continously where the event happens */
    ui32 firstOccurence;

    /** specifies after how many days the event will occur the next time; 0 if event occurs only one time */
    ui32 nextOccurence;

    bool operator<(const CMapEvent &b) const
    {
        return firstOccurence < b.firstOccurence;
    }
    bool operator<=(const CMapEvent &b) const
    {
        return firstOccurence <= b.firstOccurence;
    }

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

    /** the version of the map */
    EMapFormat::EMapFormat version;

    /** if there are any playable players on the map */
    ui8 areAnyPLayers;

    /** the height of the map */
    si32 height;

    /** the width of the map */
    si32 width;

    /** specifies if the map has two levels */
    si32 twoLevel;

    /** the name of the map */
    std::string name;

    /** the description of the map */
    std::string description;

    /** specifies the difficulty of the map ranging from 0 easy to 4 impossible */
    ui8 difficulty;

    /** specifies the maximum level to reach for a hero */
    ui8 levelLimit;

    /** the loss condition */
    LossCondition lossCondition;

    /** the victory condition */
    VictoryCondition victoryCondition;

    /** list of player information */
    std::vector<PlayerInfo> players;

    /** number of teams */
    ui8 howManyTeams;

    /** list of allowed heroes, index is hero id */
    std::vector<ui8> allowedHeroes;

    /** list of placeholded heroes, index is id of heroes types */
    std::vector<ui16> placeholdedHeroes;

    /**
     * Serialize method.
     */
    template <typename Handler>
    void serialize(Handler & h, const int Version)
    {
        h & version & name & description & width & height & twoLevel & difficulty & levelLimit & areAnyPLayers;
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
     * @param heroID the hero id
     * @return the hero with the given id
     */
    CGHeroInstance * getHero(int heroID);

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
                    for(int k = 0; k <= twoLevel; ++k)
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
                    terrain[ii][jj] = new TerrainTile[twoLevel + 1];
                }
            }
            for(int i = 0; i < width ; ++i)
            {
                for(int j = 0; j < height ; ++j)
                {
                    for(int k = 0; k <= twoLevel ; ++k)
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
                if(t.tertype != ETerrainType::WATER) continue;

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
