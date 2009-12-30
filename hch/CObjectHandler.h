#ifndef __COBJECTHANDLER_H__
#define __COBJECTHANDLER_H__
#include "../global.h"
#include <string>
#include <vector>
#include <set>
#include <map>
#include <list>
#include "CCreatureHandler.h"
#include "../lib/HeroBonus.h"
#ifndef _MSC_VER
#include "CHeroHandler.h"
#include "CTownHandler.h"
#include "../lib/VCMI_Lib.h"
#endif

/*
 * CObjectHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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
class CGTownBuilding;
class CArtifact;
class CGDefInfo;
class CSpecObjInfo;
struct TerrainTile;
struct InfoWindow;
struct Component;
struct BankConfig;
class CGBoat;

class DLL_EXPORT CCastleEvent
{
public:
	std::string name, message;
	std::vector<si32> resources;  //gain / loss of resources
	ui8 players; //players for whom this event can be applied
	ui8 forHuman, forComputer;
	ui32 firstShow; //postpone of first encounter time in days
	ui32 forEvery; //every n days this event will occure
	ui8 bytes[6]; //build specific buildings (raw format, similar to town's)
	si32 gen[7]; //additional creatures in i-th level dwelling

	bool operator<(const CCastleEvent &drugie) const
	{
		return firstShow<drugie.firstShow;
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & name & message & resources & players & forHuman & forComputer & firstShow 
			& forEvery & bytes & gen;
	}
};

class CQuest
{
public:
	enum Emission {MISSION_NONE = 0, MISSION_LEVEL = 1, MISSION_PRIMARY_STAT = 2, MISSION_KILL_HERO = 3, MISSION_KILL_CREATURE = 4,
		MISSION_ART = 5, MISSION_ARMY = 6, MISSION_RESOURCES = 7, MISSION_HERO = 8, MISSION_PLAYER = 9};

	ui8 missionType;
	si32 lastDay; //after this day (first day is 0) mission cannot be completed; if -1 - no limit

	ui32 m13489val;
	std::vector<ui32> m2stats;
	std::vector<ui16> m5arts; //artifacts id
	std::vector<std::pair<ui32, ui32> > m6creatures; //pair[cre id, cre count]
	std::vector<ui32> m7resources;

	std::string firstVisitText, nextVisitText, completedText;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & missionType & lastDay & m13489val & m2stats & m5arts & m6creatures & m7resources
			& firstVisitText & nextVisitText & completedText;
	}
};

class DLL_EXPORT IObjectInterface
{
public:
	static IGameCallback *cb;

	IObjectInterface();
	virtual ~IObjectInterface();

	virtual void onHeroVisit(const CGHeroInstance * h) const;
	virtual void onHeroLeave(const CGHeroInstance * h) const;
	virtual void newTurn() const;
	virtual void initObj(); //synchr
	virtual void setProperty(ui8 what, ui32 val);//synchr

	static void preInit(); //called before objs receive their initObj
	static void postInit();//caleed after objs receive their initObj
};

class DLL_EXPORT IShipyard
{
public:
	const CGObjectInstance *o;

	IShipyard(const CGObjectInstance *O);
	void getBoatCost(std::vector<si32> &cost) const;
	virtual void getOutOffsets(std::vector<int3> &offsets) const =0; //offsets to obj pos when we boat can be placed
	//virtual bool validLocation() const; //returns true if there is a water tile near where boat can be placed
	int3 bestLocation() const; //returns location when the boat should be placed
	int state() const; //0 - can buid, 1 - there is already a boat at dest tile, 2 - dest tile is blocked, 3 - no water

	static const IShipyard *castFrom(const CGObjectInstance *obj);
	static IShipyard *castFrom(CGObjectInstance *obj);
};

class DLL_EXPORT CGObjectInstance : public IObjectInterface
{
protected:
	void getNameVis(std::string &hname) const;
	void giveDummyBonus(int heroID, ui8 duration = HeroBonus::ONE_DAY) const;
public:
	mutable std::string hoverName;
	int3 pos; //h3m pos
	si32 ID, subID; //normal ID (this one from OH3 maps ;]) - eg. town=98; hero=34
	si32 id;//number of object in CObjectHandler's vector		
	CGDefInfo * defInfo;
	CSpecObjInfo * info;
	ui8 animPhaseShift;

	ui8 tempOwner;
	ui8 blockVisit; //if non-zero then blocks the tile but is visitable from neighbouring tile

	virtual ui8 getPassableness() const; //bitmap - if the bit is set the corresponding player can pass through the visitable tiles of object, even if it's blockvis; if not set - default properties from definfo are used
	virtual int3 getSightCenter() const; //"center" tile from which the sight distance is calculated
	virtual int getSightRadious() const; //sight distance (should be used if player-owned structure)
	void getSightTiles(std::set<int3> &tiles) const; //returns reference to the set
	int getOwner() const;
	void setOwner(int ow);
	int getWidth() const; //returns width of object graphic in tiles
	int getHeight() const; //returns height of object graphic in tiles
	bool visitableAt(int x, int y) const; //returns true if object is visitable at location (x, y) form left top tile of image (x, y in tiles)
	int3 getVisitableOffset() const; //returns (x,y,0) offset to first visitable tile from bottom right obj tile (0,0,0) (h3m pos)
	bool blockingAt(int x, int y) const; //returns true if object is blocking location (x, y) form left top tile of image (x, y in tiles)
	bool coveringAt(int x, int y) const; //returns true if object covers with picture location (x, y) form left top tile of maximal possible image (8 x 6 tiles) (x, y in tiles)
	std::set<int3> getBlockedPos() const; //returns set of positions blocked by this object
	bool operator<(const CGObjectInstance & cmp) const;  //screen printing priority comparing
	CGObjectInstance();
	virtual ~CGObjectInstance();
	//CGObjectInstance(const CGObjectInstance & right);
	//CGObjectInstance& operator=(const CGObjectInstance & right);
	virtual const std::string & getHoverText() const;
	//////////////////////////////////////////////////////////////////////////
	void initObj();
	void onHeroVisit(const CGHeroInstance * h) const;
	void setProperty(ui8 what, ui32 val);//synchr
	virtual void setPropertyDer(ui8 what, ui32 val);//synchr

	friend class CGameHandler;


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hoverName & pos & ID & subID & id & animPhaseShift & tempOwner & blockVisit;
		//definfo is handled by map serializer
	}
};

class DLL_EXPORT CPlayersVisited: public CGObjectInstance
{
public:
	std::set<ui8> players; //players that visited this object

	bool hasVisited(ui8 player) const;
	virtual void setPropertyDer( ui8 what, ui32 val );

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & players;
	}
};

class DLL_EXPORT CArmedInstance: public CGObjectInstance
{
public:
	CCreatureSet army; //army
	virtual bool needsLastStack() const; //true if last stack cannot be taken
	int getArmyStrength() const; //sum of AI values of creatures
	ui64 getPower (TSlot slot) const; //value of specific stack
	std::string getRoughAmount (TSlot slot) const; //rought size of specific stack

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & army;
	}
};

class DLL_EXPORT CGHeroInstance : public CArmedInstance
{
public:
	//////////////////////////////////////////////////////////////////////////

	ui8 moveDir; //format:	123
					//		8 4
					//		765
	mutable ui8 isStanding, tacticFormationEnabled;

	//////////////////////////////////////////////////////////////////////////

	CHero * type;
	ui64 exp; //experience points
	si32 level; //current level of hero
	std::string name; //may be custom
	std::string biography; //if custom
	si32 portrait; //may be custom
	si32 mana; // remaining spell points
	std::vector<si32> primSkills; //0-attack, 1-defence, 2-spell power, 3-knowledge
	std::vector<std::pair<ui8,ui8> > secSkills; //first - ID of skill, second - level of skill (1 - basic, 2 - adv., 3 - expert); if hero has ability (-1, -1) it meansthat it should have default secondary abilities
	si32 movement; //remaining movement points
	si32 identifier; //from the map file
	ui8 sex;
	ui8 inTownGarrison; // if hero is in town garrison 
	CGTownInstance * visitedTown; //set if hero is visiting town or in the town garrison
	CGBoat *boat; //set to CGBoat when sailing
	std::vector<ui32> artifacts; //hero's artifacts from bag
	std::map<ui16,ui32> artifWorn; //map<position,artifact_id>; positions: 0 - head; 1 - shoulders; 2 - neck; 3 - right hand; 4 - left hand; 5 - torso; 6 - right ring; 7 - left ring; 8 - feet; 9 - misc1; 10 - misc2; 11 - misc3; 12 - misc4; 13 - mach1; 14 - mach2; 15 - mach3; 16 - mach4; 17 - spellbook; 18 - misc5
	std::set<ui32> spells; //known spells (spell IDs)

	struct DLL_EXPORT Patrol
	{
		Patrol(){patrolling=false;patrolRadious=-1;};
		ui8 patrolling;
		si32 patrolRadious;
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & patrolling & patrolRadious;
		}
	} patrol;

	std::list<HeroBonus> bonuses;
	//////////////////////////////////////////////////////////////////////////


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & exp & level & name & biography & portrait & mana & primSkills & secSkills & movement
			& identifier & sex & inTownGarrison & artifacts & artifWorn & spells & patrol & bonuses
			& moveDir;

		ui8 standardType = (VLC->heroh->heroes[subID] == type);
		h & standardType;
		if(!standardType)
			h & type;
		else if(!h.saving)
			type = VLC->heroh->heroes[subID];
		//visitied town pointer will be restored by map serialization method
	}
	//////////////////////////////////////////////////////////////////////////

	int3 getSightCenter() const; //"center" tile from which the sight distance is calculated
	int getSightRadious() const; //sight distance (should be used if player-owned structure)

	//////////////////////////////////////////////////////////////////////////
	const HeroBonus *getBonus(int from, int id) const;
	int valOfBonuses(HeroBonus::BonusType type, int subtype = -1) const; //subtype -> subtype of bonus, if -1 then any
	bool hasBonusOfType(HeroBonus::BonusType type, int subtype = -1) const; //determines if hero has a bonus of given type (and optionally subtype)
	const std::string &getBiography() const;
	bool needsLastStack()const;
	unsigned int getTileCost(const TerrainTile &dest, const TerrainTile &from) const; //move cost - applying pathfinding skill, road and terrain modifiers. NOT includes diagonal move penalty, last move levelling
	unsigned int getLowestCreatureSpeed() const;
	int3 getPosition(bool h3m) const; //h3m=true - returns position of hero object; h3m=false - returns position of hero 'manifestation'
	si32 manaLimit() const; //maximum mana value for this hero (basically 10*knowledge)
	si32 manaRegain() const; //how many points of mana can hero regain "naturally" in one day
	bool canWalkOnSea() const;
	int getCurrentLuck(int stack=-1, bool town=false) const;
	std::vector<std::pair<int,std::string> > getCurrentLuckModifiers(int stack=-1, bool town=false) const; //args as above
	int getCurrentMorale(int stack=-1, bool town=false) const; //if stack - position of creature, if -1 then morale for hero is calculated; town - if bonuses from town (tavern) should be considered
	std::vector<std::pair<int,std::string> > getCurrentMoraleModifiers(int stack=-1, bool town=false) const; //args as above
	int getPrimSkillLevel(int id) const;
	ui8 getSecSkillLevel(const int & ID) const; //0 - no skill
	int maxMovePoints(bool onLand) const;
	ui32 getArtAtPos(ui16 pos) const; //-1 - no artifact
	const CArtifact * getArt(int pos) const;
	si32 getArtPos(int aid) const; //looks for equipped artifact with given ID and returns its slot ID or -1 if none(if more than one such artifact lower ID is returned)
	void giveArtifact (ui32 aid);
	int getSpellSecLevel(int spell) const; //returns level of secondary ability (fire, water, earth, air magic) known to this hero and applicable to given spell; -1 if error
	static int3 convertPosition(int3 src, bool toh3m); //toh3m=true: manifest->h3m; toh3m=false: h3m->manifest
	double getHeroStrength() const;
	int getTotalStrength() const;
	ui8 getSpellSchoolLevel(const CSpell * spell) const; //returns level on which given spell would be cast by this hero
	bool canCastThisSpell(const CSpell * spell) const; //determines if this hero can cast given spell; takes into account existing spell in spellbook, existing spellbook and artifact bonuses
	std::pair<ui32, si32> calculateNecromancy (const BattleResult &battleResult) const;
	void showNecromancyDialog (std::pair<ui32, si32> raisedStack) const;

	//////////////////////////////////////////////////////////////////////////

	void initHero(); 
	void initHero(int SUBID); 
	void recreateArtBonuses();
	void initHeroDefInfo();
	CGHeroInstance();
	virtual ~CGHeroInstance();

	//////////////////////////////////////////////////////////////////////////

	void setPropertyDer(ui8 what, ui32 val);//synchr
	void initObj();
	void onHeroVisit(const CGHeroInstance * h) const;
};

class DLL_EXPORT CGDwelling : public CArmedInstance
{
public:
	std::vector<std::pair<ui32, std::vector<ui32> > > creatures; //creatures[level] -> <vector of alternative ids (base creature and upgrades, creatures amount>

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this) & creatures;
	}

	void initObj();
	void setProperty(ui8 what, ui32 val);
	void onHeroVisit(const CGHeroInstance * h) const;
	void newTurn() const;
	void heroAcceptsCreatures(const CGHeroInstance *h, ui32 answer) const;
	void fightOver(const CGHeroInstance *h, BattleResult *result) const;
	void wantsFight(const CGHeroInstance *h, ui32 answer) const;
};


class DLL_EXPORT CGVisitableOPH : public CGObjectInstance //objects visitable only once per hero
{
public:
	std::set<si32> visitors; //ids of heroes who have visited this obj
	si8 ttype; //tree type - used only by trees of knowledge: 0 - give level for free; 1 - take 2000 gold; 2 - take 10 gems
	const std::string & getHoverText() const;

	void setPropertyDer(ui8 what, ui32 val);//synchr
	void onHeroVisit(const CGHeroInstance * h) const;
	void onNAHeroVisit(int heroID, bool alreadyVisited) const;
	void initObj();
	void treeSelected(int heroID, int resType, int resVal, ui64 expVal, ui32 result) const; //handle player's anwer to the Tree of Knowledge dialog
	void schoolSelected(int heroID, ui32 which) const;
	void arenaSelected(int heroID, int primSkill) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & visitors & ttype;
	}
};
class DLL_EXPORT CGTownBuilding : public IObjectInterface
{
///basic class for town structures handled as map objects
public:
	si32 ID; //from buildig list
	si32 id; //identifies its index on towns vector
	CGTownInstance *town;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & id;
	}
};
class DLL_EXPORT COPWBonus : public CGTownBuilding
{///used for OPW bonusing structures
public:
	std::set<si32> visitors;
	void setProperty(ui8 what, ui32 val);
	void onHeroVisit (const CGHeroInstance * h) const;

	COPWBonus (int index, CGTownInstance *TOWN);
	COPWBonus (){ID = 0; town = NULL;};
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGTownBuilding&>(*this);
		h & visitors;
	}
};
class DLL_EXPORT CTownBonus : public CGTownBuilding
{
///used for one-time bonusing structures
///feel free to merge inheritance tree
public:
	std::set<si32> visitors;
	void setProperty(ui8 what, ui32 val);
	void onHeroVisit (const CGHeroInstance * h) const;

	CTownBonus (int index, CGTownInstance *TOWN);
	CTownBonus (){ID = 0; town = NULL;};
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGTownBuilding&>(*this);
		h & visitors;
	}
};
class DLL_EXPORT CGTownInstance : public CGDwelling, public IShipyard
{
public:
	CTown * town;
	std::string name; // name of town
	si32 builded; //how many buildings has been built this turn
	si32 destroyed; //how many buildings has been destroyed this turn
	const CGHeroInstance * garrisonHero, *visitingHero;
	ui32 identifier; //special identifier from h3m (only > RoE maps)
	si32 alignment;
	std::set<si32> forbiddenBuildings, builtBuildings;
	std::vector<CGTownBuilding*> bonusingBuildings;
	std::vector<ui32> possibleSpells, obligatorySpells;
	std::vector<std::vector<ui32> > spells; //spells[level] -> vector of spells, first will be available in guild
	std::set<CCastleEvent> events;

	//////////////////////////////////////////////////////////////////////////


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGDwelling&>(*this);
		h & name & builded & destroyed & identifier & alignment & forbiddenBuildings & builtBuildings
			& possibleSpells & obligatorySpells & spells & /*strInfo & */events & bonusingBuildings;

		for (std::vector<CGTownBuilding*>::iterator i = bonusingBuildings.begin(); i!=bonusingBuildings.end(); i++)
			(*i)->town = this;


		ui8 standardType = (&VLC->townh->towns[subID] == town);
		h & standardType;
		if(!standardType)
			h & town;
		else if(!h.saving)
			town = &VLC->townh->towns[subID];

		//garrison/visiting hero pointers will be restored in the map serialization
	}

	//////////////////////////////////////////////////////////////////////////

	int3 getSightCenter() const; //"center" tile from which the sight distance is calculated
	int getSightRadious() const; //returns sight distance
	void getOutOffsets(std::vector<int3> &offsets) const; //offsets to obj pos when we boat can be placed
	void setPropertyDer(ui8 what, ui32 val);
	void newTurn() const;

	//////////////////////////////////////////////////////////////////////////

	bool needsLastStack() const;
	int fortLevel() const; //0 - none, 1 - fort, 2 - citadel, 3 - castle
	int hallLevel() const; // -1 - none, 0 - village, 1 - town, 2 - city, 3 - capitol
	int mageGuildLevel() const; // -1 - none, 0 - village, 1 - town, 2 - city, 3 - capitol
	bool creatureDwelling(const int & level, bool upgraded=false) const;
	int getHordeLevel(const int & HID) const; //HID - 0 or 1; returns creature level or -1 if that horde structure is not present
	int creatureGrowth(const int & level) const;
	bool hasFort() const;
	bool hasCapitol() const;
	int dailyIncome() const; //calculates daily income of this town
	int spellsAtLevel(int level, bool checkGuild) const; //levels are counted from 1 (1 - 5)
	void removeCapitols (ui8 owner, bool me) const;

	CGTownInstance();
	virtual ~CGTownInstance();

	//////////////////////////////////////////////////////////////////////////


	void fightOver(const CGHeroInstance *h, BattleResult *result) const;
	void onHeroVisit(const CGHeroInstance * h) const;
	void onHeroLeave(const CGHeroInstance * h) const;
	void initObj();
};
class DLL_EXPORT CGPandoraBox : public CArmedInstance
{
public:
	std::string message;
	ui8 removeAfterVisit; //true if event is removed after occurring

	//gained things:
	ui32 gainedExp;
	si32 manaDiff; //amount of gained / lost mana
	si32 moraleDiff; //morale modifier
	si32 luckDiff; //luck modifier
	std::vector<si32> resources;//gained / lost resources
	std::vector<si32> primskills;//gained / lost resources
	std::vector<si32> abilities; //gained abilities
	std::vector<si32> abilityLevels; //levels of gained abilities
	std::vector<si32> artifacts; //gained artifacts
	std::vector<si32> spells; //gained spells
	CCreatureSet creatures; //gained creatures

	void initObj();
	void onHeroVisit(const CGHeroInstance * h) const;
	void open (const CGHeroInstance * h, ui32 accept) const;
	void endBattle(const CGHeroInstance *h, BattleResult *result) const;
	void giveContents(const CGHeroInstance *h, bool afterBattle) const;
	void getText( InfoWindow &iw, bool &afterBattle, int val, int negative, int positive, const CGHeroInstance * h ) const;
	void getText( InfoWindow &iw, bool &afterBattle, int text, const CGHeroInstance * h ) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & message & gainedExp & manaDiff & moraleDiff & luckDiff & resources & primskills
			& abilities & abilityLevels & artifacts & spells & creatures & army;
	}
};

class DLL_EXPORT CGEvent : public CGPandoraBox  //event objects
{
public:

	ui8 availableFor; //players whom this event is available for
	ui8 computerActivate; //true if computre player can activate this event
	ui8 humanActivate; //true if human player can activate this event

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & message & gainedExp & manaDiff & moraleDiff & luckDiff & resources & primskills
			& abilities & abilityLevels & artifacts & spells & creatures & availableFor 
			& computerActivate & humanActivate & army;
	}
	
	void onHeroVisit(const CGHeroInstance * h) const;
	void activated(const CGHeroInstance * h) const;

};

class DLL_EXPORT CGCreature : public CArmedInstance //creatures on map
{
public:
	ui32 identifier; //unique code for this monster (used in missions)
	si8 character; //chracter of this set of creatures (0 - the most friendly, 4 - the most hostile) => on init changed to 0 (compliant) - 10 value (savage)
	std::string message; //message printed for attacking hero
	std::vector<ui32> resources; //[res_id], resources given to hero that has won with monsters
	si32 gainedArtifact; //ID of artifact gained to hero, -1 if none
	ui8 neverFlees; //if true, the troops will never flee
	ui8 notGrowingTeam; //if true, number of units won't grow

	void fight(const CGHeroInstance *h) const;
	void onHeroVisit(const CGHeroInstance * h) const;
	//const std::string & getHoverText() const;

	void flee( const CGHeroInstance * h ) const;
	void endBattle(BattleResult *result) const;
	void fleeDecision(const CGHeroInstance *h, ui32 pursue) const;
	void joinDecision(const CGHeroInstance *h, int cost, ui32 accept) const;
	void initObj();
	int takenAction(const CGHeroInstance *h, bool allowJoin=true) const; //action on confrontation: -2 - fight, -1 - flee, >=0 - will join for given value of gold (may be 0)

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & identifier & character & message & resources & gainedArtifact & neverFlees & notGrowingTeam;
	}
}; 


class DLL_EXPORT CGSignBottle : public CGObjectInstance //signs and ocean bottles
{
public:
	std::string message;

	void onHeroVisit(const CGHeroInstance * h) const;
	void initObj();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & message;
	}
};

class DLL_EXPORT CGSeerHut : public CGObjectInstance, public CQuest
{
public:
	ui8 rewardType; //type of reward: 0 - no reward; 1 - experience; 2 - mana points; 3 - morale bonus; 4 - luck bonus; 5 - resources; 6 - main ability bonus (attak, defence etd.); 7 - secondary ability gain; 8 - artifact; 9 - spell; 10 - creature
	si32 rID; //reward ID
	si32 rVal; //reward value
	ui8 textOption; //store randomized mission write-ups

	void initObj() {};
	const std::string & getHoverText() const;
	void onHeroVisit (const CGHeroInstance * h) const {};
	void finishQuest (const CGHeroInstance * h) const {};

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this) & static_cast<CQuest&>(*this);
		h & rewardType & rID & rVal & textOption;
	}
};

class DLL_EXPORT CGWitchHut : public CPlayersVisited
{
public:
	std::vector<si32> allowedAbilities;
	ui32 ability;

	const std::string & getHoverText() const;
	void onHeroVisit(const CGHeroInstance * h) const;
	void initObj();
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPlayersVisited&>(*this);
		h & allowedAbilities & ability;
	}
};


class DLL_EXPORT CGScholar : public CGObjectInstance
{
public:
	ui8 bonusType; //255 - random, 0 - primary skill, 1 - secondary skill, 2 - spell
	ui16 bonusID; //ID of skill/spell

	void giveAnyBonus(const CGHeroInstance * h) const;
	void onHeroVisit(const CGHeroInstance * h) const;
	void initObj();
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & bonusType & bonusID;
	}
};

class DLL_EXPORT CGGarrison : public CArmedInstance
{
public:
	ui8 removableUnits;

	ui8 getPassableness() const;
	void onHeroVisit (const CGHeroInstance *h) const;
	void fightOver (const CGHeroInstance *h, BattleResult *result) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & removableUnits;
	}
};

class DLL_EXPORT CGArtifact : public CArmedInstance
{
public:
	std::string message;
	ui32 spell; //if it's spell scroll
	void onHeroVisit(const CGHeroInstance * h) const;
	void fightForArt(ui32 agreed, const CGHeroInstance *h) const;
	void endBattle(BattleResult *result, const CGHeroInstance *h) const;
	void pick( const CGHeroInstance * h ) const;
	void initObj();	

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & message & spell;
	}
};

class DLL_EXPORT CGResource : public CArmedInstance
{
public:
	ui32 amount; //0 if random
	std::string message;

	void onHeroVisit(const CGHeroInstance * h) const;
	void collectRes(int player) const;
	void initObj();
	void fightForRes(ui32 agreed, const CGHeroInstance *h) const;
	void endBattle(BattleResult *result, const CGHeroInstance *h) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & amount & message;
	}
};

class DLL_EXPORT CGPickable : public CGObjectInstance //campfire, treasure chest, Flotsam, Shipwreck Survivor, Sea Chest
{
public:
	ui32 type, val1, val2;

	void onHeroVisit(const CGHeroInstance * h) const;
	void initObj();
	void chosen(int which, int heroID) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & type & val1 & val2;
	}
};

class DLL_EXPORT CGShrine : public CPlayersVisited
{
public:
	ui8 spell; //number of spell or 255 if random
	void onHeroVisit(const CGHeroInstance * h) const;
	void initObj();
	const std::string & getHoverText() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPlayersVisited&>(*this);;
		h & spell;
	}
};

class DLL_EXPORT CGQuestGuard : public CGObjectInstance, public CQuest
{
public:
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CQuest&>(*this) & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_EXPORT CGMine : public CArmedInstance
{
public: 
	void offerLeavingGuards(const CGHeroInstance *h) const;
	void onHeroVisit(const CGHeroInstance * h) const;
	void newTurn() const;
	void initObj();	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
	}
};

class DLL_EXPORT CGVisitableOPW : public CGObjectInstance //objects visitable OPW
{
public:
	ui8 visited; //true if object has been visited this week

	void setPropertyDer(ui8 what, ui32 val);//synchr
	void onHeroVisit(const CGHeroInstance * h) const;
	void newTurn() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & visited;
	}
};

class DLL_EXPORT CGTeleport : public CGObjectInstance //teleports and subterranean gates
{
public:
	static std::map<int,std::map<int, std::vector<int> > > objs; //teleports: map[ID][subID] => vector of ids
	static std::vector<std::pair<int, int> > gates; //subterranean gates: pairs of ids
	void onHeroVisit(const CGHeroInstance * h) const;
	void initObj();	
	static void postInit();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_EXPORT CGBonusingObject : public CGObjectInstance //objects giving bonuses to luck/morale/movement
{
public:
	void onHeroVisit(const CGHeroInstance * h) const;
	const std::string & getHoverText() const;
	void initObj();	

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_EXPORT CGMagicSpring : public CGVisitableOPW 
{///unfortunatelly, this one is quite different than others 
public: 
	void onHeroVisit(const CGHeroInstance * h) const; 
	const std::string & getHoverText() const; 

	template <typename Handler> void serialize(Handler &h, const int version) 
	{ 
		h & static_cast<CGObjectInstance&>(*this); 
		h & visited; 
	} 
}; 

class DLL_EXPORT CGMagicWell : public CGObjectInstance //objects giving bonuses to luck/morale/movement
{
public:
	void onHeroVisit(const CGHeroInstance * h) const;
	const std::string & getHoverText() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_EXPORT CGSirens : public CGObjectInstance
{
public:
	void onHeroVisit(const CGHeroInstance * h) const;
	const std::string & getHoverText() const;
	void initObj();	

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_EXPORT CGObservatory : public CGObjectInstance //Redwood observatory
{
public:
	void onHeroVisit(const CGHeroInstance * h) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};


class DLL_EXPORT CGKeys : public CGObjectInstance //Base class for Keymaster and guards
{	
public:
	static std::map <ui8, std::set <ui8> > playerKeyMap; //[players][keysowned]
	//SubID 0 - lightblue, 1 - green, 2 - red, 3 - darkblue, 4 - brown, 5 - purple, 6 - white, 7 - black

	void setPropertyDer (ui8 what, ui32 val);
	bool wasMyColorVisited (int player) const;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_EXPORT CGKeymasterTent : public CGKeys
{
public:
	void onHeroVisit(const CGHeroInstance * h) const;
	const std::string & getHoverText() const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_EXPORT CGBorderGuard : public CGKeys
{
public:
	void initObj();
	const std::string & getHoverText() const;
	void onHeroVisit(const CGHeroInstance * h) const;
	void openGate(const CGHeroInstance *h, ui32 accept) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & blockVisit;
	}
};

class DLL_EXPORT CGBorderGate : public CGBorderGuard //not fully imlemented, waiting for garrison
{
public:
	void onHeroVisit(const CGHeroInstance * h) const;
	ui8 getPassableness() const;
};

class DLL_EXPORT CGBoat : public CGObjectInstance 
{
public:
	ui8 direction;
	const CGHeroInstance *hero;  //hero on board

	void initObj();	

	CGBoat()
	{
		hero = NULL;
		direction = 4;
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this) & direction;
	}
};

class DLL_EXPORT CGOnceVisitable : public CPlayersVisited
///wagon, corpse, lean to, warriors tomb
{
public:
	ui8 artOrRes; //0 - nothing; 1 - artifact; 2 - resource
	ui32 bonusType, //id of res or artifact
		bonusVal; //resource amount (or not used)

	void onHeroVisit(const CGHeroInstance * h) const;
	const std::string & getHoverText() const;
	void initObj();	
	void searchTomb(const CGHeroInstance *h, ui32 accept) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPlayersVisited&>(*this);;
		h & artOrRes & bonusType & bonusVal;
	}
};

class DLL_EXPORT CBank : public CArmedInstance
{
	public:
	int index; //banks have unusal numbering - see ZCRBANK.txt and initObj()
	BankConfig *bc;
	float multiplier; //for improved banks script
	std::vector<ui32> artifacts; //fixed and deterministic
	ui32 daycounter;

	void initObj();
	const std::string & getHoverText() const;
	void setPropertyDer (ui8 what, ui32 val);
	void initialize() const;
	void reset(ui16 var1);
	void newTurn() const;
	virtual void onHeroVisit (const CGHeroInstance * h) const;
	virtual void fightGuards (const CGHeroInstance *h, ui32 accept) const;
	virtual void endBattle (const CGHeroInstance *h, const BattleResult *result) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & index & multiplier & artifacts & daycounter & bc;
	}
};
class DLL_EXPORT CGPyramid : public CBank
{
public:
	static BankConfig pyramidConfig;
	ui16 spell;

	void initObj();
	const std::string & getHoverText() const;
	void newTurn() const {}; //empty, no reset
	void onHeroVisit (const CGHeroInstance * h) const;
	void endBattle (const CGHeroInstance *h, const BattleResult *result) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CBank&>(*this);
		h & spell;
	}
};

class CGShipyard : public CGObjectInstance, public IShipyard
{
public:
	void getOutOffsets(std::vector<int3> &offsets) const; //offsets to obj pos when we boat can be placed
	CGShipyard();
	void onHeroVisit(const CGHeroInstance * h) const;
};

class DLL_EXPORT CGMagi : public CGObjectInstance
{
public:
	static std::map <si32, std::vector<si32> > eyelist; //[subID][id], supports multiple sets as in H5

	void initObj();
	void onHeroVisit(const CGHeroInstance * h) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};



class DLL_EXPORT CCartographer : public CPlayersVisited
{
///behaviour varies depending on surface and  floor 
public:
	void onHeroVisit( const CGHeroInstance * h ) const;
	void buyMap (const CGHeroInstance *h, ui32 accept) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPlayersVisited&>(*this);
		h & players;
	}

};

class DLL_EXPORT CShop : public CGObjectInstance
{
///base class for university, art merchant, slave market etc.
public:
	std::map<ui16, Component*> available;
	std::map<ui16, Component*> chosen, bought; //redundant?
	//keys are unique for all three maps
	std::map<ui16, ui32> price;

	void initObj() {};
	void setPropertyDer (ui8 what, ui32 val);
	void newTurn() const;
	virtual void reset (ui32 val) {}; //get new items for Black Market, Tavern, Refugee Camp 
	virtual void onHeroVisit (const CGHeroInstance * h) const {};
	virtual void trade (const CGHeroInstance * h) const {};
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & available & chosen & bought & price;
	}
};
class DLL_EXPORT CGArtMerchant : public CShop
{
public:
	void reset (ui32 val);
	void onHeroVisit (const CGHeroInstance * h) const {};
};
class DLL_EXPORT CGRefugeeCamp : public CShop
{
public:
	void reset (ui32 val);
	void onHeroVisit (const CGHeroInstance * h) const {};
};
struct BankConfig
{
	BankConfig() {level = chance = upgradeChance = combatValue = value = rewardDifficulty = easiest = 0; };
	ui8 level; //1 - 4, how hard the battle will be
	ui8 chance; //chance for this level being chosen
	ui8 upgradeChance; //chance for creatures to be in upgraded versions
	std::vector< std::pair <ui16, ui32> > guards; //creature ID, amount
	ui32 combatValue; //how hard are guards of this level
	std::vector<si32> resources; //resources given in case of victory
	std::vector< std::pair <ui16, ui32> > creatures; //creatures granted in case of victory (creature ID, amount)
	std::vector<ui16> artifacts; //number of artifacts given in case of victory [0] -> treasure, [1] -> minor [2] -> major [3] -> relic
	ui32 value; //overall value of given things
	ui32 rewardDifficulty; //proportion of reward value to difficulty of guards; how profitable is this creature Bank config
	ui16 easiest; //?!?

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & level & chance & upgradeChance & guards & combatValue & resources & creatures & artifacts & value & rewardDifficulty & easiest;
	}
};

class DLL_EXPORT CObjectHandler
{
public:
	std::vector<si32> cregens; //type 17. dwelling subid -> creature ID
	std::map <ui32, std::vector <BankConfig*> > banksInfo; //[index][preset]
	std::map <ui32, std::string> creBanksNames; //[crebank index] -> name of this creature bank

	void loadObjects();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & cregens & banksInfo & creBanksNames;
	}
};


#endif // __COBJECTHANDLER_H__
