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
#include "global.h"
#ifndef _MSC_VER
#include "hch/CObjectHandler.h"
#include "hch/CDefObjInfoHandler.h"
#endif
class CGDefInfo;
class CGObjectInstance;
class CGHeroInstance;
class CQuest;
class CGTownInstance;
enum ESortBy{_name, _playerAm, _size, _format, _viccon, _loscon};
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
	int identifier;
	unsigned char castles[2]; //allowed castles
};
class DLL_EXPORT CCreGen2ObjInfo : public CSpecObjInfo
{
public:
	unsigned char player; //owner
	bool asCastle;
	int identifier;
	unsigned char castles[2]; //allowed castles
	unsigned char minLevel, maxLevel; //minimal and maximal level of creature in dwelling: <0, 6>
};
class DLL_EXPORT CCreGen3ObjInfo : public CSpecObjInfo
{
public:
	unsigned char player; //owner
	unsigned char minLevel, maxLevel; //minimal and maximal level of creature in dwelling: <0, 6>
};

struct DLL_EXPORT Sresource
{
	std::string resName; //name of this resource
	int amount; //it can be greater and lesser than 0
};
struct DLL_EXPORT TimeEvent
{
	std::string eventName;
	std::string message;
	std::vector<Sresource> decIncRes; //decreases / increases of resources
	unsigned int whichPlayers; //which players are affected by this event (+1 - first, +2 - second, +4 - third, +8 - fourth etc.)
	bool areHumansAffected;
	bool areCompsAffected;
	int firstAfterNDays; //how many days after appears this event
	int nextAfterNDays; //how many days after the epperance before appaers this event
};
struct DLL_EXPORT TerrainTile
{
	EterrainType tertype; // type of terrain
	unsigned char terview; // look of terrain
	Eriver nuine; // type of Eriver (0 if there is no Eriver)
	unsigned char rivDir; // direction of Eriver
	Eroad malle; // type of Eroad (0 if there is no Eriver)
	unsigned char roadDir; // direction of Eroad
	unsigned char siodmyTajemniczyBajt; //bitfield, info whether this tile is coastal and how to rotate tile graphics

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
	int3 castlePos;
	int3 heroPos;
	int timeLimit; // in days

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & typeOfLossCon & castlePos & heroPos & timeLimit;
	}
};
struct DLL_EXPORT CVictoryCondition
{
	EvictoryConditions condition; //ID of condition
	ui8 allowNormalVictory, appliesToAI;

	int3 pos; //pos of city to upgrade (3); pos of town to build grail, {-1,-1,-1} if not relevant (4); hero pos (5); town pos(6); monster pos (7); destination pos(8)
	ui32 ID; //artifact ID (0); monster ID (1); resource ID (2); needed fort level in upgraded town (3); artifact ID (8)
	ui32 count; //needed count for creatures (1) / resource (2); upgraded town hall level (3); 


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & condition & allowNormalVictory & appliesToAI & pos & ID & count;
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
};
class DLL_EXPORT CMapHeader
{
public:
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
	std::vector<ui8> teams;  // teams[i] = team of player no i
	ui8 howManyTeams;
	void initFromMemory(unsigned char *bufor, int &i);
	void loadViCLossConditions( unsigned char * bufor, int &i);
	void loadPlayerInfo( int &pom, unsigned char * bufor, int &i);
	CMapHeader(unsigned char *map); //an argument is a reference to string described a map (unpacked)
	CMapHeader();


	template <typename Handler> void serialize(Handler &h, const int Version)
	{
		h & version & name & description & width & height & twoLevel & difficulty & levelLimit & areAnyPLayers;
		h & players & teams & lossCondition & victoryCondition & howManyTeams;
	}
};

class DLL_EXPORT CMapInfo : public CMapHeader
{
public:
	ui8 seldiff; //selected difficulty (only in saved games)
	std::string filename;
	std::string date;
	int playerAmnt, humenPlayers;
	CMapInfo(){};
	void countPlayers();
	CMapInfo(std::string fname, unsigned char *map);
};


class DLL_EXPORT mapSorter
{
public:
	ESortBy sortBy;
	bool operator()(CMapHeader *a, CMapHeader *b)
	{
		switch (sortBy)
		{
		case _format:
			return (a->version<b->version);
			break;
		case _loscon:
			return (a->lossCondition.typeOfLossCon<b->lossCondition.typeOfLossCon);
			break;
		case _playerAm:
			int playerAmntB,humenPlayersB,playerAmntA,humenPlayersA;
			playerAmntB=humenPlayersB=playerAmntA=humenPlayersA=0;
			for (int i=0;i<8;i++)
			{
				if (a->players[i].canHumanPlay) {playerAmntA++;humenPlayersA++;}
				else if (a->players[i].canComputerPlay) {playerAmntA++;}
				if (b->players[i].canHumanPlay) {playerAmntB++;humenPlayersB++;}
				else if (b->players[i].canComputerPlay) {playerAmntB++;}
			}
			if (playerAmntB!=playerAmntA)
				return (playerAmntA<playerAmntB);
			else
				return (humenPlayersA<humenPlayersB);
			break;
		case _size:
			return (a->width<b->width);
			break;
		case _viccon:
			return (a->victoryCondition.condition < b->victoryCondition.condition);
			break;
		case _name:
			return (a->name<b->name);
			break;
		default:
			return (a->name<b->name);
			break;
		}
	};
	mapSorter(ESortBy es):sortBy(es){};
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
	std::vector<ui8> allowedHeroes; //allowedHeroes[hero_ID] - if the hero is allowed
	std::list<CMapEvent> events;

	int3 grailPos;
	int grailRadious;

	std::vector<CGObjectInstance*> objects;
	std::vector<CGHeroInstance*> heroes;
	std::vector<CGTownInstance*> towns;

	void initFromBytes(unsigned char * bufor); //creates map from decompressed .h3m data

	void readEvents( unsigned char * bufor, int &i);
	void readObjects( unsigned char * bufor, int &i);
	void loadQuest( CQuest * guard, unsigned char * bufor, int & i);
	void readDefInfo( unsigned char * bufor, int &i);
	void readTerrain( unsigned char * bufor, int &i);
	void readPredefinedHeroes( unsigned char * bufor, int &i);
	void readHeader( unsigned char * bufor, int &i);
	void readRumors( unsigned char * bufor, int &i);
	void loadHero( CGObjectInstance * &nobj, unsigned char * bufor, int &i);
	void loadTown( CGObjectInstance * &nobj, unsigned char * bufor, int &i);
	int loadSeerHut( unsigned char * bufor, int i, CGObjectInstance *& nobj);


	void addBlockVisTiles(CGObjectInstance * obj);
	void removeBlockVisTiles(CGObjectInstance * obj);
	Mapa(std::string filename); //creates map structure from .h3m file
	Mapa();
	~Mapa();
	TerrainTile &getTile(int3 tile);
	CGHeroInstance * getHero(int ID, int mode=0);
	bool isInTheMap(int3 pos);
	template <typename TObject, typename Handler> void serializeObj(Handler &h, const int version, TObject ** obj)
	{
		h & *obj;
	}
	template <typename Handler> void serialize(Handler &h, const int formatVersion)
	{
		h & static_cast<CMapHeader&>(*this);
		h & rumors & allowedSpell & allowedAbilities & allowedArtifact & allowedHeroes & events & grailPos;

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
			for(int i=0; i<objects.size(); i++)
				if(objects[i])
					objects[i]->defInfo->serial = -1; //set serial to serial -1 - indicates that def is not present in defs vector

			for(int i=0; i<objects.size(); i++)
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

		h & CGTeleport::objs;

		for(int i=0; i<objects.size(); i++)
		{
			CGObjectInstance *&obj = objects[i];
			ui8 exists = (obj!=NULL);
			ui32 hlp;
			si32 shlp;
			h & exists;
			if(!exists)
			{
				if(!h.saving)
					obj = 0;
				continue;
			}
			h & (h.saving ? (hlp=obj->ID) : hlp);
			switch(hlp)
			{
				#define SERIALIZE(TYPE) (   serializeObj<TYPE>( h,version,(TYPE**) (&obj) )   )
			case 34: case 70: case 62:
				SERIALIZE(CGHeroInstance);
				break;
			case 98: case 77:
				SERIALIZE(CGTownInstance);
				break;
			case 26: //for event objects
				SERIALIZE(CGEvent);
				break;
			case 4: //arena
			case 51: //Mercenary Camp
			case 23: //Marletto Tower
			case 61: // Star Axis
			case 32: // Garden of Revelation
			case 100: //Learning Stone
			case 102: //Tree of Knowledge
				SERIALIZE(CGVisitableOPH);
				break;
			case 55: //mystical garden
			case 112://windmill
			case 109://water wheel
				SERIALIZE(CGVisitableOPW);
				break;
			case 43: //teleport
			case 44: //teleport
			case 45: //teleport
			case 103://subterranean gate
				SERIALIZE(CGTeleport);
				break;
			case 12: //campfire
			case 101: //treasure chest
				SERIALIZE(CGPickable);
				break;
			case 54:  //Monster 
			case 71: case 72: case 73: case 74: case 75:	// Random Monster 1 - 4
			case 162: case 163: case 164:	
				SERIALIZE(CGCreature);
				break;
			case 59: case 91: //ocean bottle and sign
				SERIALIZE(CGSignBottle);
				break;
			case 83: //seer's hut
				SERIALIZE(CGSeerHut);
				break;
			case 113: //witch hut
				SERIALIZE(CGWitchHut);
				break;
			case 81: //scholar
				SERIALIZE(CGScholar);
				break;
			case 33: case 219: //garrison
				SERIALIZE(CGGarrison);
				break;
			case 5: //artifact	
			case 65: case 66: case 67: case 68: case 69: //random artifact
			case 93: //spell scroll
				SERIALIZE(CGArtifact);
				break;
			case 76: case 79: //random resource; resource
				SERIALIZE(CGResource);
				break;
			case 53: 
				SERIALIZE(CGMine);
				break;
			case 88: case 89: case 90: //spell shrine
				SERIALIZE(CGShrine);
				break;
			case 6:
				SERIALIZE(CGPandoraBox);
				break;
			case 217:
			case 216:
			case 218:
				//TODO cregen
				SERIALIZE(CGObjectInstance);
				break;
			case 215:
				SERIALIZE(CGQuestGuard);
				break;
			case 28: //faerie ring
			case 14: //Swan pond
			case 38: //idol of fortune
			case 30: //Fountain of Fortune
			case 64: //Rally Flag
			case 56: //oasis
			case 96: //temple
			case 110://Watering Hole
			case 31: //Fountain of Youth
				SERIALIZE(CGBonusingObject);
				break;
			case 49: //Magic Well
				SERIALIZE(CGMagicWell);
				break;
			default:
				SERIALIZE(CGObjectInstance);
			}

#undef SERIALIZE

			//definfo
			h & (h.saving ? (shlp=obj->defInfo->serial) : shlp); //read / write pos of definfo in defs vector
			if(!h.saving)
				obj->defInfo = defy[shlp];
		}

		if(!h.saving)
		{

			for(int i=0; i<objects.size(); i++)
			{
				if(!objects[i]) continue;
				if(objects[i]->ID == HEROI_TYPE)
					heroes.push_back(static_cast<CGHeroInstance*>(objects[i]));
				else if(objects[i]->ID == TOWNI_TYPE)
					towns.push_back(static_cast<CGTownInstance*>(objects[i]));

				addBlockVisTiles(objects[i]); //recreate blockvis map
			}
			for(int i=0; i<heroes.size(); i++) //if hero is visiting/garrisoned in town set appropriate pointers
			{
				int3 vistile = heroes[i]->pos; vistile.x++;
				for(int j=0; j<towns.size(); j++)
				{
					if(vistile == towns[j]->pos) //hero stands on the town entrance
					{
						if(heroes[i]->inTownGarrison)
							towns[j]->garrisonHero = heroes[i];
						else
							towns[j]->visitingHero = heroes[i];

						heroes[i]->visitedTown = towns[j];
					}
				}
			}

		}
	}
};
#endif // __MAP_H__
