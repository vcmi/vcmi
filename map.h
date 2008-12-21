#ifndef MAPD_H
#define MAPD_H
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
class CGTownInstance;
enum ESortBy{_name, _playerAm, _size, _format, _viccon, _loscon};
enum EDefType {TOWN_DEF, HERO_DEF, CREATURES_DEF, SEERHUT_DEF, RESOURCE_DEF, TERRAINOBJ_DEF, 
	EVENTOBJ_DEF, SIGN_DEF, GARRISON_DEF, ARTIFACT_DEF, WITCHHUT_DEF, SCHOLAR_DEF, PLAYERONLY_DEF, 
	SHRINE_DEF, SPELLSCROLL_DEF, PANDORA_DEF, GRAIL_DEF, CREGEN_DEF, CREGEN2_DEF, CREGEN3_DEF, 
	BORDERGUARD_DEF, HEROPLACEHOLDER_DEF};
class DLL_EXPORT CSpecObjInfo //class with object - specific info (eg. different information for creatures and heroes); use inheritance to make object - specific classes
{
};
class DLL_EXPORT CEventObjInfo : public CSpecObjInfo
{
public:
	bool areGuarders; //true if there are
	CCreatureSet guarders;
	bool isMessage; //true if there is a message
	std::string message;
	unsigned int gainedExp;
	int manaDiff; //amount of gained / lost mana
	int moraleDiff; //morale modifier
	int luckDiff; //luck modifier
	int wood, mercury, ore, sulfur, crystal, gems, gold; //gained / lost resources
	unsigned int attack; //added attack points
	unsigned int defence; //added defence points
	unsigned int power; //added power points
	unsigned int knowledge; //added knowledge points
	std::vector<int> abilities; //gained abilities
	std::vector<int> abilityLevels; //levels of gained abilities
	std::vector<int> artifacts; //gained artifacts
	std::vector<int> spells; //gained spells
	CCreatureSet creatures; //gained creatures
	unsigned char availableFor; //players whom this event is available for
	bool computerActivate; //true if computre player can activate this event
	bool humanActivate; //true if human player can activate this event
};
class DLL_EXPORT CCreatureObjInfo : public CSpecObjInfo
{
public:
	unsigned char bytes[4]; //mysterious bytes identifying creature
	unsigned int number; //number of units (0 - random)
	unsigned char character; //chracter of this set of creatures (0 - the most friendly, 4 - the most hostile)
	std::string message; //message printed for attacking hero
	int wood, mercury, ore, sulfur, crytal, gems, gold; //resources gained to hero that has won with monsters
	int gainedArtifact; //ID of artifact gained to hero
	bool neverFlees; //if true, the troops will never flee
	bool notGrowingTeam; //if true, number of units won't grow
};
class DLL_EXPORT CSignObjInfo : public CSpecObjInfo
{
public:
	std::string message; //message
};
class DLL_EXPORT CSeerHutObjInfo : public CSpecObjInfo
{
public:
	unsigned char missionType; //type of mission: 0 - no mission; 1 - reach level; 2 - reach main statistics values; 3 - win with a certain hero; 4 - win with a certain creature; 5 - collect some atifacts; 6 - have certain troops in army; 7 - collect resources; 8 - be a certain hero; 9 - be a certain player
	bool isDayLimit; //if true, there is a day limit
	int lastDay; //after this day (first day is 0) mission cannot be completed
	int m1level; //for mission 1	
	int m2attack, m2defence, m2power, m2knowledge;//for mission 2
	unsigned char m3bytes[4];//for mission 3
	unsigned char m4bytes[4];//for mission 4
	std::vector<int> m5arts;//for mission 5 - artifact ID
	std::vector<CCreature *> m6cre;//for mission 6
	std::vector<int> m6number;
	int m7wood, m7mercury, m7ore, m7sulfur, m7crystal, m7gems, m7gold;	//for mission 7
	int m8hero;//for mission 8 - hero ID
	int m9player; //for mission 9 - number; from 0 to 7

	std::string firstVisitText, nextVisitText, completedText;

	char rewardType; //type of reward: 0 - no reward; 1 - experience; 2 - mana points; 3 - morale bonus; 4 - luck bonus; 5 - resources; 6 - main ability bonus (attak, defence etd.); 7 - secondary ability gain; 8 - artifact; 9 - spell; 10 - creature
	//for reward 1
	int r1exp;
	//for reward 2
	int r2mana;
	//for reward 3
	int r3morale;
	//for reward 4
	int r4luck;
	//for reward 5
	unsigned char r5type; //0 - wood, 1 - mercury, 2 - ore, 3 - sulfur, 4 - crystal, 5 - gems, 6 - gold
	int r5amount;
	//for reward 6
	unsigned char r6type; //0 - attack, 1 - defence, 2 - power, 3 - knowledge
	int r6amount;
	//for reward 7
	int r7ability; //ability id
	unsigned char r7level; //1 - basic, 2 - advanced, 3 - expert
	//for reward 8
	int r8art;//artifact id
	//for reward 9
	int r9spell;//spell id
	//for reward 10
	int r10creature; //creature id
	int r10amount;
};
class DLL_EXPORT CWitchHutObjInfo : public CSpecObjInfo
{
public:
	std::vector<int> allowedAbilities;
};
class DLL_EXPORT CScholarObjInfo : public CSpecObjInfo
{
public:
	unsigned char bonusType; //255 - random, 0 - primary skill, 1 - secondary skill, 2 - spell

	unsigned char r0type;
	int r1; //Ability ID
	int r2; //Spell ID
};
class DLL_EXPORT CGarrisonObjInfo : public CSpecObjInfo
{
public:
	unsigned char player; //255 - nobody; 0 - 7 - players
	CCreatureSet units;
	bool movableUnits; //if true, units can be moved
};
class DLL_EXPORT CArtifactObjInfo : public CSpecObjInfo
{
public:
	bool areGuards;
	std::string message;
	CCreatureSet guards;
};
class DLL_EXPORT CResourceObjInfo : public CSpecObjInfo
{
public:
	bool randomAmount;
	int amount; //if not random
	bool areGuards;
	CCreatureSet guards;
	std::string message;
};
class DLL_EXPORT CPlayerOnlyObjInfo : public CSpecObjInfo
{
public:
	unsigned char player; //FF - nobody, 0 - 7
};
class DLL_EXPORT CShrineObjInfo : public CSpecObjInfo
{
public:
	unsigned char spell; //number of spell or 255
};
class DLL_EXPORT CSpellScrollObjinfo : public CSpecObjInfo
{
public:
	std::string message;
	int spell;
	bool areGuarders;
	CCreatureSet guarders;
};
class DLL_EXPORT CPandorasBoxObjInfo : public CSpecObjInfo
{
public:
	std::string message;
	bool areGuarders;
	CCreatureSet guarders;

	//gained things:
	unsigned int gainedExp;
	int manaDiff;
	int moraleDiff;
	int luckDiff;
	int wood, mercury, ore, sulfur, crystal, gems, gold;
	int attack, defence, power, knowledge;
	std::vector<int> abilities;
	std::vector<int> abilityLevels;
	std::vector<int> artifacts;
	std::vector<int> spells;
	CCreatureSet creatures;
};

class DLL_EXPORT CGrailObjInfo : public CSpecObjInfo
{
public:
	int radius; //place grail at the distance lesser or equal radius from this place
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
class DLL_EXPORT CBorderGuardObjInfo : public CSpecObjInfo //copied form seer huts, seems to be similar
{
public:
	char missionType; //type of mission: 0 - no mission; 1 - reach level; 2 - reach main statistics values; 3 - win with a certain hero; 4 - win with a certain creature; 5 - collect some atifacts; 6 - have certain troops in army; 7 - collect resources; 8 - be a certain hero; 9 - be a certain player
	bool isDayLimit; //if true, there is a day limit
	int lastDay; //after this day (first day is 0) mission cannot be completed
	//for mission 1
	int m1level;
	//for mission 2
	int m2attack, m2defence, m2power, m2knowledge;
	//for mission 3
	unsigned char m3bytes[4];
	//for mission 4
	unsigned char m4bytes[4];
	//for mission 5
	std::vector<int> m5arts; //artifacts id
	//for mission 6
	std::vector<CCreature *> m6cre;
	std::vector<int> m6number;
	//for mission 7
	int m7wood, m7mercury, m7ore, m7sulfur, m7crystal, m7gems, m7gold;
	//for mission 8
	int m8hero; //hero id
	//for mission 9
	int m9player; //number; from 0 to 7

	std::string firstVisitText, nextVisitText, completedText;
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

	std::vector<CGObjectInstance*> objects;
	std::vector<CGHeroInstance*> heroes;
	std::vector<CGTownInstance*> towns;

	void initFromBytes(unsigned char * bufor); //creates map from decompressed .h3m data

	void readEvents( unsigned char * bufor, int &i);
	void readObjects( unsigned char * bufor, int &i);
	void readDefInfo( unsigned char * bufor, int &i);
	void readTerrain( unsigned char * bufor, int &i);
	void readPredefinedHeroes( unsigned char * bufor, int &i);
	void readHeader( unsigned char * bufor, int &i);
	void readRumors( unsigned char * bufor, int &i);
	void loadViCLossConditions( unsigned char * bufor, int &i);
	void loadPlayerInfo( int &pom, unsigned char * bufor, int &i);
	void loadHero( CGObjectInstance * &nobj, unsigned char * bufor, int &i);
	void loadTown( CGObjectInstance * &nobj, unsigned char * bufor, int &i);
	int loadSeerHut( unsigned char * bufor, int i, CGObjectInstance * nobj);


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
#endif //MAPD_H
