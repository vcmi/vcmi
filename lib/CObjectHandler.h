#pragma once

#include "../lib/CCreatureSet.h"
#include "../lib/CTownHandler.h"
#include "../lib/CDefObjInfoHandler.h"
#include "CArtHandler.h"
#include "../lib/ConstTransitivePtr.h"
#include "int3.h"
#include "GameConstants.h"
#include "ResourceSet.h"
#include "CRandomGenerator.h"

/*
 * CObjectHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CGameState;
class CArtifactInstance;
struct MetaString;
struct BattleInfo;
struct QuestInfo;
class IGameCallback;
struct BattleResult;
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
class CSpecObjInfo;
class CCastleEvent;
struct TerrainTile;
struct InfoWindow;
struct Component;
struct BankConfig;
struct UpdateHerospecialty;
struct NewArtifact;
class CGBoat;
class CArtifactSet;
class CCommanderInstance;

class DLL_LINKAGE CQuest
{
public:
	enum Emission {MISSION_NONE = 0, MISSION_LEVEL = 1, MISSION_PRIMARY_STAT = 2, MISSION_KILL_HERO = 3, MISSION_KILL_CREATURE = 4,
		MISSION_ART = 5, MISSION_ARMY = 6, MISSION_RESOURCES = 7, MISSION_HERO = 8, MISSION_PLAYER = 9, MISSION_KEYMASTER = 10};
	enum Eprogress {NOT_ACTIVE, IN_PROGRESS, COMPLETE};

	si32 qid; //unique quest id for serialization / identification

	Emission missionType;
	Eprogress progress;
	si32 lastDay; //after this day (first day is 0) mission cannot be completed; if -1 - no limit

	ui32 m13489val;
	std::vector<ui32> m2stats;
	std::vector<ui16> m5arts; //artifacts id
	std::vector<CStackBasicDescriptor> m6creatures; //pair[cre id, cre count], CreatureSet info irrelevant
	std::vector<ui32> m7resources; //TODO: use resourceset?

	//following field are used only for kill creature/hero missions, the original objects became inaccessible after their removal, so we need to store info needed for messages / hover text
	ui8 textOption;
	CStackBasicDescriptor stackToKill;
	ui8 stackDirection;
	std::string heroName; //backup of hero name
	si32 heroPortrait;

	std::string firstVisitText, nextVisitText, completedText;
	bool isCustomFirst, isCustomNext, isCustomComplete;

	CQuest(){missionType = MISSION_NONE;}; //default constructor
	virtual ~CQuest(){};

	virtual bool checkQuest (const CGHeroInstance * h) const; //determines whether the quest is complete or not
	virtual void getVisitText (MetaString &text, std::vector<Component> &components, bool isCustom, bool FirstVisit, const CGHeroInstance * h = nullptr) const;
	virtual void getCompletionText (MetaString &text, std::vector<Component> &components, bool isCustom, const CGHeroInstance * h = nullptr) const;
	virtual void getRolloverText (MetaString &text, bool onHover) const; //hover or quest log entry
	virtual void completeQuest (const CGHeroInstance * h) const {};
	virtual void addReplacements(MetaString &out, const std::string &base) const;

	bool operator== (const CQuest & quest) const
	{
		return (quest.qid == qid);
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & qid & missionType & progress & lastDay & m13489val & m2stats & m5arts & m6creatures & m7resources
			& textOption & stackToKill & stackDirection & heroName & heroPortrait
			& firstVisitText & nextVisitText & completedText & isCustomFirst & isCustomNext & isCustomComplete;
	}
};

class DLL_LINKAGE IObjectInterface
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
	
	//Called when queries created DURING HERO VISIT are resolved
	//First parameter is always hero that visited object and triggered the query
	virtual void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const;
	virtual void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const;
	virtual void garrisonDialogClosed(const CGHeroInstance *hero) const;
	virtual void heroLevelUpDone(const CGHeroInstance *hero) const;

//unified interface, AI helpers
	virtual bool wasVisited (PlayerColor player) const;
	virtual bool wasVisited (const CGHeroInstance * h) const;

	static void preInit(); //called before objs receive their initObj
	static void postInit();//called after objs receive their initObj

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		logGlobal->errorStream() << "IObjectInterface serialized, unexpected, should not happen!";
	}
};

class DLL_LINKAGE IBoatGenerator
{
public:
	const CGObjectInstance *o;

	IBoatGenerator(const CGObjectInstance *O);
	virtual ~IBoatGenerator() {}

	virtual std::string getBoatAnimationName() const; 
	virtual int getBoatType() const;
	virtual void getOutOffsets(std::vector<int3> &offsets) const = 0; //offsets to obj pos when we boat can be placed
	int3 bestLocation() const; //returns location when the boat should be placed

	enum EGeneratorState {GOOD, BOAT_ALREADY_BUILT, TILE_BLOCKED, NO_WATER};
	EGeneratorState shipyardStatus() const; //0 - can buid, 1 - there is already a boat at dest tile, 2 - dest tile is blocked, 3 - no water
	void getProblemText(MetaString &out, const CGHeroInstance *visitor = nullptr) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & o;
	}
};

class DLL_LINKAGE IShipyard : public IBoatGenerator
{
public:
	IShipyard(const CGObjectInstance *O);
	virtual ~IShipyard() {}

	virtual TResources getBoatCost(int boatType) const;

	static const IShipyard *castFrom(const CGObjectInstance *obj);
	static IShipyard *castFrom(CGObjectInstance *obj);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<IBoatGenerator&>(*this);
	}
};

class DLL_LINKAGE IMarket
{
public:
	const CGObjectInstance *o;

	IMarket(const CGObjectInstance *O);
	virtual ~IMarket() {}

	virtual int getMarketEfficiency() const =0;
	virtual bool allowsTrade(EMarketMode::EMarketMode mode) const;
	virtual int availableUnits(EMarketMode::EMarketMode mode, int marketItemSerial) const; //-1 if unlimited
	virtual std::vector<int> availableItemsIds(EMarketMode::EMarketMode mode) const;

	bool getOffer(int id1, int id2, int &val1, int &val2, EMarketMode::EMarketMode mode) const; //val1 - how many units of id1 player has to give to receive val2 units
	std::vector<EMarketMode::EMarketMode> availableModes() const;

	static const IMarket *castFrom(const CGObjectInstance *obj, bool verbose = true);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & o;
	}
};

class DLL_LINKAGE CGObjectInstance : public IObjectInterface
{
public:
	mutable std::string hoverName;
	int3 pos; //h3m pos
	Obj ID;
	si32 subID; //normal subID (this one from OH3 maps ;])
	ObjectInstanceID id;//number of object in map's vector
	ObjectTemplate appearance;

	PlayerColor tempOwner;
	bool blockVisit; //if non-zero then blocks the tile but is visitable from neighbouring tile

	virtual ui8 getPassableness() const; //bitmap - if the bit is set the corresponding player can pass through the visitable tiles of object, even if it's blockvis; if not set - default properties from definfo are used
	virtual int3 getSightCenter() const; //"center" tile from which the sight distance is calculated
	virtual int getSightRadious() const; //sight distance (should be used if player-owned structure)
	bool passableFor(PlayerColor color) const;
	void getSightTiles(std::unordered_set<int3, ShashInt3> &tiles) const; //returns reference to the set
	PlayerColor getOwner() const;
	void setOwner(PlayerColor ow);
	int getWidth() const; //returns width of object graphic in tiles
	int getHeight() const; //returns height of object graphic in tiles
	virtual bool visitableAt(int x, int y) const; //returns true if object is visitable at location (x, y) (h3m pos)
	virtual int3 getVisitableOffset() const; //returns (x,y,0) offset to first visitable tile from bottom right obj tile (0,0,0) (h3m pos)
	int3 visitablePos() const;
	bool blockingAt(int x, int y) const; //returns true if object is blocking location (x, y) (h3m pos)
	bool coveringAt(int x, int y) const; //returns true if object covers with picture location (x, y) (h3m pos)
	std::set<int3> getBlockedPos() const; //returns set of positions blocked by this object
	bool isVisitable() const; //returns true if object is visitable
	bool operator<(const CGObjectInstance & cmp) const;  //screen printing priority comparing
	void hideTiles(PlayerColor ourplayer, int radius) const;
	CGObjectInstance();
	virtual ~CGObjectInstance();
	//CGObjectInstance(const CGObjectInstance & right);
	//CGObjectInstance& operator=(const CGObjectInstance & right);
	virtual const std::string & getHoverText() const;

	///IObjectInterface
	void initObj() override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void setProperty(ui8 what, ui32 val) override;//synchr

	friend class CGameHandler;


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hoverName & pos & ID & subID & id & tempOwner & blockVisit & appearance;
		//definfo is handled by map serializer
	}
protected:
	virtual void setPropertyDer(ui8 what, ui32 val);//synchr

	void getNameVis(std::string &hname) const;
	void giveDummyBonus(ObjectInstanceID heroID, ui8 duration = Bonus::ONE_DAY) const;
};

/// function object which can be used to find an object with an specific sub ID
class CGObjectInstanceBySubIdFinder
{
public:
	CGObjectInstanceBySubIdFinder(CGObjectInstance * obj);
	bool operator()(CGObjectInstance * obj) const;

private:
	CGObjectInstance * obj;
};

class CGHeroPlaceholder : public CGObjectInstance
{
public:
	//subID stores id of hero type. If it's 0xff then following field is used
	ui8 power;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & power;
	}
};

class DLL_LINKAGE CPlayersVisited: public CGObjectInstance
{
public:
	std::set<PlayerColor> players; //players that visited this object

	bool wasVisited(PlayerColor player) const;
	bool wasVisited(TeamID team) const;
	void setPropertyDer(ui8 what, ui32 val) override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & players;
	}
};

class DLL_LINKAGE CArmedInstance: public CGObjectInstance, public CBonusSystemNode, public CCreatureSet
{
public:
	BattleInfo *battle; //set to the current battle, if engaged

	void randomizeArmy(int type);
	virtual void updateMoraleBonusFromArmy();

	void armyChanged() override;

	//////////////////////////////////////////////////////////////////////////
//	int valOfGlobalBonuses(CSelector selector) const; //used only for castle interface								???
	virtual CBonusSystemNode *whereShouldBeAttached(CGameState *gs);
	virtual CBonusSystemNode *whatShouldBeAttached();
	//////////////////////////////////////////////////////////////////////////

	CArmedInstance();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & static_cast<CBonusSystemNode&>(*this);
		h & static_cast<CCreatureSet&>(*this);
	}
};

class DLL_LINKAGE CGHeroInstance : public CArmedInstance, public IBoatGenerator, public CArtifactSet
{
public:
	enum ECanDig
	{
		CAN_DIG, LACK_OF_MOVEMENT, WRONG_TERRAIN, TILE_OCCUPIED
	};
	//////////////////////////////////////////////////////////////////////////

	ui8 moveDir; //format:	123
					//		8 4
					//		765
	mutable ui8 isStanding, tacticFormationEnabled;

	//////////////////////////////////////////////////////////////////////////

	ConstTransitivePtr<CHero> type;
	TExpType exp; //experience points
	ui32 level; //current level of hero
	std::string name; //may be custom
	std::string biography; //if custom
	si32 portrait; //may be custom
	si32 mana; // remaining spell points
	std::vector<std::pair<SecondarySkill,ui8> > secSkills; //first - ID of skill, second - level of skill (1 - basic, 2 - adv., 3 - expert); if hero has ability (-1, -1) it meansthat it should have default secondary abilities
	ui32 movement; //remaining movement points
	ui8 sex;
	bool inTownGarrison; // if hero is in town garrison
	ConstTransitivePtr<CGTownInstance> visitedTown; //set if hero is visiting town or in the town garrison
	ConstTransitivePtr<CCommanderInstance> commander;
	const CGBoat *boat; //set to CGBoat when sailing


	//std::vector<const CArtifact*> artifacts; //hero's artifacts from bag
	//std::map<ui16, const CArtifact*> artifWorn; //map<position,artifact_id>; positions: 0 - head; 1 - shoulders; 2 - neck; 3 - right hand; 4 - left hand; 5 - torso; 6 - right ring; 7 - left ring; 8 - feet; 9 - misc1; 10 - misc2; 11 - misc3; 12 - misc4; 13 - mach1; 14 - mach2; 15 - mach3; 16 - mach4; 17 - spellbook; 18 - misc5
	std::set<SpellID> spells; //known spells (spell IDs)


	struct DLL_LINKAGE Patrol
	{
		Patrol(){patrolling=false;patrolRadious=-1;};
		bool patrolling;
		ui32 patrolRadious;
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & patrolling & patrolRadious;
		}
	} patrol;

	struct DLL_LINKAGE HeroSpecial : CBonusSystemNode
	{
		bool growsWithLevel;

		HeroSpecial(){growsWithLevel = false;};

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & static_cast<CBonusSystemNode&>(*this);
			h & growsWithLevel;
		}
	};

	std::vector<HeroSpecial*> specialty;

	struct DLL_LINKAGE SecondarySkillsInfo
	{
		//skills are determined, initialized at map start
		//FIXME remove mutable
		mutable CRandomGenerator rand;
		ui8 magicSchoolCounter;
		ui8 wisdomCounter;

		void resetMagicSchoolCounter();
		void resetWisdomCounter();

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & magicSchoolCounter & wisdomCounter & rand;
		}
	} skillsInfo;

	int3 getSightCenter() const; //"center" tile from which the sight distance is calculated
	int getSightRadious() const; //sight distance (should be used if player-owned structure)
	//////////////////////////////////////////////////////////////////////////

	std::string getBoatAnimationName() const; 
	int getBoatType() const;
	void getOutOffsets(std::vector<int3> &offsets) const; //offsets to obj pos when we boat can be placed

	//////////////////////////////////////////////////////////////////////////

	bool hasSpellbook() const;
	EAlignment::EAlignment getAlignment() const;
	const std::string &getBiography() const;
	bool needsLastStack()const;
	ui32 getTileCost(const TerrainTile &dest, const TerrainTile &from) const; //move cost - applying pathfinding skill, road and terrain modifiers. NOT includes diagonal move penalty, last move levelling
	ui32 getLowestCreatureSpeed() const;
	int3 getPosition(bool h3m = false) const; //h3m=true - returns position of hero object; h3m=false - returns position of hero 'manifestation'
	si32 manaRegain() const; //how many points of mana can hero regain "naturally" in one day
	bool canWalkOnSea() const;
	int getCurrentLuck(int stack=-1, bool town=false) const;
	int getSpellCost(const CSpell *sp) const; //do not use during battles -> bonuses from army would be ignored

	// ----- primary and secondary skill, experience, level handling -----

	/// Returns true if hero has lower level than should upon his experience.
	bool gainsLevel() const;

	/// Returns the next primary skill on level up. Can only be called if hero can gain a level up.
	PrimarySkill::PrimarySkill nextPrimarySkill() const;

	/// Returns the next secondary skill randomly on level up. Can only be called if hero can gain a level up.
	boost::optional<SecondarySkill> nextSecondarySkill() const;

	/// Gets 0, 1 or 2 secondary skills which are proposed on hero level up.
	std::vector<SecondarySkill> getLevelUpProposedSecondarySkills() const;

	ui8 getSecSkillLevel(SecondarySkill skill) const; //0 - no skill

	/// Returns true if hero has free secondary skill slot.
	bool canLearnSkill() const;

	void setPrimarySkill(PrimarySkill::PrimarySkill primarySkill, si64 value, ui8 abs);
	void setSecSkillLevel(SecondarySkill which, int val, bool abs);// abs == 0 - changes by value; 1 - sets to value
	void levelUp(std::vector<SecondarySkill> skills);

	int maxMovePoints(bool onLand) const;
	int movementPointsAfterEmbark(int MPsBefore, int basicCost, bool disembark = false) const;

	//int getSpellSecLevel(int spell) const; //returns level of secondary ability (fire, water, earth, air magic) known to this hero and applicable to given spell; -1 if error
	static int3 convertPosition(int3 src, bool toh3m); //toh3m=true: manifest->h3m; toh3m=false: h3m->manifest
	double getFightingStrength() const; // takes attack / defense skill into account
	double getMagicStrength() const; // takes knowledge / spell power skill into account
	double getHeroStrength() const; // includes fighting and magic strength
	ui64 getTotalStrength() const; // includes fighting strength and army strength
	TExpType calculateXp(TExpType exp) const; //apply learning skill
	ui8 getSpellSchoolLevel(const CSpell * spell, int *outSelectedSchool = nullptr) const; //returns level on which given spell would be cast by this hero (0 - none, 1 - basic etc); optionally returns number of selected school by arg - 0 - air magic, 1 - fire magic, 2 - water magic, 3 - earth magic,
	bool canCastThisSpell(const CSpell * spell) const; //determines if this hero can cast given spell; takes into account existing spell in spellbook, existing spellbook and artifact bonuses
	CStackBasicDescriptor calculateNecromancy (const BattleResult &battleResult) const;
	void showNecromancyDialog(const CStackBasicDescriptor &raisedStack) const;
	ECanDig diggingStatus() const; //0 - can dig; 1 - lack of movement; 2 -

	//////////////////////////////////////////////////////////////////////////

	void initHero();
	void initHero(HeroTypeID SUBID);

	void putArtifact(ArtifactPosition pos, CArtifactInstance *art);
	void putInBackpack(CArtifactInstance *art);
	void initExp();
	void initArmy(IArmyDescriptor *dst = nullptr);
	//void giveArtifact (ui32 aid);
	void pushPrimSkill(PrimarySkill::PrimarySkill which, int val);
	ui8 maxlevelsToMagicSchool() const;
	ui8 maxlevelsToWisdom() const;
	void Updatespecialty();
	void recreateSecondarySkillsBonuses();
	void updateSkill(SecondarySkill which, int val);

	CGHeroInstance();
	virtual ~CGHeroInstance();
	//////////////////////////////////////////////////////////////////////////
	//
	ArtBearer::ArtBearer bearerType() const override;
	//////////////////////////////////////////////////////////////////////////

	CBonusSystemNode *whereShouldBeAttached(CGameState *gs) override;
	std::string nodeName() const override;
	void deserializationFix();

	void initObj() override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	const std::string & getHoverText() const override;
protected:
	void setPropertyDer(ui8 what, ui32 val) override;//synchr

private:
	void levelUpAutomatically();

public:
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & static_cast<CArtifactSet&>(*this);
		h & exp & level & name & biography & portrait & mana & secSkills & movement
			& sex & inTownGarrison & spells & patrol & moveDir & skillsInfo;
		h & visitedTown & boat;
		h & type & specialty & commander;
		BONUS_TREE_DESERIALIZATION_FIX
		//visitied town pointer will be restored by map serialization method
	}
};

class DLL_LINKAGE CSpecObjInfo
{
public:
	virtual ~CSpecObjInfo(){};
	PlayerColor player; //owner
};

class DLL_LINKAGE CCreGenAsCastleInfo : public virtual CSpecObjInfo
{
public:
	bool asCastle;
	ui32 identifier;
	ui8 castles[2]; //allowed castles
};

class DLL_LINKAGE CCreGenLeveledInfo : public virtual CSpecObjInfo
{
public:
	ui8 minLevel, maxLevel; //minimal and maximal level of creature in dwelling: <0, 6>
};

class DLL_LINKAGE CCreGenLeveledCastleInfo : public CCreGenAsCastleInfo, public CCreGenLeveledInfo
{
};

class DLL_LINKAGE CGDwelling : public CArmedInstance
{
public:
	typedef std::vector<std::pair<ui32, std::vector<CreatureID> > > TCreaturesSet;

	CSpecObjInfo * info; //h3m info about dewlling
	TCreaturesSet creatures; //creatures[level] -> <vector of alternative ids (base creature and upgrades, creatures amount>

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this) & creatures;
	}

	void initObj() override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void newTurn() const override;
	void setProperty(ui8 what, ui32 val) override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

private:
	void heroAcceptsCreatures(const CGHeroInstance *h) const;
};


class DLL_LINKAGE CGVisitableOPH : public CGObjectInstance //objects visitable only once per hero
{
public:
	std::set<ObjectInstanceID> visitors; //ids of heroes who have visited this obj
	TResources treePrice; //used only by trees of knowledge: empty, 2000 gold, 10 gems

	const std::string & getHoverText() const override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	bool wasVisited (const CGHeroInstance * h) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & visitors & treePrice;
	}
protected:
	void setPropertyDer(ui8 what, ui32 val) override;//synchr
private:
	void onNAHeroVisit(const CGHeroInstance * h, bool alreadyVisited) const;
	///dialog callbacks
	void treeSelected(const CGHeroInstance * h, ui32 result) const;
	void schoolSelected(const CGHeroInstance * h, ui32 which) const;
	void arenaSelected(const CGHeroInstance * h, int primSkill) const;
};
class DLL_LINKAGE CGTownBuilding : public IObjectInterface
{
///basic class for town structures handled as map objects
public:
	BuildingID ID; //from buildig list
	si32 id; //identifies its index on towns vector
	CGTownInstance *town;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & id;
	}
};
class DLL_LINKAGE COPWBonus : public CGTownBuilding
{///used for OPW bonusing structures
public:
	std::set<si32> visitors;
	void setProperty(ui8 what, ui32 val) override;
	void onHeroVisit (const CGHeroInstance * h) const override;

	COPWBonus (BuildingID index, CGTownInstance *TOWN);
	COPWBonus (){ID = BuildingID::NONE; town = nullptr;};
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGTownBuilding&>(*this);
		h & visitors;
	}
};

class DLL_LINKAGE CTownBonus : public CGTownBuilding
{
///used for one-time bonusing structures
///feel free to merge inheritance tree
public:
	std::set<ObjectInstanceID> visitors;
	void setProperty(ui8 what, ui32 val) override;
	void onHeroVisit (const CGHeroInstance * h) const override;

	CTownBonus (BuildingID index, CGTownInstance *TOWN);
	CTownBonus (){ID = BuildingID::NONE; town = nullptr;};
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGTownBuilding&>(*this);
		h & visitors;
	}
};

class DLL_LINKAGE CTownAndVisitingHero : public CBonusSystemNode
{
public:
	CTownAndVisitingHero();
};

struct DLL_LINKAGE GrowthInfo
{
	struct Entry
	{
		int count;
		std::string description;
		Entry(const std::string &format, int _count);
		Entry(int subID, BuildingID building, int _count);
	};

	std::vector<Entry> entries;
	int totalGrowth() const;
};

class DLL_LINKAGE CGTownInstance : public CGDwelling, public IShipyard, public IMarket
{
public:
	enum EFortLevel {NONE = 0, FORT = 1, CITADEL = 2, CASTLE = 3};

	CTownAndVisitingHero townAndVis;
	const CTown * town;
	std::string name; // name of town
	si32 builded; //how many buildings has been built this turn
	si32 destroyed; //how many buildings has been destroyed this turn
	ConstTransitivePtr<CGHeroInstance> garrisonHero, visitingHero;
	ui32 identifier; //special identifier from h3m (only > RoE maps)
	si32 alignment;
	std::set<BuildingID> forbiddenBuildings, builtBuildings;
	std::vector<CGTownBuilding*> bonusingBuildings;
	std::vector<SpellID> possibleSpells, obligatorySpells;
	std::vector<std::vector<SpellID> > spells; //spells[level] -> vector of spells, first will be available in guild
	std::list<CCastleEvent> events;
	std::pair<si32, si32> bonusValue;//var to store town bonuses (rampart = resources from mystic pond);

	//////////////////////////////////////////////////////////////////////////
	static std::vector<const CArtifact *> merchantArtifacts; //vector of artifacts available at Artifact merchant, NULLs possible (for making empty space when artifact is bought)
	static std::vector<int> universitySkills;//skills for university of magic

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGDwelling&>(*this);
		h & static_cast<IShipyard&>(*this);
		h & static_cast<IMarket&>(*this);
		h & name & builded & destroyed & identifier;
		h & garrisonHero & visitingHero;
		h & alignment & forbiddenBuildings & builtBuildings & bonusValue
			& possibleSpells & obligatorySpells & spells & /*strInfo & */events & bonusingBuildings;

		for (std::vector<CGTownBuilding*>::iterator i = bonusingBuildings.begin(); i!=bonusingBuildings.end(); i++)
			(*i)->town = this;

		h & town & townAndVis;
		BONUS_TREE_DESERIALIZATION_FIX

		vstd::erase_if(builtBuildings, [this](BuildingID building) -> bool
		{
			if(!town->buildings.count(building) ||  !town->buildings.at(building))
			{
				logGlobal->errorStream() << boost::format("#1444-like issue in CGTownInstance::serialize. From town %s at %s removing the bogus builtBuildings item %s")
					% name % pos % building;
				return true;
			}
			return false;
		});
	}
	//////////////////////////////////////////////////////////////////////////

	CBonusSystemNode *whatShouldBeAttached() override;
	std::string nodeName() const override;
	void updateMoraleBonusFromArmy() override;
	void deserializationFix();
	void recreateBuildingsBonuses();
	bool addBonusIfBuilt(BuildingID building, Bonus::BonusType type, int val, TPropagatorPtr &prop, int subtype = -1); //returns true if building is built and bonus has been added
	bool addBonusIfBuilt(BuildingID building, Bonus::BonusType type, int val, int subtype = -1); //convienence version of above
	void setVisitingHero(CGHeroInstance *h);
	void setGarrisonedHero(CGHeroInstance *h);
	const CArmedInstance *getUpperArmy() const; //garrisoned hero if present or the town itself

	//////////////////////////////////////////////////////////////////////////

	ui8 getPassableness() const; //bitmap - if the bit is set the corresponding player can pass through the visitable tiles of object, even if it's blockvis; if not set - default properties from definfo are used
	int3 getSightCenter() const override; //"center" tile from which the sight distance is calculated
	int getSightRadious() const override; //returns sight distance
	std::string getBoatAnimationName() const;
	int getBoatType() const;
	void getOutOffsets(std::vector<int3> &offsets) const; //offsets to obj pos when we boat can be placed
	int getMarketEfficiency() const override; //=market count
	bool allowsTrade(EMarketMode::EMarketMode mode) const;
	std::vector<int> availableItemsIds(EMarketMode::EMarketMode mode) const;

	void updateAppearance();

	//////////////////////////////////////////////////////////////////////////

	bool needsLastStack() const;
	CGTownInstance::EFortLevel fortLevel() const;
	int hallLevel() const; // -1 - none, 0 - village, 1 - town, 2 - city, 3 - capitol
	int mageGuildLevel() const; // -1 - none, 0 - village, 1 - town, 2 - city, 3 - capitol
	int getHordeLevel(const int & HID) const; //HID - 0 or 1; returns creature level or -1 if that horde structure is not present
	int creatureGrowth(const int & level) const;
	GrowthInfo getGrowthInfo(int level) const;
	bool hasFort() const;
	bool hasCapitol() const;
	//checks if building is constructed and town has same subID
	bool hasBuilt(BuildingID buildingID) const;
	bool hasBuilt(BuildingID buildingID, int townID) const;
	TResources dailyIncome() const; //calculates daily income of this town
	int spellsAtLevel(int level, bool checkGuild) const; //levels are counted from 1 (1 - 5)
	bool armedGarrison() const; //true if town has creatures in garrison or garrisoned hero
	int getTownLevel() const;

	void removeCapitols (PlayerColor owner) const;
	void addHeroToStructureVisitors(const CGHeroInstance *h, si32 structureInstanceID) const; //hero must be visiting or garrisoned in town

	CGTownInstance();
	virtual ~CGTownInstance();

	///IObjectInterface overrides
	void newTurn() const override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void onHeroLeave(const CGHeroInstance * h) const override;
	void initObj() override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
protected:
	void setPropertyDer(ui8 what, ui32 val) override;
};
class DLL_LINKAGE CGPandoraBox : public CArmedInstance
{
public:
	std::string message;
	bool hasGuardians; //helper - after battle even though we have no stacks, allows us to know that there was battle

	//gained things:
	ui32 gainedExp;
	si32 manaDiff; //amount of gained / lost mana
	si32 moraleDiff; //morale modifier
	si32 luckDiff; //luck modifier
	TResources resources;//gained / lost resources
	std::vector<si32> primskills;//gained / lost prim skills
	std::vector<SecondarySkill> abilities; //gained abilities
	std::vector<si32> abilityLevels; //levels of gained abilities
	std::vector<ArtifactID> artifacts; //gained artifacts
	std::vector<SpellID> spells; //gained spells
	CCreatureSet creatures; //gained creatures

	void initObj() override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;
	void heroLevelUpDone(const CGHeroInstance *hero) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & message & hasGuardians & gainedExp & manaDiff & moraleDiff & luckDiff & resources & primskills
			& abilities & abilityLevels & artifacts & spells & creatures;
	}
protected:
	void giveContentsUpToExp(const CGHeroInstance *h) const;
	void giveContentsAfterExp(const CGHeroInstance *h) const;
private:
	void getText( InfoWindow &iw, bool &afterBattle, int val, int negative, int positive, const CGHeroInstance * h ) const;
	void getText( InfoWindow &iw, bool &afterBattle, int text, const CGHeroInstance * h ) const;
};

class DLL_LINKAGE CGEvent : public CGPandoraBox  //event objects
{
public:
	bool removeAfterVisit; //true if event is removed after occurring
	ui8 availableFor; //players whom this event is available for
	bool computerActivate; //true if computer player can activate this event
	bool humanActivate; //true if human player can activate this event

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGPandoraBox &>(*this);
		h & removeAfterVisit & availableFor & computerActivate & humanActivate;
	}

	void onHeroVisit(const CGHeroInstance * h) const override;
private:
	void activated(const CGHeroInstance * h) const;
};

class DLL_LINKAGE CGCreature : public CArmedInstance //creatures on map
{
	enum Action {
		FIGHT = -2, FLEE = -1, JOIN_FOR_FREE = 0 //values > 0 mean gold price
	};

public:
	ui32 identifier; //unique code for this monster (used in missions)
	si8 character; //character of this set of creatures (0 - the most friendly, 4 - the most hostile) => on init changed to -4 (compliant) ... 10 value (savage)
	std::string message; //message printed for attacking hero
	TResources resources; // resources given to hero that has won with monsters
	ArtifactID gainedArtifact; //ID of artifact gained to hero, -1 if none
	bool neverFlees; //if true, the troops will never flee
	bool notGrowingTeam; //if true, number of units won't grow
	ui64 temppower; //used to handle fractional stack growth for tiny stacks

	bool refusedJoining;

	void onHeroVisit(const CGHeroInstance * h) const override;
	const std::string & getHoverText() const override;
	void initObj() override;
	void newTurn() const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;


	struct DLL_LINKAGE formationInfo // info about merging stacks after battle back into one
	{
		si32 basicType;
		ui32 randomFormation; //random seed used to determine number of stacks and is there's upgraded stack
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & basicType & randomFormation;
		}
	} formation;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & identifier & character & message & resources & gainedArtifact & neverFlees & notGrowingTeam & temppower;
		h & refusedJoining & formation;
	}
protected:
	void setPropertyDer(ui8 what, ui32 val) override;
private:

	void fight(const CGHeroInstance *h) const;
	void flee( const CGHeroInstance * h ) const;
	void fleeDecision(const CGHeroInstance *h, ui32 pursue) const;
	void joinDecision(const CGHeroInstance *h, int cost, ui32 accept) const;

	int takenAction(const CGHeroInstance *h, bool allowJoin=true) const; //action on confrontation: -2 - fight, -1 - flee, >=0 - will join for given value of gold (may be 0)

};


class DLL_LINKAGE CGSignBottle : public CGObjectInstance //signs and ocean bottles
{
public:
	std::string message;

	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & message;
	}
};

class DLL_LINKAGE IQuestObject
{
public:
	CQuest * quest;

	IQuestObject(): quest(new CQuest()){};
	virtual ~IQuestObject() {};
	virtual void getVisitText (MetaString &text, std::vector<Component> &components, bool isCustom, bool FirstVisit, const CGHeroInstance * h = nullptr) const;
	virtual bool checkQuest (const CGHeroInstance * h) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & quest;
	}
};

class DLL_LINKAGE CGSeerHut : public CArmedInstance, public IQuestObject //army is used when giving reward
{
public:
	enum ERewardType {NOTHING, EXPERIENCE, MANA_POINTS, MORALE_BONUS, LUCK_BONUS, RESOURCES, PRIMARY_SKILL, SECONDARY_SKILL, ARTIFACT, SPELL, CREATURE};
	ERewardType rewardType;
	si32 rID; //reward ID
	si32 rVal; //reward value
	std::string seerName;

	CGSeerHut() : IQuestObject(){};
	void initObj() override;
	const std::string & getHoverText() const override;
	void newTurn() const override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	virtual void init();
	int checkDirection() const; //calculates the region of map where monster is placed
	void setObjToKill(); //remember creatures / heroes to kill after they are initialized
	const CGHeroInstance *getHeroToKill(bool allowNull = false) const;
	const CGCreature *getCreatureToKill(bool allowNull = false) const;
	void getRolloverText (MetaString &text, bool onHover) const;
	void getCompletionText(MetaString &text, std::vector<Component> &components, bool isCustom, const CGHeroInstance * h = nullptr) const;
	void finishQuest (const CGHeroInstance * h, ui32 accept) const; //common for both objects
	virtual void completeQuest (const CGHeroInstance * h) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this) & static_cast<IQuestObject&>(*this);
		h & rewardType & rID & rVal & seerName;
	}
protected:
	void setPropertyDer(ui8 what, ui32 val) override;
};

class DLL_LINKAGE CGQuestGuard : public CGSeerHut
{
public:
	CGQuestGuard() : CGSeerHut(){};
	void init() override;
	void completeQuest (const CGHeroInstance * h) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGSeerHut&>(*this);
	}
};

class DLL_LINKAGE CGWitchHut : public CPlayersVisited
{
public:
	std::vector<si32> allowedAbilities;
	ui32 ability;

	const std::string & getHoverText() const override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPlayersVisited&>(*this);
		h & allowedAbilities & ability;
	}
};


class DLL_LINKAGE CGScholar : public CGObjectInstance
{
public:
	enum EBonusType {PRIM_SKILL, SECONDARY_SKILL, SPELL, RANDOM = 255};
	EBonusType bonusType;
	ui16 bonusID; //ID of skill/spell

//	void giveAnyBonus(const CGHeroInstance * h) const; //TODO: remove
	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & bonusType & bonusID;
	}
};

class DLL_LINKAGE CGGarrison : public CArmedInstance
{
public:
	bool removableUnits;

	ui8 getPassableness() const;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & removableUnits;
	}
};

class DLL_LINKAGE CGArtifact : public CArmedInstance
{
public:
	CArtifactInstance *storedArtifact;
	std::string message;

	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	void pick( const CGHeroInstance * h ) const;
	void initObj() override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & message & storedArtifact;
	}
};

class DLL_LINKAGE CGResource : public CArmedInstance
{
public:
	ui32 amount; //0 if random
	std::string message;

	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	void collectRes(PlayerColor player) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & amount & message;
	}
};

class DLL_LINKAGE CGPickable : public CGObjectInstance //campfire, treasure chest, Flotsam, Shipwreck Survivor, Sea Chest
{
public:
	ui32 type, val1, val2;

	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & type & val1 & val2;
	}
};

class DLL_LINKAGE CGShrine : public CPlayersVisited
{
public:
	SpellID spell; //id of spell or NONE if random
	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	const std::string & getHoverText() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPlayersVisited&>(*this);;
		h & spell;
	}
};

class DLL_LINKAGE CGMine : public CArmedInstance
{
public:
	Res::ERes producedResource;
	ui32 producedQuantity;
	
	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	void flagMine(PlayerColor player) const;
	void newTurn() const override;
	void initObj() override;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & producedResource & producedQuantity;
	}
	ui32 defaultResProduction();
};

class DLL_LINKAGE CGVisitableOPW : public CGObjectInstance //objects visitable OPW
{
public:
	ui8 visited; //true if object has been visited this week

	bool wasVisited(PlayerColor player) const;
	void onHeroVisit(const CGHeroInstance * h) const override;
	virtual void newTurn() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & visited;
	}
protected:
	void setPropertyDer(ui8 what, ui32 val) override;
};

class DLL_LINKAGE CGTeleport : public CGObjectInstance //teleports and subterranean gates
{
public:
	static std::map<Obj, std::map<int, std::vector<ObjectInstanceID> > > objs; //teleports: map[ID][subID] => vector of ids
	static std::vector<std::pair<ObjectInstanceID, ObjectInstanceID> > gates; //subterranean gates: pairs of ids
	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	static void postInit();
	static ObjectInstanceID getMatchingGate(ObjectInstanceID id); //receives id of one subterranean gate and returns id of the paired one, -1 if none

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGBonusingObject : public CGObjectInstance //objects giving bonuses to luck/morale/movement
{
public:
	bool wasVisited (const CGHeroInstance * h) const;
	void onHeroVisit(const CGHeroInstance * h) const override;
	const std::string & getHoverText() const override;
	void initObj() override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGMagicSpring : public CGVisitableOPW
{///unfortunately, this one is quite different than others
	enum EVisitedEntrance
	{
		CLEAR = 0, LEFT = 1, RIGHT
	};
public:
	EVisitedEntrance visitedTile; //only one entrance was visited - there are two

	std::vector<int3> getVisitableOffsets() const;
	int3 getVisitableOffset() const override;
	void setPropertyDer(ui8 what, ui32 val) override;
	void newTurn() const override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	const std::string & getHoverText() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & visitedTile & visited;
	}
};

class DLL_LINKAGE CGMagicWell : public CGObjectInstance //objects giving bonuses to luck/morale/movement
{
public:
	void onHeroVisit(const CGHeroInstance * h) const override;
	const std::string & getHoverText() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGSirens : public CGObjectInstance
{
public:
	void onHeroVisit(const CGHeroInstance * h) const override;
	const std::string & getHoverText() const override;
	void initObj() override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGObservatory : public CGObjectInstance //Redwood observatory
{
public:
	void onHeroVisit(const CGHeroInstance * h) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};


class DLL_LINKAGE CGKeys : public CGObjectInstance //Base class for Keymaster and guards
{
public:
	static std::map <PlayerColor, std::set <ui8> > playerKeyMap; //[players][keysowned]
	//SubID 0 - lightblue, 1 - green, 2 - red, 3 - darkblue, 4 - brown, 5 - purple, 6 - white, 7 - black

	const std::string getName() const; //depending on color
	bool wasMyColorVisited (PlayerColor player) const;

	const std::string & getHoverText() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
protected:
	void setPropertyDer(ui8 what, ui32 val) override;
};

class DLL_LINKAGE CGKeymasterTent : public CGKeys
{
public:
	bool wasVisited (PlayerColor player) const;
	void onHeroVisit(const CGHeroInstance * h) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};

class DLL_LINKAGE CGBorderGuard : public CGKeys, public IQuestObject
{
public:
	CGBorderGuard() : IQuestObject(){};
	void initObj() override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	void getVisitText (MetaString &text, std::vector<Component> &components, bool isCustom, bool FirstVisit, const CGHeroInstance * h = nullptr) const;
	void getRolloverText (MetaString &text, bool onHover) const;
	bool checkQuest (const CGHeroInstance * h) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<IQuestObject&>(*this);
		h & static_cast<CGObjectInstance&>(*this);
		h & blockVisit;
	}
};

class DLL_LINKAGE CGBorderGate : public CGBorderGuard
{
public:
	CGBorderGate() : CGBorderGuard(){};
	void onHeroVisit(const CGHeroInstance * h) const override;

	ui8 getPassableness() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGBorderGuard&>(*this); //need to serialize or object will be empty
	}
};

class DLL_LINKAGE CGBoat : public CGObjectInstance
{
public:
	ui8 direction;
	const CGHeroInstance *hero;  //hero on board

	void initObj() override;

	CGBoat()
	{
		hero = nullptr;
		direction = 4;
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this) & direction & hero;
	}
};

class DLL_LINKAGE CGOnceVisitable : public CPlayersVisited
///wagon, corpse, lean to, warriors tomb
{
public:
	ui8 artOrRes; //0 - nothing; 1 - artifact; 2 - resource
	ui32 bonusType, //id of res or artifact
		bonusVal; //resource amount (or not used)

	void onHeroVisit(const CGHeroInstance * h) const override;
	const std::string & getHoverText() const override;
	void initObj() override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPlayersVisited&>(*this);;
		h & artOrRes & bonusType & bonusVal;
	}
};

class DLL_LINKAGE CBank : public CArmedInstance
{
	public:
	int index; //banks have unusal numbering - see ZCRBANK.txt and initObj()
	BankConfig *bc;
	double multiplier; //for improved banks script
	std::vector<ui32> artifacts; //fixed and deterministic
	ui32 daycounter;

	void initObj() override;
	const std::string & getHoverText() const override;
	void initialize() const;
	void reset(ui16 var1);
	void newTurn() const override;
	bool wasVisited (PlayerColor player) const override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CArmedInstance&>(*this);
		h & index & multiplier & artifacts & daycounter & bc;
	}
protected:
	void setPropertyDer(ui8 what, ui32 val) override;
};
class DLL_LINKAGE CGPyramid : public CBank
{
public:
	ui16 spell;

	void initObj() override;
	const std::string & getHoverText() const override;
	void newTurn() const override {}; //empty, no reset
	void onHeroVisit(const CGHeroInstance * h) const override;
	void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const override;

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
	void onHeroVisit(const CGHeroInstance * h) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & static_cast<IShipyard&>(*this);
	}
};

class DLL_LINKAGE CGMagi : public CGObjectInstance
{
public:
	static std::map <si32, std::vector<ObjectInstanceID> > eyelist; //[subID][id], supports multiple sets as in H5

	void initObj() override;
	void onHeroVisit(const CGHeroInstance * h) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
};



class DLL_LINKAGE CCartographer : public CPlayersVisited
{
///behaviour varies depending on surface and  floor
public:
	void onHeroVisit(const CGHeroInstance * h) const override;
	void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPlayersVisited&>(*this);
	}
};

class DLL_LINKAGE CGDenOfthieves : public CGObjectInstance
{
	void onHeroVisit(const CGHeroInstance * h) const override;
};

class DLL_LINKAGE CGObelisk : public CPlayersVisited
{
public:
	static ui8 obeliskCount; //how many obelisks are on map
	static std::map<TeamID, ui8> visited; //map: team_id => how many obelisks has been visited

	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	const std::string & getHoverText() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPlayersVisited&>(*this);
	}
protected:
	void setPropertyDer(ui8 what, ui32 val) override;
};

class DLL_LINKAGE CGLighthouse : public CGObjectInstance
{
public:
	void onHeroVisit(const CGHeroInstance * h) const override;
	void initObj() override;
	const std::string & getHoverText() const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
	}
	void giveBonusTo( PlayerColor player ) const;
};

class DLL_LINKAGE CGMarket : public CGObjectInstance, public IMarket
{
public:
	CGMarket();
	///IObjectIntercae
	void onHeroVisit(const CGHeroInstance * h) const override; //open trading window

	///IMarket
	int getMarketEfficiency() const override;
	bool allowsTrade(EMarketMode::EMarketMode mode) const override;
	int availableUnits(EMarketMode::EMarketMode mode, int marketItemSerial) const override; //-1 if unlimited
	std::vector<int> availableItemsIds(EMarketMode::EMarketMode mode) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGObjectInstance&>(*this);
		h & static_cast<IMarket&>(*this);
	}
};

class DLL_LINKAGE CGBlackMarket : public CGMarket
{
public:
	std::vector<const CArtifact *> artifacts; //available artifacts

	void newTurn() const override; //reset artifacts for black market every month
	std::vector<int> availableItemsIds(EMarketMode::EMarketMode mode) const override;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGMarket&>(*this);
		h & artifacts;
	}
};

class DLL_LINKAGE CGUniversity : public CGMarket
{
public:
	std::vector<int> skills; //available skills

	std::vector<int> availableItemsIds(EMarketMode::EMarketMode mode) const;
	void initObj() override;//set skills for trade
	void onHeroVisit(const CGHeroInstance * h) const override; //open window

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CGMarket&>(*this);
		h & skills;
	}
};

struct BankConfig
{
	BankConfig() {level = chance = upgradeChance = combatValue = value = rewardDifficulty = easiest = 0; };
	ui8 level; //1 - 4, how hard the battle will be
	ui8 chance; //chance for this level being chosen
	ui8 upgradeChance; //chance for creatures to be in upgraded versions
	std::vector< std::pair <CreatureID, ui32> > guards; //creature ID, amount
	ui32 combatValue; //how hard are guards of this level
	Res::ResourceSet resources; //resources given in case of victory
	std::vector< std::pair <CreatureID, ui32> > creatures; //creatures granted in case of victory (creature ID, amount)
	std::vector<ui16> artifacts; //number of artifacts given in case of victory [0] -> treasure, [1] -> minor [2] -> major [3] -> relic
	ui32 value; //overall value of given things
	ui32 rewardDifficulty; //proportion of reward value to difficulty of guards; how profitable is this creature Bank config
	ui16 easiest; //?!?

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & level & chance & upgradeChance & guards & combatValue & resources & creatures & artifacts & value & rewardDifficulty & easiest;
	}
};

class DLL_LINKAGE CObjectHandler
{
public:
	std::map<si32, CreatureID> cregens; //type 17. dwelling subid -> creature ID
	std::map <ui32, std::vector < ConstTransitivePtr<BankConfig> > > banksInfo; //[index][preset]
	std::map <ui32, std::string> creBanksNames; //[crebank index] -> name of this creature bank
	std::vector<ui32> resVals; //default values of resources in gold

	CObjectHandler();
	~CObjectHandler();

	int bankObjToIndex (const CGObjectInstance * obj);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & cregens & banksInfo & creBanksNames & resVals;
	}
};
