#ifndef COBJECTHANDLER_H
#define COBJECTHANDLER_H
#include "../global.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include "CCreatureHandler.h"

using boost::logic::tribool;
class IGameCallback;
struct BattleResult;
class CCPPObjectScript;
class CGObjectInstance;
class CScript;
class CObjectScript;
class CGHeroInstance;
class CTown;
class CHero;
class CBuilding;
class CSpell;
class CGTownInstance;
class CArtifact;
class CGDefInfo;
class CSpecObjInfo;

class DLL_EXPORT CCastleEvent
{
public:
	std::string name, message;
	int wood, mercury, ore, sulfur, crystal, gems, gold; //gain / loss of resources
	unsigned char players; //players for whom this event can be applied
	bool forHuman, forComputer;
	int firstShow; //postpone of first encounter time in days
	int forEvery; //every n days this event will occure

	unsigned char bytes[6]; //build specific buildings (raw format, similar to town's)

	int gen[7]; //additional creatures in i-th level dwelling

	bool operator<(const CCastleEvent &drugie) const
	{
		return firstShow<drugie.firstShow;
	}
};

class DLL_EXPORT IObjectInterface
{
public:
	static IGameCallback *cb;

	IObjectInterface();
	virtual ~IObjectInterface();

	virtual void onHeroVisit(const CGHeroInstance * h);
	virtual void onHeroLeave(const CGHeroInstance * h);
	virtual void newTurn();
	virtual void initObj();
};

class DLL_EXPORT CGObjectInstance : protected IObjectInterface
{
protected:
public:
	mutable std::string hoverName;
	int3 pos; //h3m pos
	int ID, subID; //normal ID (this one from OH3 maps ;]) - eg. town=98; hero=34
	si32 id;//number of object in CObjectHandler's vector		
	CGDefInfo * defInfo;
	CCPPObjectScript * state;
	CSpecObjInfo * info;
	unsigned char animPhaseShift;

	ui8 tempOwner; //uzywane dla szybkosci, skrypt ma obowiazek aktualizowac te zmienna
	ui8 blockVisit; //if non-zero then blocks the tile but is visitable from neighbouring tile

	int getOwner() const;
	void setOwner(int ow);
	int getWidth() const; //returns width of object graphic in tiles
	int getHeight() const; //returns height of object graphic in tiles
	bool visitableAt(int x, int y) const; //returns true if object is visitable at location (x, y) form left top tile of image (x, y in tiles)
	bool blockingAt(int x, int y) const; //returns true if object is blocking location (x, y) form left top tile of image (x, y in tiles)
	bool operator<(const CGObjectInstance & cmp) const;  //screen printing priority comparing
	CGObjectInstance();
	virtual ~CGObjectInstance();
	CGObjectInstance(const CGObjectInstance & right);
	CGObjectInstance& operator=(const CGObjectInstance & right);
	virtual const std::string & getHoverText() const;
	//////////////////////////////////////////////////////////////////////////
	void initObj();

	friend class CGameHandler;
};

class  DLL_EXPORT CArmedInstance: public CGObjectInstance
{
public:
	CCreatureSet army; //army
	virtual bool needsLastStack() const; //true if last stack cannot be taken
};

class DLL_EXPORT CGHeroInstance : public CArmedInstance
{
public:
	//////////////////////////////////////////////////////////////////////////

	mutable int moveDir; //format:	123
					//		8 4
					//		765
	mutable ui8 isStanding, tacticFormationEnabled;

	//////////////////////////////////////////////////////////////////////////

	CHero * type;
	ui32 exp; //experience point
	int level; //current level of hero
	std::string name; //may be custom
	std::string biography; //if custom
	int portrait; //may be custom
	int mana; // remaining spell points
	std::vector<int> primSkills; //0-attack, 1-defence, 2-spell power, 3-knowledge
	std::vector<std::pair<ui8,ui8> > secSkills; //first - ID of skill, second - level of skill (1 - basic, 2 - adv., 3 - expert); if hero has ability (-1, -1) it meansthat it should have default secondary abilities
	int movement; //remaining movement points
	int identifier; //from the map file
	bool sex;
	struct DLL_EXPORT Patrol
	{
		Patrol(){patrolling=false;patrolRadious=-1;};
		bool patrolling;
		int patrolRadious;
	} patrol;
	bool inTownGarrison; // if hero is in town garrison 
	CGTownInstance * visitedTown; //set if hero is visiting town or in the town garrison
	std::vector<ui32> artifacts; //hero's artifacts from bag
	std::map<ui16,ui32> artifWorn; //map<position,artifact_id>; positions: 0 - head; 1 - shoulders; 2 - neck; 3 - right hand; 4 - left hand; 5 - torso; 6 - right ring; 7 - left ring; 8 - feet; 9 - misc1; 10 - misc2; 11 - misc3; 12 - misc4; 13 - mach1; 14 - mach2; 15 - mach3; 16 - mach4; 17 - spellbook; 18 - misc5
	std::set<ui32> spells; //known spells (spell IDs)

	//////////////////////////////////////////////////////////////////////////

	const std::string &getBiography() const;
	bool needsLastStack()const;
	unsigned int getTileCost(const EterrainType & ttype, const Eroad & rdtype, const Eriver & rvtype) const;
	unsigned int getLowestCreatureSpeed() const;
	unsigned int getAdditiveMoveBonus() const;
	float getMultiplicativeMoveBonus() const;
	int3 getPosition(bool h3m) const; //h3m=true - returns position of hero object; h3m=false - returns position of hero 'manifestation'
	int getSightDistance() const; //returns sight distance of this hero
	int manaLimit() const; //maximum mana value for this hero (basically 10*knowledge)
	bool canWalkOnSea() const;
	int getCurrentLuck() const;
	int getCurrentMorale() const;
	int getPrimSkillLevel(int id) const;
	int getSecSkillLevel(const int & ID) const; //0 - no skill
	int maxMovePoints(bool onLand) const;
	ui32 getArtAtPos(ui16 pos) const; //-1 - no artifact
	void setArtAtPos(ui16 pos, int art);
	const CArtifact * getArt(int pos) const;
	int getSpellSecLevel(int spell) const; //returns level of secondary ability (fire, water, earth, air magic) known to this hero and applicable to given spell; -1 if error
	static int3 convertPosition(int3 src, bool toh3m); //toh3m=true: manifest->h3m; toh3m=false: h3m->manifest

	//////////////////////////////////////////////////////////////////////////

	void initHero(); 
	void initHero(int SUBID); 
	CGHeroInstance();
	virtual ~CGHeroInstance();

	//////////////////////////////////////////////////////////////////////////
	void initObj();
	void onHeroVisit(const CGHeroInstance * h);
};

class DLL_EXPORT CGTownInstance : public CArmedInstance
{
public:
	CTown * town;
	std::string name; // name of town
	int builded; //how many buildings has been built this turn
	int destroyed; //how many buildings has been destroyed this turn
	const CGHeroInstance * garrisonHero, *visitingHero;
	int identifier; //special identifier from h3m (only > RoE maps)
	int alignment;
	std::set<si32> forbiddenBuildings, builtBuildings;
	std::vector<int> possibleSpells, obligatorySpells;
	std::vector<std::vector<ui32> > spells; //spells[level] -> vector of spells, first will be available in guild

	struct StrInfo
	{
		std::map<si32,ui32> creatures; //level - available amount

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & creatures;
		}
	} strInfo;
	std::set<CCastleEvent> events;


	bool needsLastStack() const;
	int getSightDistance() const; //returns sight distance
	int fortLevel() const; //0 - none, 1 - fort, 2 - citadel, 3 - castle
	int hallLevel() const; // -1 - none, 0 - village, 1 - town, 2 - city, 3 - capitol
	int mageGuildLevel() const; // -1 - none, 0 - village, 1 - town, 2 - city, 3 - capitol
	bool creatureDwelling(const int & level, bool upgraded=false) const;
	int getHordeLevel(const int & HID) const; //HID - 0 or 1; returns creature level or -1 if that horde structure is not present
	int creatureGrowth(const int & level) const;
	bool hasFort() const;
	bool hasCapitol() const;
	int dailyIncome() const;
	int spellsAtLevel(int level, bool checkGuild) const; //levels are counted from 1 (1 - 5)

	CGTownInstance();
	virtual ~CGTownInstance();

	//////////////////////////////////////////////////////////////////////////


	void onHeroVisit(const CGHeroInstance * h);
	void onHeroLeave(const CGHeroInstance * h);
	void initObj();
};

class DLL_EXPORT CGVisitableOPH : public CGObjectInstance //objects visitable only once per hero
{
public:
	std::set<si32> visitors; //ids of heroes who have visited this obj
	si8 ttype; //tree type - used only by trees of knowledge: 0 - give level for free; 1 - take 2000 gold; 2 - take 10 gems
	const std::string & getHoverText() const;

	void onHeroVisit(const CGHeroInstance * h);
	void onNAHeroVisit(int heroID, bool alreadyVisited);
	void initObj();
	void treeSelected(int heroID, int resType, int resVal, int expVal, ui32 result); //handle player's anwer to the Tree of Knowledge dialog
};

class DLL_EXPORT CGEvent : public CGObjectInstance //event objects
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

class DLL_EXPORT CGCreature : public CArmedInstance //creatures on map
{
public:
	ui32 identifier; //unique code for this monster (used in missions)
	ui8 character; //chracter of this set of creatures (0 - the most friendly, 4 - the most hostile)
	std::string message; //message printed for attacking hero
	std::vector<ui32> resources; //[res_id], resources given to hero that has won with monsters
	si32 gainedArtifact; //ID of artifact gained to hero, -1 if none
	ui8 neverFlees; //if true, the troops will never flee
	ui8 notGrowingTeam; //if true, number of units won't grow

	void onHeroVisit(const CGHeroInstance * h);
	void endBattle(BattleResult *result);
	void initObj();
}; 


class DLL_EXPORT CGSignBottle : public CGObjectInstance //signs and ocean bottles
{
	//TODO: generate default message if sign is 'empty'
public:
	std::string message;
};

class DLL_EXPORT CGSeerHut : public CGObjectInstance
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

class DLL_EXPORT CGWitchHut : public CGObjectInstance
{
public:
	std::vector<int> allowedAbilities;
};


class DLL_EXPORT CGScholar : public CGObjectInstance
{
public:
	ui8 bonusType; //255 - random, 0 - primary skill, 1 - secondary skill, 2 - spell

	ui8 r0type;
	ui32 r1; //Ability ID
	ui32 r2; //Spell ID
};

class DLL_EXPORT CGGarrison : public CArmedInstance
{
public:
	bool removableUnits;
};

class DLL_EXPORT CGArtifact : public CArmedInstance
{
public:
	std::string message;
	ui32 spell; //if it's spell scroll
};

class DLL_EXPORT CGResource : public CArmedInstance
{
public:
	int amount; //0 if random
	std::string message;
};

class DLL_EXPORT CGShrine : public CGObjectInstance
{
public:
	unsigned char spell; //number of spell or 255 if random
};

class DLL_EXPORT CGPandoraBox : public CArmedInstance
{
public:
	std::string message;

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

class DLL_EXPORT CGQuestGuard : public CArmedInstance
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

class DLL_EXPORT CObjectHandler
{
public:
	std::vector<std::string> names; //vector of objects; i-th object in vector has subnumber i
	std::vector<int> cregens; //type 17. dwelling subid -> creature ID
	void loadObjects();

	std::vector<std::string> creGens; //names of creatures' generators
	std::vector<std::string> advobtxt;
	std::vector<std::string> xtrainfo;
	std::vector<std::string> restypes;
	std::vector<std::pair<std::string,std::string> > mines; //first - name; second - event description
};


#endif //COBJECTHANDLER_H
