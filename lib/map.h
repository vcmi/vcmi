#ifndef __MAP_H__
#define __MAP_H__
#ifdef _MSC_VER
#pragma warning (disable : 4482)
#endif
#include <cstring>
#include <vector>
#include <map>
#include <set>
#include <list>
#include "../global.h"
#ifndef _MSC_VER
#include "../hch/CObjectHandler.h"
#include "../hch/CDefObjInfoHandler.h"
#endif

/*
 * map.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CGDefInfo;
class CGObjectInstance;
class CGHeroInstance;
class CGCreature;
class CQuest;
class CGTownInstance;

enum EDefType {TOWN_DEF, HERO_DEF, CREATURES_DEF, SEERHUT_DEF, RESOURCE_DEF, TERRAINOBJ_DEF, 
	EVENTOBJ_DEF, SIGN_DEF, GARRISON_DEF, ARTIFACT_DEF, WITCHHUT_DEF, SCHOLAR_DEF, PLAYERONLY_DEF, 
	SHRINE_DEF, SPELLSCROLL_DEF, PANDORA_DEF, GRAIL_DEF, CREGEN_DEF, CREGEN2_DEF, CREGEN3_DEF, 
	BORDERGUARD_DEF, HEROPLACEHOLDER_DEF};

class DLL_EXPORT CSpecObjInfo
{
public:
	virtual ~CSpecObjInfo(){};
};

class DLL_EXPORT CCreGenObjInfo : public CSpecObjInfo
{
public:
	unsigned char player; //owner
	bool asCastle;
	ui32 identifier;
	unsigned char castles[2]; //allowed castles
};
class DLL_EXPORT CCreGen2ObjInfo : public CSpecObjInfo
{
public:
	unsigned char player; //owner
	bool asCastle;
	ui32 identifier;
	unsigned char castles[2]; //allowed castles
	unsigned char minLevel, maxLevel; //minimal and maximal level of creature in dwelling: <0, 6>
};
class DLL_EXPORT CCreGen3ObjInfo : public CSpecObjInfo
{
public:
	unsigned char player; //owner
	unsigned char minLevel, maxLevel; //minimal and maximal level of creature in dwelling: <0, 6>
};
struct DLL_EXPORT TerrainTile
{
	enum EterrainType {border=-1, dirt, sand, grass, snow, swamp, rough, subterranean, lava, water, rock};
	enum Eriver {noRiver=0, clearRiver, icyRiver, muddyRiver, lavaRiver};
	enum Eroad {dirtRoad=1, grazvelRoad, cobblestoneRoad};

	EterrainType tertype; // type of terrain
	unsigned char terview; // look of terrain
	Eriver nuine; // type of Eriver (0 if there is no river)
	unsigned char rivDir; // direction of Eriver
	Eroad malle; // type of Eroad (0 if there is no river)
	unsigned char roadDir; // direction of Eroad
	unsigned char siodmyTajemniczyBajt; //first two bits - how to rotate terrain graphic (next two - river graphic, next two - road); 7th bit - whether tile is coastal (allows disembarking if land or block movement if water); 8th bit - Favourable Winds effect

	bool visitable; //false = not visitable; true = visitable
	bool blocked; //false = free; true = blocked;

	std::vector <CGObjectInstance*> visitableObjects; //pointers to objects hero can visit while being on this tile
	std::vector <CGObjectInstance*> blockingObjects; //pointers to objects that are blocking this tile

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tertype & terview & nuine & rivDir & malle &roadDir & siodmyTajemniczyBajt;

		if(!h.saving)
		{
			visitable = blocked = false;
			//these flags (and obj vectors) will be restored in map serialization
		}
	}

	bool entrableTerrain(const TerrainTile *from = NULL) const; //checks if terrain is not a rock. If from is water/land, same type is also required. 
	bool entrableTerrain(bool allowLand, bool allowSea) const; //checks if terrain is not a rock. If from is water/land, same type is also required. 
	bool isClear(const TerrainTile *from = NULL) const; //checks for blocking objs and terraint type (water / land)
};
struct DLL_EXPORT SheroName //name of starting hero
{
	int heroID;
	std::string heroName;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroID & heroName;
	}
};
struct DLL_EXPORT PlayerInfo
{
	si32 p7, p8, p9;
	ui8 powerPlacehodlers; //q-ty of hero placeholders containing hero type, WARNING: powerPlacehodlers sometimes gives false 0 (eg. even if there is one placeholder), maybe different meaning???
	ui8 canHumanPlay;
	ui8 canComputerPlay;
	ui32 AITactic; //(00 - random, 01 -  warrior, 02 - builder, 03 - explorer)
	ui32 allowedFactions; //(01 - castle; 02 - rampart; 04 - tower; 08 - inferno; 16 - necropolis; 32 - dungeon; 64 - stronghold; 128 - fortress; 256 - conflux);
	ui8 isFactionRandom;
	ui32 mainHeroPortrait; //it's ID of hero with choosen portrait; 255 if standard
	std::string mainHeroName;
	std::vector<SheroName> heroesNames;
	ui8 hasMainTown;
	ui8 generateHeroAtMainTown;
	int3 posOfMainTown;
	ui8 team;
	ui8 generateHero;

	PlayerInfo(): p7(0), p8(0), p9(0), canHumanPlay(0), canComputerPlay(0),
		AITactic(0), allowedFactions(0), isFactionRandom(0),
		mainHeroPortrait(0), hasMainTown(0), generateHeroAtMainTown(0),
		team(255), generateHero(0) {};

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & p7 & p8 & p9 & canHumanPlay & canComputerPlay & AITactic & allowedFactions & isFactionRandom &
			mainHeroPortrait & mainHeroName & heroesNames & hasMainTown & generateHeroAtMainTown &
			posOfMainTown & team & generateHero;
	}
};
struct DLL_EXPORT LossCondition
{
	ElossCon typeOfLossCon;

	int3 pos;

	si32 timeLimit; // in days; -1 if not used
	const CGObjectInstance *obj; //set during map parsing: hero/town (depending on typeOfLossCon); NULL if not used


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & typeOfLossCon & pos & timeLimit & obj;
	}

	LossCondition();
};
struct DLL_EXPORT CVictoryCondition
{
	EvictoryConditions condition; //ID of condition
	ui8 allowNormalVictory, appliesToAI;

	int3 pos; //pos of city to upgrade (3); pos of town to build grail, {-1,-1,-1} if not relevant (4); hero pos (5); town pos(6); monster pos (7); destination pos(8)
	si32 ID; //artifact ID (0); monster ID (1); resource ID (2); needed fort level in upgraded town (3); artifact ID (8)
	si32 count; //needed count for creatures (1) / resource (2); upgraded town hall level (3); 

	const CGObjectInstance *obj; //object of specific monster / city / hero instance (NULL if not used); set during map parsing

	CVictoryCondition();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & condition & allowNormalVictory & appliesToAI & pos & ID & count & obj;
	}
};

struct DLL_EXPORT Rumor
{
	std::string name, text;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name & text;
	}
};

struct DLL_EXPORT DisposedHero
{
	ui32 ID;
	ui16 portrait; //0xFF - default
	std::string name;
	ui8 players; //who can hire this hero (bitfield)

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & portrait & name & players;
	}
};

class DLL_EXPORT CMapEvent
{
public:
	std::string name, message;
	std::vector<si32> resources; //gained / taken resources
	ui8 players; //affected players
	ui8 humanAffected;
	ui8 computerAffected;
	ui32 firstOccurence;
	ui32 nextOccurence; //after nextOccurance day event will occur; if it it 0, event occures only one time;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name & message & resources
			& players & humanAffected & computerAffected & firstOccurence & nextOccurence;
	}
	bool operator<(const CMapEvent &b) const
	{
		return firstOccurence < b.firstOccurence;
	}
};
class DLL_EXPORT CMapHeader
{
public:
	enum Eformat {invalid, WoG=0x33, AB=0x15, RoE=0x0e,  SoD=0x1c};
	Eformat version; // version of map Eformat
	ui8 areAnyPLayers; // if there are any playable players on map
	si32 height, width, twoLevel; //sizes
	std::string name;  //name of map
	std::string description;  //and description
	ui8 difficulty; // 0 easy - 4 impossible
	ui8 levelLimit;
	LossCondition lossCondition;
	CVictoryCondition victoryCondition; //victory conditions
	std::vector<PlayerInfo> players; // info about players - size 8
	ui8 howManyTeams;
	std::vector<ui8> allowedHeroes; //allowedHeroes[hero_ID] - if the hero is allowed
	std::vector<ui16> placeholdedHeroes; //ID of types of heroes in placeholders
	void initFromMemory(const unsigned char *bufor, int &i);
	void loadViCLossConditions( const unsigned char * bufor, int &i);
	void loadPlayerInfo( int &pom, const unsigned char * bufor, int &i);
	CMapHeader(const unsigned char *map); //an argument is a reference to string described a map (unpacked)
	CMapHeader();
	virtual ~CMapHeader();


	template <typename Handler> void serialize(Handler &h, const int Version)
	{
		h & version & name & description & width & height & twoLevel & difficulty & levelLimit & areAnyPLayers;
		h & players & lossCondition & victoryCondition & howManyTeams;
	}
};

struct DLL_EXPORT Mapa : public CMapHeader
{
	ui32 checksum;
	TerrainTile*** terrain; 
	std::vector<Rumor> rumors;
	std::vector<DisposedHero> disposedHeroes;
	std::vector<CGHeroInstance*> predefinedHeroes;
	std::vector<CGDefInfo *> defy; // list of .def files with definitions from .h3m (may be custom)
	std::vector<ui8> allowedSpell; //allowedSpell[spell_ID] - if the spell is allowed
	std::vector<ui8> allowedArtifact; //allowedArtifact[artifact_ID] - if the artifact is allowed
	std::vector<ui8> allowedAbilities; //allowedAbilities[ability_ID] - if the ability is allowed
	std::list<CMapEvent*> events;

	int3 grailPos;
	int grailRadious;

	std::vector<CGObjectInstance*> objects;
	std::vector<CGHeroInstance*> heroes;
	std::vector<CGTownInstance*> towns;
	std::map<ui16, CGCreature*> monsters;
	std::map<ui16, CGHeroInstance*> heroesToBeat;

	void initFromBytes( const unsigned char * bufor); //creates map from decompressed .h3m data

	void readEvents( const unsigned char * bufor, int &i);
	void readObjects( const unsigned char * bufor, int &i);
	void loadQuest( CQuest * guard, const unsigned char * bufor, int & i);
	void readDefInfo( const unsigned char * bufor, int &i);
	void readTerrain( const unsigned char * bufor, int &i);
	void readPredefinedHeroes( const unsigned char * bufor, int &i);
	void readHeader( const unsigned char * bufor, int &i);
	void readRumors( const unsigned char * bufor, int &i);
	void loadHero( CGObjectInstance * &nobj, const unsigned char * bufor, int &i);
	void loadTown( CGObjectInstance * &nobj, const unsigned char * bufor, int &i, int subid);
	int loadSeerHut( const unsigned char * bufor, int i, CGObjectInstance *& nobj);

	void checkForObjectives();

	void addBlockVisTiles(CGObjectInstance * obj);
	void removeBlockVisTiles(CGObjectInstance * obj, bool total=false);
	Mapa(std::string filename); //creates map structure from .h3m file
	Mapa();
	~Mapa();
	TerrainTile &getTile(int3 tile);
	const TerrainTile &getTile(int3 tile) const;
	CGHeroInstance * getHero(int ID, int mode=0);
	bool isInTheMap(const int3 &pos) const;
	bool isWaterTile(const int3 &pos) const; //out-of-pos safe
	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & static_cast<CMapHeader&>(*this);
		h & rumors & allowedSpell & allowedAbilities & allowedArtifact & allowedHeroes & events & grailPos;
		h & monsters & heroesToBeat; //hoprfully serialization is now automagical?

		//TODO: viccondetails
		if(h.saving)
		{
			//saving terrain
			for (int i = 0; i < width ; i++)
				for (int j = 0; j < height ; j++)
					for (int k = 0; k <= twoLevel ; k++)
						h & terrain[i][j][k];
		}
		else
		{
			//loading terrain
			terrain = new TerrainTile**[width]; // allocate memory 
			for (int ii=0;ii<width;ii++)
			{
				terrain[ii] = new TerrainTile*[height]; // allocate memory 
				for(int jj=0;jj<height;jj++)
					terrain[ii][jj] = new TerrainTile[twoLevel+1];
			}
			for (int i = 0; i < width ; i++)
				for (int j = 0; j < height ; j++)
					for (int k = 0; k <= twoLevel ; k++)
						h & terrain[i][j][k];
		}

		//definfos
		std::vector<CGDefInfo*> defs;

		if(h.saving) //create vector with all defs used on map
		{
			for(unsigned int i=0; i<objects.size(); i++)
				if(objects[i])
					objects[i]->defInfo->serial = -1; //set serial to serial -1 - indicates that def is not present in defs vector

			for(unsigned int i=0; i<objects.size(); i++)
			{
				if(!objects[i]) continue;
				CGDefInfo *cur = objects[i]->defInfo;
				if(cur->serial < 0)
				{
					cur->serial = defs.size();
					defs.push_back(cur);
				}
			}
		}

		h & ((h.saving) ? defs : defy);

		//objects
		if(h.saving)
		{
			ui32 hlp = objects.size(); 
			h & hlp;
		}
		else
		{
			ui32 hlp;
			h & hlp;
			objects.resize(hlp);
		}

		//static members
		h & CGTeleport::objs;
		h & CGTeleport::gates;
		h & CGKeys::playerKeyMap;
		h & CGMagi::eyelist;
		h & CGPyramid::pyramidConfig;
		h & CGObelisk::obeliskCount & CGObelisk::visited;
		h & CGTownInstance::merchantArtifacts;

		for(unsigned int i=0; i<objects.size(); i++)
		{
			CGObjectInstance *&obj = objects[i];
			h & obj;

			if (obj)
			{
				si32 shlp;
				//definfo
				h & (h.saving ? (shlp=obj->defInfo->serial) : shlp); //read / write pos of definfo in defs vector
				if(!h.saving)
					obj->defInfo = defy[shlp];
			}
		}

		if(!h.saving)
		{

			for(unsigned int i=0; i<objects.size(); i++)
			{
				if(!objects[i]) continue;
				if(objects[i]->ID == HEROI_TYPE)
					heroes.push_back(static_cast<CGHeroInstance*>(objects[i]));
				else if(objects[i]->ID == TOWNI_TYPE)
					towns.push_back(static_cast<CGTownInstance*>(objects[i]));

				addBlockVisTiles(objects[i]); //recreate blockvis map
			}
			for(unsigned int i=0; i<heroes.size(); i++) //if hero is visiting/garrisoned in town set appropriate pointers
			{
				int3 vistile = heroes[i]->pos; vistile.x++;
				for(unsigned int j=0; j<towns.size(); j++)
				{
					if(vistile == towns[j]->pos) //hero stands on the town entrance
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
				const TerrainTile &t = getTile(vistile);
				if(t.tertype != TerrainTile::water) continue;
				//hero stands on the water - he must be in the boat
				for(unsigned int j = 0; j < t.visitableObjects.size(); j++)
				{
					if(t.visitableObjects[j]->ID == 8)
					{
						CGBoat *b = static_cast<CGBoat *>(t.visitableObjects[j]);
						heroes[i]->boat = b;
						b->hero = heroes[i];
						removeBlockVisTiles(b);
						break;
					}
				}
			} //heroes loop
		} //!saving
	}
};
#endif // __MAP_H__
