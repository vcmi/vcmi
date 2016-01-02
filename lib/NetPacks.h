#pragma once

#include "NetPacksBase.h"

#include "BattleAction.h"
//#include "HeroBonus.h"
#include "mapObjects/CGHeroInstance.h"
//#include "CCreatureSet.h"
//#include "mapping/CMapInfo.h"
//#include "StartInfo.h"
#include "ConstTransitivePtr.h"
#include "int3.h"
#include "ResourceSet.h"
//#include "CObstacleInstance.h"
#include "CGameStateFwd.h"
#include "mapping/CMapDefines.h"
#include "CObstacleInstance.h"

#include "spells/ViewSpellInt.h"

/*
 * NetPacks.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CClient;
class CGameState;
class CGameHandler;
class CConnection;
class CCampaignState;
class CArtifact;
class CSelectionScreen;
class CGObjectInstance;
class CArtifactInstance;
//class CMapInfo;
struct StackLocation;
struct ArtSlotInfo;
struct QuestInfo;
class CMapInfo;
class StartInfo;


struct CPackForClient : public CPack
{
	CPackForClient(){type = 1;};

	CGameState* GS(CClient *cl);
	void applyFirstCl(CClient *cl)//called before applying to gs
	{}
	void applyCl(CClient *cl)//called after applying to gs
	{}
};

struct CPackForServer : public CPack
{
	PlayerColor player;
	CConnection *c;
	CGameState* GS(CGameHandler *gh);
	CPackForServer():
		player(PlayerColor::NEUTRAL),
		c(nullptr)
	{
		type = 2;
	}

	bool applyGh(CGameHandler *gh) //called after applying to gs
	{
		logGlobal->errorStream() << "Should not happen... applying plain CPackForServer";
		return false;
	}
};


struct Query : public CPackForClient
{
	QueryID queryID; // equals to -1 if it is not an actual query (and should not be answered)

	Query()
	{
	}
};



struct StackLocation
{
	ConstTransitivePtr<CArmedInstance> army;
	SlotID slot;

	StackLocation()
	{}
	StackLocation(const CArmedInstance *Army, SlotID Slot):
		army(const_cast<CArmedInstance*>(Army)), //we are allowed here to const cast -> change will go through one of our packages... do not abuse!
		slot(Slot)
	{
	}

	DLL_LINKAGE const CStackInstance *getStack();
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & army & slot;
	}
};

/***********************************************************************************************************/


struct PackageApplied : public CPackForClient //94
{
	PackageApplied() {type = 94;}
	PackageApplied(ui8 Result) : result(Result) {type = 94;}
	void applyCl(CClient *cl);

	ui8 result; //0 - something went wrong, request hasn't been realized; 1 - OK
	ui32 packType; //type id of applied package
	ui32 requestID; //an ID given by client to the request that was applied
	PlayerColor player;


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & result & packType & requestID & player;
	}
};

struct SystemMessage : public CPackForClient //95
{
	SystemMessage(const std::string & Text) : text(Text){type = 95;};
	SystemMessage(){type = 95;};
	void applyCl(CClient *cl);

	std::string text;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & text;
	}
};

struct PlayerBlocked : public CPackForClient //96
{
	PlayerBlocked(){type = 96;};
	void applyCl(CClient *cl);

	enum EReason { UPCOMING_BATTLE, ONGOING_MOVEMENT };
	enum EMode { BLOCKADE_STARTED, BLOCKADE_ENDED };
	
	EReason reason;
	EMode startOrEnd;
	PlayerColor player;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & reason & startOrEnd & player;
	}
};

struct YourTurn : public CPackForClient //100
{
	YourTurn(){type = 100;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	PlayerColor player;
	boost::optional<ui8> daysWithoutCastle;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player & daysWithoutCastle;
	}
};

struct SetResource : public CPackForClient //102
{
	SetResource(){type = 102;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	PlayerColor player;
	Res::ERes resid;
	TResourceCap val;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player & resid & val;
	}
};
 struct SetResources : public CPackForClient //104
 {
 	SetResources(){type = 104;};
 	void applyCl(CClient *cl);
 	DLL_LINKAGE void applyGs(CGameState *gs);

 	PlayerColor player;
 	TResources res; //res[resid] => res amount

 	template <typename Handler> void serialize(Handler &h, const int version)
 	{
 		h & player & res;
 	}
 };

struct SetPrimSkill : public CPackForClient //105
{
	SetPrimSkill(){type = 105;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 abs; //0 - changes by value; 1 - sets to value
	ObjectInstanceID id;
	PrimarySkill::PrimarySkill which;
	si64 val;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & abs & id & which & val;
	}
};
struct SetSecSkill : public CPackForClient //106
{
	SetSecSkill(){type = 106;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 abs; //0 - changes by value; 1 - sets to value
	ObjectInstanceID id;
	SecondarySkill which;
	ui16 val;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & abs & id & which & val;
	}
};
struct HeroVisitCastle : public CPackForClient //108
{
	HeroVisitCastle(){flags=0;type = 108;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 flags; //1 - start
	ObjectInstanceID tid, hid;

	bool start() //if hero is entering castle (if false - leaving)
	{
		return flags & 1;
	}
// 	bool garrison() //if hero is entering/leaving garrison (if false - it's only visiting hero)
// 	{
// 		return flags & 2;
// 	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & flags & tid & hid;
	}
};
struct ChangeSpells : public CPackForClient //109
{
	ChangeSpells(){type = 109;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 learn; //1 - gives spell, 0 - takes
	ObjectInstanceID hid;
	std::set<SpellID> spells;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & learn & hid & spells;
	}
};

struct SetMana : public CPackForClient //110
{
	SetMana(){type = 110;absolute=true;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);


	ObjectInstanceID hid;
	si32 val;
	bool absolute;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & val & hid & absolute;
	}
};

struct SetMovePoints : public CPackForClient //111
{
	SetMovePoints(){type = 111;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID hid;
	si32 val;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & val & hid;
	}
};

struct FoWChange : public CPackForClient //112
{
	FoWChange(){type = 112;waitForDialogs = false;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	std::unordered_set<int3, struct ShashInt3 > tiles;
	PlayerColor player;
	ui8 mode; //mode==0 - hide, mode==1 - reveal
	bool waitForDialogs;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tiles & player & mode & waitForDialogs;
	}
};

struct SetAvailableHeroes : public CPackForClient //113
{
	SetAvailableHeroes()
	{
		type = 113;
		for (int i = 0; i < GameConstants::AVAILABLE_HEROES_PER_PLAYER; i++)
			army[i].clear();
	}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	PlayerColor player;
	si32 hid[GameConstants::AVAILABLE_HEROES_PER_PLAYER]; //-1 if no hero
	CSimpleArmy army[GameConstants::AVAILABLE_HEROES_PER_PLAYER];
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player & hid & army;
	}
};

struct GiveBonus :  public CPackForClient //115
{
	GiveBonus(ui8 Who = 0)
	{
		who = Who;
		type = 115;
	}

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	enum {HERO, PLAYER, TOWN};
	ui8 who; //who receives bonus, uses enum above
	si32 id; //hero. town or player id - whoever receives it
	Bonus bonus;
	MetaString bdescr;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & bonus & id & bdescr & who;
		assert( id != -1);
	}
};

struct ChangeObjPos : public CPackForClient //116
{
	ChangeObjPos()
	{
		type = 116;
		flags = 0;
	}
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID objid;
	int3 nPos;
	ui8 flags; //bit flags: 1 - redraw

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objid & nPos & flags;
	}
};

struct PlayerEndsGame : public CPackForClient //117
{
	PlayerEndsGame()
	{
		type = 117;
	}

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	PlayerColor player;
	EVictoryLossCheckResult victoryLossCheckResult;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player & victoryLossCheckResult;
	}
};


struct RemoveBonus :  public CPackForClient //118
{
	RemoveBonus(ui8 Who = 0)
	{
		who = Who;
		type = 118;
	}

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	enum {HERO, PLAYER, TOWN};
	ui8 who; //who receives bonus, uses enum above
	ui32 whoID; //hero, town or player id - whoever loses bonus

	//vars to identify bonus: its source
	ui8 source;
	ui32 id; //source id

	//used locally: copy of removed bonus
	Bonus bonus;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & source & id & who & whoID;
	}
};

struct UpdateCampaignState : public CPackForClient //119
{
	UpdateCampaignState()
	{
		type = 119;
	}

	std::shared_ptr<CCampaignState> camp;
	void applyCl(CClient *cl);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & camp;
	}
};
struct SetCommanderProperty : public CPackForClient //120
{
	enum ECommanderProperty {ALIVE, BONUS, SECONDARY_SKILL, EXPERIENCE, SPECIAL_SKILL};

	SetCommanderProperty(){type = 120;};
	void applyCl(CClient *cl){};
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID heroid; //for commander attached to hero
	StackLocation sl; //for commander not on the hero?

	ECommanderProperty which;
	TExpType amount; //0 for dead, >0 for alive
	si32 additionalInfo; //for secondary skills choice
	Bonus accumulatedBonus;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroid & sl & which & amount & additionalInfo & accumulatedBonus;
	}
};
struct AddQuest : public CPackForClient //121
{
	AddQuest(){type = 121;};
	void applyCl(CClient *cl){};
	DLL_LINKAGE void applyGs(CGameState *gs);

	PlayerColor player;
	QuestInfo quest;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player & quest;
	}
};

struct PrepareForAdvancingCampaign : public CPackForClient //122
{
	PrepareForAdvancingCampaign() {type = 122;}

	void applyCl(CClient *cl);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
	}
};

struct UpdateArtHandlerLists : public CPackForClient //123
{
	UpdateArtHandlerLists(){type = 123;};
	std::vector<CArtifact*> treasures, minors, majors, relics;

	DLL_LINKAGE void applyGs(CGameState *gs);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & treasures & minors & majors & relics;
	}
};

struct UpdateMapEvents : public CPackForClient //124
{
	UpdateMapEvents(){type = 124;}

	std::list<CMapEvent> events;
	DLL_LINKAGE void applyGs(CGameState *gs);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & events;
	}
};

struct UpdateCastleEvents : public CPackForClient //125
{
	UpdateCastleEvents(){type = 125;}

	ObjectInstanceID town;
	std::list<CCastleEvent> events;

	DLL_LINKAGE void applyGs(CGameState *gs);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & town & events;
	}
};

struct RemoveObject : public CPackForClient //500
{
	RemoveObject(){type = 500;};
	RemoveObject(ObjectInstanceID ID){id = ID;type = 500;};
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID id;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id;
	}
};
struct TryMoveHero : public CPackForClient //501
{
	TryMoveHero(){type = 501;humanKnows=false;};
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	void applyGs(CGameState *gs);

	enum EResult
	{
		FAILED, SUCCESS, TELEPORTATION, RESERVED___, BLOCKING_VISIT, EMBARK, DISEMBARK
	};

	ObjectInstanceID id;
	ui32 movePoints;
	EResult result; //uses EResult
	int3 start, end; //h3m format
	std::unordered_set<int3, ShashInt3> fowRevealed; //revealed tiles
	boost::optional<int3> attackedFrom; // Set when stepping into endangered tile.

	bool humanKnows; //used locally during applying to client

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & result & start & end & movePoints & fowRevealed & attackedFrom;
	}
};

// struct SetGarrisons : public CPackForClient //502
// {
// 	SetGarrisons(){type = 502;};
// 	void applyCl(CClient *cl);
// 	DLL_LINKAGE void applyGs(CGameState *gs);
//
// 	std::map<ui32,CCreatureSet> garrs;
//
// 	template <typename Handler> void serialize(Handler &h, const int version)
// 	{
// 		h & garrs;
// 	}
// };

struct NewStructures : public CPackForClient //504
{
	NewStructures(){type = 504;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID tid;
	std::set<BuildingID> bid;
	si16 builded;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid & bid & builded;
	}
};
struct RazeStructures : public CPackForClient //505
{
	RazeStructures() {type = 505;};
	void applyCl (CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID tid;
	std::set<BuildingID> bid;
	si16 destroyed;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid & bid & destroyed;
	}
};
struct SetAvailableCreatures : public CPackForClient //506
{
	SetAvailableCreatures(){type = 506;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID tid;
	std::vector<std::pair<ui32, std::vector<CreatureID> > > creatures;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid & creatures;
	}
};
struct SetHeroesInTown : public CPackForClient //508
{
	SetHeroesInTown(){type = 508;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID tid, visiting, garrison; //id of town, visiting hero, hero in garrison

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid & visiting & garrison;
	}
};

// struct SetHeroArtifacts : public CPackForClient //509
// {
// 	SetHeroArtifacts(){type = 509;};
// 	void applyCl(CClient *cl);
// 	DLL_LINKAGE void applyGs(CGameState *gs);
// 	DLL_LINKAGE void setArtAtPos(ui16 pos, const CArtifact* art);
//
// 	si32 hid;
// 	std::vector<const CArtifact*> artifacts; //hero's artifacts from bag
// 	std::map<ui16, const CArtifact*> artifWorn; //map<position,artifact_id>; positions: 0 - head; 1 - shoulders; 2 - neck; 3 - right hand; 4 - left hand; 5 - torso; 6 - right ring; 7 - left ring; 8 - feet; 9 - misc1; 10 - misc2; 11 - misc3; 12 - misc4; 13 - mach1; 14 - mach2; 15 - mach3; 16 - mach4; 17 - spellbook; 18 - misc5
//
// 	template <typename Handler> void serialize(Handler &h, const int version)
// 	{
// 		h & hid & artifacts & artifWorn;
// 	}
//
// 	std::vector<const CArtifact*> equipped, unequipped; //used locally
// 	BonusList gained, lost; //used locally as hlp when applying
// };

struct HeroRecruited : public CPackForClient //515
{
	HeroRecruited(){type = 515;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	si32 hid;//subID of hero
	ObjectInstanceID tid;
	int3 tile;
	PlayerColor player;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid & tid & tile & player;
	}
};

struct GiveHero : public CPackForClient //516
{
	GiveHero(){type = 516;};
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID id; //object id
	PlayerColor player;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & player;
	}
};

struct OpenWindow : public CPackForClient //517
{
	OpenWindow(){type = 517;};
	void applyCl(CClient *cl);

	enum EWindow {EXCHANGE_WINDOW, RECRUITMENT_FIRST, RECRUITMENT_ALL, SHIPYARD_WINDOW, THIEVES_GUILD,
	              UNIVERSITY_WINDOW, HILL_FORT_WINDOW, MARKET_WINDOW, PUZZLE_MAP, TAVERN_WINDOW};
	ui8 window;
	si32 id1, id2;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & window & id1 & id2;
	}
};

struct NewObject  : public CPackForClient //518
{
	NewObject()
	{
		type = 518;
	}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	Obj ID;
	ui32 subID;
	int3 pos;

	ObjectInstanceID id; //used locally, filled during applyGs

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID & subID & pos;
	}
};

struct SetAvailableArtifacts  : public CPackForClient //519
{
	SetAvailableArtifacts(){type = 519;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	si32 id; //two variants: id < 0: set artifact pool for Artifact Merchants in towns; id >= 0: set pool for adv. map Black Market (id is the id of Black Market instance then)
	std::vector<const CArtifact *> arts;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & arts;
	}
};

struct NewArtifact : public CPackForClient //520
{
	NewArtifact(){type = 520;};
	//void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ConstTransitivePtr<CArtifactInstance> art;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & art;
	}
};

struct CGarrisonOperationPack : CPackForClient
{
};
struct CArtifactOperationPack : CPackForClient
{
};


struct ChangeStackCount : CGarrisonOperationPack  //521
{
	StackLocation sl;
	TQuantity count;
	ui8 absoluteValue; //if not -> count will be added (or subtracted if negative)

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sl & count & absoluteValue;
	}
};

struct SetStackType : CGarrisonOperationPack  //522
{
	StackLocation sl;
	const CCreature *type;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sl & type;
	}
};

struct EraseStack : CGarrisonOperationPack  //523
{
	StackLocation sl;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sl;
	}
};

struct SwapStacks : CGarrisonOperationPack  //524
{
	StackLocation sl1, sl2;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sl1 & sl2;
	}
};

struct InsertNewStack : CGarrisonOperationPack //525
{
	StackLocation sl;
	CStackBasicDescriptor stack;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sl & stack;
	}
};

//moves creatures from src stack to dst slot, may be used for merging/splittint/moving stacks
struct RebalanceStacks : CGarrisonOperationPack  //526
{
	StackLocation src, dst;
	TQuantity count;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & src & dst & count;
	}
};




struct GetEngagedHeroIds : boost::static_visitor<boost::optional<ObjectInstanceID>>
{
	boost::optional<ObjectInstanceID> operator()(const ConstTransitivePtr<CGHeroInstance> &h) const
	{
		return h->id;
	}
	boost::optional<ObjectInstanceID> operator()(const ConstTransitivePtr<CStackInstance> &s) const
	{
		if(s->armyObj && s->armyObj->ID == Obj::HERO)
			return s->armyObj->id;
		return boost::optional<ObjectInstanceID>();
	}
};

struct PutArtifact : CArtifactOperationPack  //526
{
	ArtifactLocation al;
	ConstTransitivePtr<CArtifactInstance> art;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & al & art;
	}
};

struct EraseArtifact : CArtifactOperationPack  //527
{
	ArtifactLocation al;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & al;
	}
};

struct MoveArtifact : CArtifactOperationPack  //528
{
	ArtifactLocation src, dst;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & src & dst;
	}
};

struct AssembledArtifact : CArtifactOperationPack  //529
{
	ArtifactLocation al; //where assembly will be put
	CArtifact *builtArt;
	//std::vector<CArtifactInstance *> constituents;


	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & al & builtArt/* & constituents*/;
	}
};

struct DisassembledArtifact : CArtifactOperationPack  //530
{
	ArtifactLocation al;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & al;
	}
};

struct HeroVisit : CPackForClient //531
{
	const CGHeroInstance *hero;
	const CGObjectInstance *obj;
	PlayerColor player; //if hero was killed during the visit, its color is already reset
	bool starting; //false -> ending

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hero & obj & player & starting;
	}
};

struct NewTurn : public CPackForClient //101
{
	enum weekType {NORMAL, DOUBLE_GROWTH, BONUS_GROWTH, DEITYOFFIRE, PLAGUE, NO_ACTION};

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	struct Hero
	{
		ObjectInstanceID id;
		ui32 move, mana; //id is a general serial id
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & id & move & mana;
		}
		bool operator<(const Hero&h)const{return id < h.id;}
	};

	std::set<Hero> heroes; //updates movement and mana points
	//std::vector<SetResources> res;//resource list
	std::map<PlayerColor, TResources> res; //player ID => resource value[res_id]
	std::map<ObjectInstanceID, SetAvailableCreatures> cres;//creatures to be placed in towns
	ui32 day;
	ui8 specialWeek; //weekType
	CreatureID creatureid; //for creature weeks

	NewTurn(){type = 101;};

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroes & cres & res & day & specialWeek & creatureid;
	}
};

struct InfoWindow : public CPackForClient //103  - displays simple info window
{
	void applyCl(CClient *cl);

	MetaString text;
	std::vector<Component> components;
	PlayerColor player;
	ui16 soundID;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & text & components & player & soundID;
	}
	InfoWindow()
	{
		type = 103;
		soundID = 0;
	}
};

namespace ObjProperty
{
	enum {OWNER = 1, BLOCKVIS = 2, PRIMARY_STACK_COUNT = 3, VISITORS = 4, VISITED = 5, ID = 6, AVAILABLE_CREATURE = 7, SUBID = 8,
		MONSTER_COUNT = 10, MONSTER_POWER = 11, MONSTER_EXP = 12, MONSTER_RESTORE_TYPE = 13, MONSTER_REFUSED_JOIN,
	
		//town-specific
		STRUCTURE_ADD_VISITING_HERO, STRUCTURE_CLEAR_VISITORS, STRUCTURE_ADD_GARRISONED_HERO,  //changing buildings state
		BONUS_VALUE_FIRST, BONUS_VALUE_SECOND, //used in Rampart for special building that generates resources (storing resource type and quantity)

		//creature-bank specific
		BANK_DAYCOUNTER, BANK_RESET, BANK_CLEAR,

		//object with reward
		REWARD_RESET, REWARD_SELECT
	};
}

struct SetObjectProperty : public CPackForClient//1001
{
	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	ObjectInstanceID id;
	ui8 what; // see ObjProperty enum
	ui32 val;
	SetObjectProperty(){type = 1001;};
	SetObjectProperty(ObjectInstanceID ID, ui8 What, ui32 Val):id(ID),what(What),val(Val){type = 1001;};

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id & what & val;
	}
};

struct ChangeObjectVisitors : public CPackForClient // 1003
{
	enum VisitMode
	{
		VISITOR_ADD,    // mark hero as one that have visited this object
		VISITOR_REMOVE, // unmark visitor, reversed to ADD
		VISITOR_CLEAR   // clear all visitors from this object (object reset)
	};
	ui32 mode; // uses VisitMode enum
	ObjectInstanceID object;
	ObjectInstanceID hero; // note: hero owner will be also marked as "visited" this object

	DLL_LINKAGE void applyGs(CGameState *gs);

	ChangeObjectVisitors()
	{ type = 1003; }

	ChangeObjectVisitors(ui32 mode, ObjectInstanceID object, ObjectInstanceID heroID = ObjectInstanceID(-1)):
		mode(mode),
		object(object),
		hero(heroID)
	{ type = 1003; }

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & object & hero & mode;
	}
};

struct HeroLevelUp : public Query//2000
{
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	const CGHeroInstance *hero;
	PrimarySkill::PrimarySkill primskill;
	std::vector<SecondarySkill> skills;

	HeroLevelUp(){type = 2000;};

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & queryID & hero & primskill & skills;
	}
};

struct CommanderLevelUp : public Query
{
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	const CGHeroInstance *hero;

	std::vector<ui32> skills; //0-5 - secondary skills, val-100 - special skill

	CommanderLevelUp(){type = 2005;};

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & queryID & hero & skills;
	}
};

struct TradeComponents : public CPackForClient, public CPackForServer
{
///used to handle info about components available in shops
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	si32 heroid;
	ui32 objectid;
	std::map<ui16, Component> available, chosen, bought;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroid & objectid & available & chosen & bought;
	}

};
//A dialog that requires making decision by player - it may contain components to choose between or has yes/no options
//Client responds with QueryReply, where answer: 0 - cancel pressed, choice doesn't matter; 1/2/...  - first/second/... component selected and OK pressed
//Until sending reply player won't be allowed to take any actions
struct BlockingDialog : public Query//2003
{
	enum {ALLOW_CANCEL = 1, SELECTION = 2};

	void applyCl(CClient *cl);

	MetaString text;
	std::vector<Component> components;
	PlayerColor player;
	ui8 flags;
	ui16 soundID;

	bool cancel() const
	{
		return flags & ALLOW_CANCEL;
	}
	bool selection() const
	{
		return flags & SELECTION;
	}

	BlockingDialog(bool yesno, bool Selection)
	{
		type = 2003;
		flags = 0;
		soundID = 0;
		if(yesno) flags |= ALLOW_CANCEL;
		if(Selection) flags |= SELECTION;
	}
	BlockingDialog()
	{
		type = 2003;
		flags = 0;
		soundID = 0;
	};

	void addResourceComponents(TResources resources)
	{
		for(TResources::nziterator i(resources); i.valid(); i++)
			components.push_back(Component(Component::RESOURCE, i->resType, i->resVal, 0));
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & queryID & text & components & player & flags & soundID;
	}
};

struct GarrisonDialog : public Query//2004
{
	GarrisonDialog(){type = 2004;}
	void applyCl(CClient *cl);
	ObjectInstanceID objid, hid;
	bool removableUnits;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & queryID & objid & hid & removableUnits;
	}
};

struct ExchangeDialog : public Query//2005
{
	ExchangeDialog(){type = 2005;}
	void applyCl(CClient *cl);

	std::array<const CGHeroInstance*, 2> heroes;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & queryID & heroes;
	}
};

struct TeleportDialog : public Query//2006
{
	TeleportDialog() {type = 2006;}
	TeleportDialog(const CGHeroInstance *Hero, TeleportChannelID Channel)
		: hero(Hero), channel(Channel), impassable(false)
	{
		type = 2006;
	}

	void applyCl(CClient *cl);

	const CGHeroInstance *hero;
	TeleportChannelID channel;
	TTeleportExitsList exits;
	bool impassable;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & queryID & hero & channel & exits & impassable;
	}
};

struct BattleInfo;
struct BattleStart : public CPackForClient//3000
{
	BattleStart(){type = 3000;};

	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	BattleInfo * info;


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & info;
	}
};
struct BattleNextRound : public CPackForClient//3001
{
	BattleNextRound(){type = 3001;};
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs( CGameState *gs );
	si32 round;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & round;
	}
};
struct BattleSetActiveStack : public CPackForClient//3002
{
	BattleSetActiveStack()
	{
		type = 3002;
		askPlayerInterface = true;
	}

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui32 stack;
	ui8 askPlayerInterface;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stack & askPlayerInterface;
	}
};
struct BattleResult : public CPackForClient//3003
{
	enum EResult {NORMAL = 0, ESCAPE = 1, SURRENDER = 2};

	BattleResult(){type = 3003;};
	void applyFirstCl(CClient *cl);
	void applyGs(CGameState *gs);

	EResult result;
	ui8 winner; //0 - attacker, 1 - defender, [2 - draw (should be possible?)]
	std::map<ui32,si32> casualties[2]; //first => casualties of attackers - map crid => number
	TExpType exp[2]; //exp for attacker and defender
	std::set<ArtifactInstanceID> artifacts; //artifacts taken from loser to winner - currently unused

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & result & winner & casualties[0] & casualties[1] & exp & artifacts;
	}
};

struct BattleStackMoved : public CPackForClient//3004
{
	ui32 stack;
	std::vector<BattleHex> tilesToMove;
	ui8 distance, teleporting;
	BattleStackMoved(){type = 3004;};
	void applyFirstCl(CClient *cl);
	void applyGs(CGameState *gs);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stack & tilesToMove & distance;
	}
};

struct StacksHealedOrResurrected : public CPackForClient //3013
{
	StacksHealedOrResurrected(){type = 3013;}

	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	struct HealInfo
	{
		ui32 stackID;
		ui32 healedHP;
		bool lowLevelResurrection; //in case this stack is resurrected by this heal, it will be marked as SUMMONED //TODO: replace with separate counter
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & stackID & healedHP & lowLevelResurrection;
		}
	};

	std::vector<HealInfo> healedStacks;
	bool lifeDrain; //if true, this heal is an effect of life drain
	bool tentHealing; //if true, than it's healing via First Aid Tent
	si32 drainedFrom; //if life drain - then stack life was drain from, if tentHealing - stack that is a healer

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & healedStacks & lifeDrain & tentHealing & drainedFrom;
	}
};

struct BattleStackAttacked : public CPackForClient//3005
{
	BattleStackAttacked():
		flags(0), spellID(SpellID::NONE){type=3005;};
	void applyFirstCl(CClient * cl);
	//void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui32 stackAttacked, attackerID;
	ui32 newAmount, newHP, killedAmount, damageAmount;
	enum EFlags {KILLED = 1, EFFECT = 2/*deprecated */, SECONDARY = 4, REBIRTH = 8, CLONE_KILLED = 16, SPELL_EFFECT = 32 /*, BONUS_EFFECT = 64 */};
	ui32 flags; //uses EFlags (above)
	ui32 effect; //set only if flag EFFECT is set
	SpellID spellID; //only if flag SPELL_EFFECT is set
	std::vector<StacksHealedOrResurrected> healedStacks; //used when life drain


	bool killed() const//if target stack was killed
	{
		return flags & KILLED || flags & CLONE_KILLED;
	}
	bool cloneKilled() const
	{
		return flags & CLONE_KILLED;
	}
	bool isEffect() const//if stack has been attacked by a spell
	{
		return flags & EFFECT;
	}
	bool isSecondary() const//if stack was not a primary target (receives no spell effects)
	{
		return flags & SECONDARY;
	}
	///Attacked with spell (SPELL_LIKE_ATTACK)
	bool isSpell() const
	{
		return flags & SPELL_EFFECT;
	}
	bool willRebirth() const//resurrection, e.g. Phoenix
	{
		return flags & REBIRTH;
	}
	bool lifeDrain() const //if this attack involves life drain effect
	{
		return healedStacks.size() > 0;
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stackAttacked & attackerID & newAmount & newHP & flags & killedAmount & damageAmount & effect
			& healedStacks;
		h & spellID;
	}
	bool operator<(const BattleStackAttacked &b) const
	{
		return stackAttacked < b.stackAttacked;
	}
};

struct BattleAttack : public CPackForClient//3006
{
	BattleAttack(): flags(0), spellID(SpellID::NONE){type = 3006;};
	void applyFirstCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	std::vector<BattleStackAttacked> bsa;
	ui32 stackAttacking;
	ui32 flags; //uses Eflags (below)
	enum EFlags{SHOT = 1, COUNTER = 2, LUCKY = 4, UNLUCKY = 8, BALLISTA_DOUBLE_DMG = 16, DEATH_BLOW = 32, SPELL_LIKE = 64};
	
	SpellID spellID; //for SPELL_LIKE 

	bool shot() const//distance attack - decrease number of shots
	{
		return flags & SHOT;
	}
	bool counter() const//is it counterattack?
	{
		return flags & COUNTER;
	}
	bool lucky() const
	{
		return flags & LUCKY;
	}
	bool unlucky() const
	{
		return flags & UNLUCKY;
	}
	bool ballistaDoubleDmg() const //if it's ballista attack and does double dmg
	{
		return flags & BALLISTA_DOUBLE_DMG;
	}
	bool deathBlow() const
	{
		return flags & DEATH_BLOW;
	}
	bool spellLike() const
	{
		return flags & SPELL_LIKE;
	}
	//bool killed() //if target stack was killed
	//{
	//	return bsa.killed();
	//}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & bsa & stackAttacking & flags & spellID;
	}
};

struct StartAction : public CPackForClient//3007
{
	StartAction(){type = 3007;};
	StartAction(const BattleAction &act){ba = act; type = 3007;};
	void applyFirstCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	BattleAction ba;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ba;
	}
};

struct EndAction : public CPackForClient//3008
{
	EndAction(){type = 3008;};
	void applyCl(CClient *cl);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
	}
};

struct BattleSpellCast : public CPackForClient//3009
{
	///custom effect (resistance, reflection, etc)
	struct CustomEffect
	{
		/// WoG AC format
		ui32 effect;		
		ui32 stack;
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & effect & stack;
		}		
	};

	BattleSpellCast(){type = 3009; casterStack = -1;};
	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	si32 dmgToDisplay; //this amount will be displayed as amount of damage dealt by spell
	ui8 side; //which hero did cast spell: 0 - attacker, 1 - defender
	ui32 id; //id of spell
	ui8 skill; //caster's skill level
	ui8 manaGained; //mana channeling ability
	BattleHex tile; //destination tile (may not be set in some global/mass spells
	std::vector<CustomEffect> customEffects;
	std::set<ui32> affectedCres; //ids of creatures affected by this spell, generally used if spell does not set any effect (like dispel or cure)
	si32 casterStack;// -1 if not cated by creature, >=0 caster stack ID
	bool castByHero; //if true - spell has been cast by hero, otherwise by a creature
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & dmgToDisplay & side & id & skill & manaGained & tile & customEffects & affectedCres & casterStack & castByHero;
	}
};

struct SetStackEffect : public CPackForClient //3010
{
	SetStackEffect(){type = 3010;};
	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	std::vector<ui32> stacks; //affected stacks (IDs)
	std::vector<Bonus> effect; //bonuses to apply
	std::vector<std::pair<ui32, Bonus> > uniqueBonuses; //bonuses per single stack
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stacks & effect & uniqueBonuses;
	}
};

struct StacksInjured : public CPackForClient //3011
{
	StacksInjured(){type = 3011;}
	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	std::vector<BattleStackAttacked> stacks;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stacks;
	}
};

struct BattleResultsApplied : public CPackForClient //3012
{
	BattleResultsApplied(){type = 3012;}

	PlayerColor player1, player2;

	void applyCl(CClient *cl);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player1 & player2;
	}
};

struct ObstaclesRemoved : public CPackForClient //3014
{
	ObstaclesRemoved(){type = 3014;}

	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	std::set<si32> obstacles; //uniqueIDs of removed obstacles

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & obstacles;
	}
};

struct ELF_VISIBILITY CatapultAttack : public CPackForClient //3015
{
	struct AttackInfo
	{
		si16 destinationTile;
		ui8 attackedPart;
		ui8 damageDealt;

		DLL_LINKAGE std::string toString() const;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & destinationTile & attackedPart & damageDealt;
		}
	};

	DLL_LINKAGE CatapultAttack();
	DLL_LINKAGE ~CatapultAttack();

	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);
	DLL_LINKAGE std::string toString() const override;

	std::vector< AttackInfo > attackedParts;
	int attacker; //if -1, then a spell caused this

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & attackedParts & attacker;
	}
};

DLL_LINKAGE std::ostream & operator<<(std::ostream & out, const CatapultAttack::AttackInfo & attackInfo);

struct BattleStacksRemoved : public CPackForClient //3016
{
	BattleStacksRemoved(){type = 3016;}

	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyFirstCl(CClient *cl);//inform client before stack objects are destroyed

	std::set<ui32> stackIDs; //IDs of removed stacks

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stackIDs;
	}
};

struct BattleStackAdded : public CPackForClient //3017
{
	BattleStackAdded(){type = 3017;};

	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	int attacker; // if true, stack belongs to attacker
	CreatureID creID;
	int amount;
	int pos;
	int summoned; //if true, remove it afterwards
	
	///Actual stack ID, set on apply, do not serialize
	int newStackID; 

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & attacker & creID & amount & pos & summoned;
	}
};

struct BattleSetStackProperty : public CPackForClient //3018
{
	BattleSetStackProperty(){type = 3018;};

	enum BattleStackProperty {CASTS, ENCHANTER_COUNTER, UNBIND, CLONED, HAS_CLONE};

	DLL_LINKAGE void applyGs(CGameState *gs);

	int stackID;
	BattleStackProperty which;
	int val;
	int absolute;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stackID & which & val & absolute;
	}
};

struct BattleTriggerEffect : public CPackForClient //3019
{ //activated at the beginning of turn
	BattleTriggerEffect(){type = 3019;};

	DLL_LINKAGE void applyGs(CGameState *gs); //effect
	void applyCl(CClient *cl); //play animations & stuff

	//enum BattleEffect {REGENERATION, MANA_DRAIN, FEAR, MANA_CHANNELING, ENCHANTER, UNBIND, POISON, ENCHANTED, SUMMONER};

	int stackID;
	int effect; //use enumBattleEffect or corresponding Bonus type for sanity?
	int val;
	int additionalInfo;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stackID & effect & val & additionalInfo;
	}
};

struct BattleObstaclePlaced : public CPackForClient //3020
{ //activated at the beginning of turn
	BattleObstaclePlaced(){type = 3020;};

	DLL_LINKAGE void applyGs(CGameState *gs); //effect
	void applyCl(CClient *cl); //play animations & stuff

	std::shared_ptr<CObstacleInstance> obstacle;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & obstacle;
	}
};


struct ShowInInfobox : public CPackForClient //107
{
	ShowInInfobox(){type = 107;};
	PlayerColor player;
	Component c;
	MetaString text;

	void applyCl(CClient *cl);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player & c & text;
	}
};

struct AdvmapSpellCast : public CPackForClient //108
{
	AdvmapSpellCast(){type = 108;}
	const CGHeroInstance * caster;
	SpellID spellID;

	void applyCl(CClient *cl);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & caster & spellID;
	}
};

struct ShowWorldViewEx : public CPackForClient //4000
{
	PlayerColor player;
	
	std::vector<ObjectPosInfo> objectPositions;
	
	ShowWorldViewEx(){type = 4000;}
	
	void applyCl(CClient *cl);
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player & objectPositions;
	}	
};

/***********************************************************************************************************/

struct CommitPackage : public CPackForServer
{
	bool freePack; //for local usage, DO NOT serialize
	bool applyGh(CGameHandler *gh);
	CPackForClient *packToCommit;

	CommitPackage()
	{
		freePack = true;
	}
	~CommitPackage()
	{
		if(freePack)
			delete packToCommit;
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & packToCommit;
	}
};


struct CloseServer : public CPackForServer
{
	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{}
};

struct EndTurn : public CPackForServer
{
	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{}
};

struct DismissHero : public CPackForServer
{
	DismissHero(){};
	DismissHero(ObjectInstanceID HID) : hid(HID) {};
	ObjectInstanceID hid;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid;
	}
};

struct MoveHero : public CPackForServer
{
	MoveHero(){};
	MoveHero(const int3 &Dest, ObjectInstanceID HID, bool Transit) : dest(Dest), hid(HID), transit(Transit) {};
	int3 dest;
	ObjectInstanceID hid;
	bool transit;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & dest & hid & transit;
	}
};

struct CastleTeleportHero : public CPackForServer
{
	CastleTeleportHero(){};
	CastleTeleportHero(const ObjectInstanceID HID, ObjectInstanceID Dest, ui8 Source ) : dest(Dest), hid(HID), source(Source){};
	ObjectInstanceID dest;
	ObjectInstanceID hid;
	si8 source;//who give teleporting, 1=castle gate

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & dest & hid;
	}
};

struct ArrangeStacks : public CPackForServer
{
	ArrangeStacks(){};
	ArrangeStacks(ui8 W, SlotID P1, SlotID P2, ObjectInstanceID ID1, ObjectInstanceID ID2, si32 VAL)
		:what(W),p1(P1),p2(P2),id1(ID1),id2(ID2),val(VAL) {};

	ui8 what; //1 - swap; 2 - merge; 3 - split
	SlotID p1, p2; //positions of first and second stack
	ObjectInstanceID id1, id2; //ids of objects with garrison
	si32 val;
	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & what & p1 & p2 & id1 & id2 & val;
	}
};

struct DisbandCreature : public CPackForServer
{
	DisbandCreature(){};
	DisbandCreature(SlotID Pos, ObjectInstanceID ID):pos(Pos),id(ID){};
	SlotID pos; //stack pos
	ObjectInstanceID id; //object id

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & pos & id;
	}
};

struct BuildStructure : public CPackForServer
{
	BuildStructure(){};
	BuildStructure(ObjectInstanceID TID, BuildingID BID):tid(TID), bid(BID){};
	ObjectInstanceID tid; //town id
	BuildingID bid; //structure id

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid & bid;
	}
};
struct RazeStructure : public BuildStructure
{
	RazeStructure(){};
	//RazeStructure(si32 TID, si32 BID):bid(BID),tid(TID){};

	bool applyGh(CGameHandler *gh);
};
struct RecruitCreatures : public CPackForServer
{
	RecruitCreatures(){};
	RecruitCreatures(ObjectInstanceID TID, ObjectInstanceID DST, CreatureID CRID, si32 Amount, si32 Level):
	    tid(TID), dst(DST), crid(CRID), amount(Amount), level(Level){};
	ObjectInstanceID tid; //dwelling id, or town
	ObjectInstanceID dst; //destination ID, e.g. hero
	CreatureID crid;
	ui32 amount;//creature amount
	si32 level;//dwelling level to buy from, -1 if any
	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid & dst & crid & amount & level;
	}
};

struct UpgradeCreature : public CPackForServer
{
	UpgradeCreature(){};
	UpgradeCreature(SlotID Pos, ObjectInstanceID ID, CreatureID CRID):pos(Pos),id(ID), cid(CRID){};
	SlotID pos; //stack pos
	ObjectInstanceID id; //object id
	CreatureID cid; //id of type to which we want make upgrade

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & pos & id & cid;
	}
};

struct GarrisonHeroSwap : public CPackForServer
{
	GarrisonHeroSwap(){};
	GarrisonHeroSwap(ObjectInstanceID TID):tid(TID){};
	ObjectInstanceID tid;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid;
	}
};

struct ExchangeArtifacts : public CPackForServer
//TODO: allow exchange between heroes, stacks and commanders
{
	ArtifactLocation src, dst;
	ExchangeArtifacts(){};

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & src & dst;
	}
};

struct AssembleArtifacts : public CPackForServer
{
	AssembleArtifacts(){};
	AssembleArtifacts(ObjectInstanceID _heroID, ArtifactPosition _artifactSlot, bool _assemble, ArtifactID _assembleTo)
		: heroID(_heroID), artifactSlot(_artifactSlot), assemble(_assemble), assembleTo(_assembleTo){};
	ObjectInstanceID heroID;
	ArtifactPosition artifactSlot;
	bool assemble; // True to assemble artifact, false to disassemble.
	ArtifactID assembleTo; // Artifact to assemble into.

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroID & artifactSlot & assemble & assembleTo;
	}
};

struct BuyArtifact : public CPackForServer
{
	BuyArtifact(){};
	BuyArtifact(ObjectInstanceID HID, ArtifactID AID):hid(HID),aid(AID){};
	ObjectInstanceID hid;
	ArtifactID aid;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid & aid;
	}
};

struct TradeOnMarketplace : public CPackForServer
{
	TradeOnMarketplace(){};

	const CGObjectInstance *market;
	const CGHeroInstance *hero; //needed when trading artifacts / creatures
	EMarketMode::EMarketMode mode;
	ui32 r1, r2; //mode 0: r1 - sold resource, r2 - bought res (exception: when sacrificing art r1 is art id [todo: make r2 preferred slot?]
	ui32 val; //units of sold resource

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & market & hero & mode & r1 & r2 & val;
	}
};

struct SetFormation : public CPackForServer
{
	SetFormation(){};
	SetFormation(ObjectInstanceID HID, ui8 Formation):hid(HID),formation(Formation){};
	ObjectInstanceID hid;
	ui8 formation;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid & formation;
	}
};

struct HireHero : public CPackForServer
{
	HireHero(){};
	HireHero(si32 HID, ObjectInstanceID TID):hid(HID),tid(TID){};
	si32 hid; //available hero serial
	ObjectInstanceID tid; //town (tavern) id
	PlayerColor player;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid & tid & player;
	}
};

struct BuildBoat : public CPackForServer
{
	BuildBoat(){};
	ObjectInstanceID objid; //where player wants to buy a boat

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objid;
	}

};

struct QueryReply : public CPackForServer
{
	QueryReply(){type = 6000;};
	QueryReply(QueryID QID, ui32 Answer):qid(QID),answer(Answer){type = 6000;};
	QueryID qid;
	ui32 answer; //hero and artifact id
	PlayerColor player;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & qid & answer & player;
	}
};

struct MakeAction : public CPackForServer
{
	MakeAction(){};
	MakeAction(const BattleAction &BA):ba(BA){};
	BattleAction ba;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ba;
	}
};

struct MakeCustomAction : public CPackForServer
{
	MakeCustomAction(){};
	MakeCustomAction(const BattleAction &BA):ba(BA){};
	BattleAction ba;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ba;
	}
};

struct DigWithHero : public CPackForServer
{
	DigWithHero(){}
	ObjectInstanceID id; //digging hero id

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id;
	}
};

struct CastAdvSpell : public CPackForServer
{
	CastAdvSpell(){}
	ObjectInstanceID hid; //hero id
	SpellID sid; //spell id
	int3 pos; //selected tile (not always used)

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid & sid & pos;
	}
};

/***********************************************************************************************************/

struct SaveGame : public CPackForClient, public CPackForServer
{
	SaveGame(){};
	SaveGame(const std::string &Fname) :fname(Fname){};
	std::string fname;

	void applyCl(CClient *cl);
	void applyGs(CGameState *gs){};
	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & fname;
	}
};

struct PlayerMessage : public CPackForClient, public CPackForServer //513
{
	PlayerMessage(){CPackForClient::type = 513;};
	PlayerMessage(PlayerColor Player, const std::string &Text, ObjectInstanceID obj)
		:player(Player),text(Text), currObj(obj)
	{CPackForClient::type = 513;};
	void applyCl(CClient *cl);
	void applyGs(CGameState *gs){};
	bool applyGh(CGameHandler *gh);

	PlayerColor player;
	std::string text;
	ObjectInstanceID currObj; // optional parameter that specifies current object. For cheats :)

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & text & player & currObj;
	}
};

struct CenterView : public CPackForClient//515
{
	CenterView(){CPackForClient::type = 515;};
	void applyCl(CClient *cl);

	PlayerColor player;
	int3 pos;
	ui32 focusTime; //ms

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & pos & player & focusTime;
	}
};

/***********************************************************************************************************/

struct CPackForSelectionScreen : public CPack
{
	void apply(CSelectionScreen *selScreen){}; //that functions are implemented in CPreGame.cpp
};

class CPregamePackToPropagate  : public CPackForSelectionScreen
{};

class CPregamePackToHost  : public CPackForSelectionScreen
{};

struct ChatMessage : public CPregamePackToPropagate
{
	std::string playerName, message;

	void apply(CSelectionScreen *selScreen);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & playerName & message;
	}
};

struct QuitMenuWithoutStarting : public CPregamePackToPropagate
{
	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{}
};

struct PlayerJoined : public CPregamePackToHost
{
	std::string playerName;
	ui8 connectionID;

	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & playerName & connectionID;
	}
};

struct ELF_VISIBILITY SelectMap : public CPregamePackToPropagate
{
	const CMapInfo *mapInfo;
	bool free;//local flag, do not serialize

	DLL_LINKAGE SelectMap(const CMapInfo &src);
	DLL_LINKAGE SelectMap();
	DLL_LINKAGE ~SelectMap();

	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & mapInfo;
	}

};

struct ELF_VISIBILITY UpdateStartOptions : public CPregamePackToPropagate
{
	StartInfo *options;
	bool free;//local flag, do not serialize

	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	DLL_LINKAGE UpdateStartOptions(StartInfo &src);
	DLL_LINKAGE UpdateStartOptions();
	DLL_LINKAGE ~UpdateStartOptions();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & options;
	}

};

struct PregameGuiAction : public CPregamePackToPropagate
{
	enum {NO_TAB, OPEN_OPTIONS, OPEN_SCENARIO_LIST, OPEN_RANDOM_MAP_OPTIONS} 
		action;

	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & action;
	}
};

struct RequestOptionsChange : public CPregamePackToHost
{
	enum EWhat {TOWN, HERO, BONUS};
	ui8 what;
	si8 direction; //-1 or +1
	ui8 playerID;

	RequestOptionsChange(ui8 What, si8 Dir, ui8 Player)
		:what(What), direction(Dir), playerID(Player)
	{}
	RequestOptionsChange(){}

	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & what & direction & playerID;
	}
};

struct PlayerLeft : public CPregamePackToPropagate
{
	ui8 playerID;

	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & playerID;
	}
};

struct PlayersNames : public CPregamePackToPropagate
{
public:
	std::map<ui8, std::string> playerNames;

	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & playerNames;
	}
};

struct StartWithCurrentSettings : public CPregamePackToPropagate
{
public:
	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		//h & playerNames;
	}
};
