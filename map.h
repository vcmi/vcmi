#ifndef __MAP_H__
#define __MAP_H__
#ifdef _MSC_VER
#pragma warning (disable : 4482)
#endif
#include <cstring>
#include <vector>
#include <map>
#include <set>
#include "global.h"
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
struct DLL_EXPORT CspecificVictoryConidtions
{
	bool allowNormalVictory;
	bool appliesToAI;
};
struct DLL_EXPORT VicCon0 : public CspecificVictoryConidtions //acquire artifact
{
	int ArtifactID;
};
struct DLL_EXPORT VicCon1 : public CspecificVictoryConidtions //accumulate creatures
{
	int monsterID;
	int neededQuantity;
};
struct DLL_EXPORT VicCon2 : public CspecificVictoryConidtions // accumulate resources
{
	int resourceID;
	int neededQuantity;
};
struct DLL_EXPORT VicCon3 : public CspecificVictoryConidtions // upgrade specific town
{
	int3 posOfCity;
	int councilNeededLevel; //0 - town; 1 - city; 2 - capitol
	int fortNeededLevel;// 0 - fort; 1 - citadel; 2 - castle
};
struct DLL_EXPORT VicCon4 : public CspecificVictoryConidtions // build grail structure
{
	bool anyLocation;
	int3 whereBuildGrail;
};
struct DLL_EXPORT VicCon5 : public CspecificVictoryConidtions // defeat a specific hero
{
	int3 locationOfHero;
};
struct DLL_EXPORT VicCon6 : public CspecificVictoryConidtions // capture a specific town
{
	int3 locationOfTown;
};
struct DLL_EXPORT VicCon7 : public CspecificVictoryConidtions // defeat a specific monster
{
	int3 locationOfMonster;
};
struct DLL_EXPORT VicCona : public CspecificVictoryConidtions //transport specific artifact
{
	int artifactID;
	int3 destinationPlace;
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
	si32 wood, mercury, ore, sulfur, crystal, gems, gold; //gained / taken resources
	ui8 players; //affected players
	ui8 humanAffected;
	ui8 computerAffected;
	ui32 firstOccurence;
	ui32 nextOccurence; //after nextOccurance day event will occure; if it it 0, event occures only one time;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name & message & wood & mercury & ore & sulfur & crystal & gems & gold
			& players & humanAffected & computerAffected & firstOccurence & nextOccurence;
	}
};
class DLL_EXPORT CMapHeader
{
public:
	Eformat version; // version of map Eformat
	bool areAnyPLayers; // if there are any playable players on map
	int height, width;
	bool twoLevel; // if map has underground level
	std::string name;  //name of map
	std::string description;  //and description
	int difficulty; // 0 easy - 4 impossible
	int levelLimit;
	LossCondition lossCondition;
	EvictoryConditions victoryCondition; //victory conditions
	CspecificVictoryConidtions * vicConDetails; // used only if vistory conditions aren't standard
	PlayerInfo players[8]; // info about players
	std::vector<int> teams;  // teams[i] = team of player no i
	int howManyTeams;
	CMapHeader(unsigned char *map); //an argument is a reference to string described a map (unpacked)
};
class DLL_EXPORT CMapInfo : public CMapHeader
{
public:
	std::string filename;
	int playerAmnt, humenPlayers;
	CMapInfo(std::string fname, unsigned char *map):CMapHeader(map),filename(fname)
	{
		playerAmnt=humenPlayers=0;
		for (int i=0;i<PLAYER_LIMIT;i++)
		{
			if (players[i].canHumanPlay) {playerAmnt++;humenPlayers++;}
			else if (players[i].canComputerPlay) {playerAmnt++;}
		}
	};
};


class DLL_EXPORT mapSorter
{
public:
	ESortBy sortBy;
	bool operator()(const CMapHeader & a, const CMapHeader& b)
	{
		switch (sortBy)
		{
		case _format:
			return (a.version<b.version);
			break;
		case _loscon:
			return (a.lossCondition.typeOfLossCon<b.lossCondition.typeOfLossCon);
			break;
		case _playerAm:
			int playerAmntB,humenPlayersB,playerAmntA,humenPlayersA;
			playerAmntB=humenPlayersB=playerAmntA=humenPlayersA=0;
			for (int i=0;i<8;i++)
			{
				if (a.players[i].canHumanPlay) {playerAmntA++;humenPlayersA++;}
				else if (a.players[i].canComputerPlay) {playerAmntA++;}
				if (b.players[i].canHumanPlay) {playerAmntB++;humenPlayersB++;}
				else if (b.players[i].canComputerPlay) {playerAmntB++;}
			}
			if (playerAmntB!=playerAmntA)
				return (playerAmntA<playerAmntB);
			else
				return (humenPlayersA<humenPlayersB);
			break;
		case _size:
			return (a.width<b.width);
			break;
		case _viccon:
			return (a.victoryCondition<b.victoryCondition);
			break;
		case _name:
			return (a.name<b.name);
			break;
		default:
			return (a.name<b.name);
			break;
		}
	};
	mapSorter(ESortBy es):sortBy(es){};
};
struct DLL_EXPORT Mapa
{
	Eformat version; // version of map Eformat
	ui32 checksum;
	int twoLevel; // if map has underground level
	int difficulty; // 0 easy - 4 impossible
	int levelLimit;
	bool areAnyPLayers; // if there are any playable players on map
	std::string name;  //name of map
	std::string description;  //and description
	int height, width; 
	TerrainTile*** terrain; 
	std::vector<Rumor> rumors;
	std::vector<DisposedHero> disposedHeroes;
	std::vector<CGHeroInstance*> predefinedHeroes;
	std::vector<CGDefInfo *> defy; // list of .def files with definitions from .h3m (may be custom)
	std::set<CGDefInfo *> defs; // other defInfos - for randomized objects, objects added or modified by scripts
	PlayerInfo players[8]; // info about players
	std::vector<int> teams;  // teams[i] = team of player no i 
	LossCondition lossCondition;
	EvictoryConditions victoryCondition; //victory conditions
	CspecificVictoryConidtions * vicConDetails; // used only if vistory conditions aren't standard
	int howManyTeams;
	std::vector<bool> allowedSpell; //allowedSpell[spell_ID] - if the spell is allowed
	std::vector<bool> allowedArtifact; //allowedArtifact[artifact_ID] - if the artifact is allowed
	std::vector<bool> allowedAbilities; //allowedAbilities[ability_ID] - if the ability is allowed
	std::vector<bool> allowedHeroes; //allowedHeroes[hero_ID] - if the hero is allowed
	std::vector<CMapEvent> events;

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
	void loadViCLossConditions( unsigned char * bufor, int &i);
	void loadPlayerInfo( int &pom, unsigned char * bufor, int &i);
	void loadHero( CGObjectInstance * &nobj, unsigned char * bufor, int &i);
	void loadTown( CGObjectInstance * &nobj, unsigned char * bufor, int &i);
	int loadSeerHut( unsigned char * bufor, int i, CGObjectInstance *& nobj);


	void addBlockVisTiles(CGObjectInstance * obj);
	void removeBlockVisTiles(CGObjectInstance * obj);
	Mapa(std::string filename); //creates map structure from .h3m file
	CGHeroInstance * getHero(int ID, int mode=0);
	bool isInTheMap(int3 pos);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & version & name & description & width & height & twoLevel & difficulty & levelLimit & rumors & defy & defs
			& players & teams & lossCondition & victoryCondition & howManyTeams & allowedSpell & allowedAbilities
			& allowedArtifact &allowedHeroes & events;
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
		//TODO: recreate blockvis maps
	}
};
#endif // __MAP_H__
