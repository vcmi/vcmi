/*
 * NetPacks.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "NetPacksBase.h"

#include "battle/BattleAction.h"
#include "JsonNode.h"
#include "mapObjects/CGHeroInstance.h"
#include "ConstTransitivePtr.h"
#include "int3.h"
#include "ResourceSet.h"
#include "CGameStateFwd.h"
#include "mapping/CMapDefines.h"
#include "battle/CObstacleInstance.h"

#include "spells/ViewSpellInt.h"

class CClient;
class CGameState;
class CGameHandler;
class CConnection;
class CCampaignState;
class CArtifact;
class CSelectionScreen;
class CGObjectInstance;
class CArtifactInstance;
struct StackLocation;
struct ArtSlotInfo;
struct QuestInfo;
class CMapInfo;
struct StartInfo;

struct CPackForClient : public CPack
{
	CPackForClient(){};

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
		h & army;
		h & slot;
	}
};

/***********************************************************************************************************/


struct PackageApplied : public CPackForClient
{
	PackageApplied()
		: result(0), packType(0),requestID(0)
	{}
	PackageApplied(ui8 Result)
		: result(Result), packType(0), requestID(0)
	{}
	void applyCl(CClient *cl);

	ui8 result; //0 - something went wrong, request hasn't been realized; 1 - OK
	ui32 packType; //type id of applied package
	ui32 requestID; //an ID given by client to the request that was applied
	PlayerColor player;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & result;
		h & packType;
		h & requestID;
		h & player;
	}
};

struct SystemMessage : public CPackForClient
{
	SystemMessage(const std::string & Text) : text(Text){}
	SystemMessage(){}
	void applyCl(CClient *cl);

	std::string text;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & text;
	}
};

struct PlayerBlocked : public CPackForClient
{
	PlayerBlocked() : reason(UPCOMING_BATTLE), startOrEnd(BLOCKADE_STARTED) {}
	void applyCl(CClient *cl);

	enum EReason { UPCOMING_BATTLE, ONGOING_MOVEMENT };
	enum EMode { BLOCKADE_STARTED, BLOCKADE_ENDED };

	EReason reason;
	EMode startOrEnd;
	PlayerColor player;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & reason;
		h & startOrEnd;
		h & player;
	}
};

struct PlayerCheated : public CPackForClient
{
	PlayerCheated() : losingCheatCode(false), winningCheatCode(false) {}
	DLL_LINKAGE void applyGs(CGameState *gs);

	PlayerColor player;
	bool losingCheatCode;
	bool winningCheatCode;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player;
		h & losingCheatCode;
		h & winningCheatCode;
	}
};

struct YourTurn : public CPackForClient
{
	YourTurn(){}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	PlayerColor player;
	boost::optional<ui8> daysWithoutCastle;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player;
		h & daysWithoutCastle;
	}
};

struct SetResources : public CPackForClient
{
	SetResources():abs(true){};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	bool abs; //false - changes by value; 1 - sets to value
	PlayerColor player;
	TResources res; //res[resid] => res amount

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & abs;
		h & player;
		h & res;
	}
};

struct SetPrimSkill : public CPackForClient
{
	SetPrimSkill()
		: abs(0), which(PrimarySkill::ATTACK), val(0)
	{}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 abs; //0 - changes by value; 1 - sets to value
	ObjectInstanceID id;
	PrimarySkill::PrimarySkill which;
	si64 val;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & abs;
		h & id;
		h & which;
		h & val;
	}
};

struct SetSecSkill : public CPackForClient
{
	SetSecSkill()
		: abs(0), val(0)
	{}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 abs; //0 - changes by value; 1 - sets to value
	ObjectInstanceID id;
	SecondarySkill which;
	ui16 val;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & abs;
		h & id;
		h & which;
		h & val;
	}
};

struct HeroVisitCastle : public CPackForClient
{
	HeroVisitCastle(){flags=0;};
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 flags; //1 - start
	ObjectInstanceID tid, hid;

	bool start() //if hero is entering castle (if false - leaving)
	{
		return flags & 1;
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & flags;
		h & tid;
		h & hid;
	}
};

struct ChangeSpells : public CPackForClient
{
	ChangeSpells():learn(1){}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 learn; //1 - gives spell, 0 - takes
	ObjectInstanceID hid;
	std::set<SpellID> spells;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & learn;
		h & hid;
		h & spells;
	}
};

struct SetMana : public CPackForClient
{
	SetMana(){val = 0; absolute=true;}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);


	ObjectInstanceID hid;
	si32 val;
	bool absolute;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & val;
		h & hid;
		h & absolute;
	}
};

struct SetMovePoints : public CPackForClient
{
	SetMovePoints(){val = 0; absolute=true;}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID hid;
	si32 val;
	bool absolute;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & val;
		h & hid;
		h & absolute;
	}
};

struct FoWChange : public CPackForClient
{
	FoWChange(){mode = 0; waitForDialogs = false;}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	std::unordered_set<int3, struct ShashInt3 > tiles;
	PlayerColor player;
	ui8 mode; //mode==0 - hide, mode==1 - reveal
	bool waitForDialogs;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tiles;
		h & player;
		h & mode;
		h & waitForDialogs;
	}
};

struct SetAvailableHeroes : public CPackForClient
{
	SetAvailableHeroes()
	{
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
		h & player;
		h & hid;
		h & army;
	}
};

struct GiveBonus :  public CPackForClient
{
	GiveBonus(ui8 Who = 0)
	{
		who = Who;
		id = 0;
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
		h & bonus;
		h & id;
		h & bdescr;
		h & who;
		assert( id != -1);
	}
};

struct ChangeObjPos : public CPackForClient
{
	ChangeObjPos()
	{
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
		h & objid;
		h & nPos;
		h & flags;
	}
};

struct PlayerEndsGame : public CPackForClient
{
	PlayerEndsGame()
	{
	}

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	PlayerColor player;
	EVictoryLossCheckResult victoryLossCheckResult;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player;
		h & victoryLossCheckResult;
	}
};

struct RemoveBonus :  public CPackForClient
{
	RemoveBonus(ui8 Who = 0)
	{
		who = Who;
		whoID = 0;
		source = 0;
		id = 0;
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
		h & source;
		h & id;
		h & who;
		h & whoID;
	}
};

struct UpdateCampaignState : public CPackForClient
{
	UpdateCampaignState(){}
	std::shared_ptr<CCampaignState> camp;
	void applyCl(CClient *cl);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & camp;
	}
};

struct SetCommanderProperty : public CPackForClient
{
	enum ECommanderProperty {ALIVE, BONUS, SECONDARY_SKILL, EXPERIENCE, SPECIAL_SKILL};

	SetCommanderProperty()
		:which(ALIVE), amount(0), additionalInfo(0)
	{}
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
		h & heroid;
		h & sl;
		h & which;
		h & amount;
		h & additionalInfo;
		h & accumulatedBonus;
	}
};

struct AddQuest : public CPackForClient
{
	AddQuest(){};
	void applyCl(CClient *cl){};
	DLL_LINKAGE void applyGs(CGameState *gs);

	PlayerColor player;
	QuestInfo quest;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player;
		h & quest;
	}
};

struct PrepareForAdvancingCampaign : public CPackForClient
{
	PrepareForAdvancingCampaign(){}

	void applyCl(CClient *cl);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
	}
};

struct UpdateArtHandlerLists : public CPackForClient
{
	UpdateArtHandlerLists(){}
	std::vector<CArtifact*> treasures, minors, majors, relics;

	DLL_LINKAGE void applyGs(CGameState *gs);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & treasures;
		h & minors;
		h & majors;
		h & relics;
	}
};

struct UpdateMapEvents : public CPackForClient
{
	UpdateMapEvents(){}

	std::list<CMapEvent> events;
	DLL_LINKAGE void applyGs(CGameState *gs);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & events;
	}
};

struct UpdateCastleEvents : public CPackForClient
{
	UpdateCastleEvents(){}

	ObjectInstanceID town;
	std::list<CCastleEvent> events;

	DLL_LINKAGE void applyGs(CGameState *gs);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & town;
		h & events;
	}
};

struct ChangeFormation : public CPackForClient
{
	ChangeFormation():formation(0){}

	ObjectInstanceID hid;
	ui8 formation;

	DLL_LINKAGE void applyGs(CGameState *gs);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid;
		h & formation;
	}
};

struct RemoveObject : public CPackForClient
{
	RemoveObject(){}
	RemoveObject(ObjectInstanceID ID){id = ID;};
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID id;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id;
	}
};

struct TryMoveHero : public CPackForClient
{
	TryMoveHero()
		: movePoints(0), result(FAILED), humanKnows(false)
	{}
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
		h & id;
		h & result;
		h & start;
		h & end;
		h & movePoints;
		h & fowRevealed;
		h & attackedFrom;
	}
};

struct NewStructures : public CPackForClient
{
	NewStructures():builded(0){}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID tid;
	std::set<BuildingID> bid;
	si16 builded;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid;
		h & bid;
		h & builded;
	}
};

struct RazeStructures : public CPackForClient
{
	RazeStructures():destroyed(0){}
	void applyCl (CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID tid;
	std::set<BuildingID> bid;
	si16 destroyed;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid;
		h & bid;
		h & destroyed;
	}
};

struct SetAvailableCreatures : public CPackForClient
{
	SetAvailableCreatures(){}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID tid;
	std::vector<std::pair<ui32, std::vector<CreatureID> > > creatures;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid;
		h & creatures;
	}
};

struct SetHeroesInTown : public CPackForClient
{
	SetHeroesInTown(){}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID tid, visiting, garrison; //id of town, visiting hero, hero in garrison

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid;
		h & visiting;
		h & garrison;
	}
};

struct HeroRecruited : public CPackForClient
{
	HeroRecruited():hid(-1){}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	si32 hid;//subID of hero
	ObjectInstanceID tid;
	int3 tile;
	PlayerColor player;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid;
		h & tid;
		h & tile;
		h & player;
	}
};

struct GiveHero : public CPackForClient
{
	GiveHero(){}
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID id; //object id
	PlayerColor player;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id;
		h & player;
	}
};

struct OpenWindow : public CPackForClient
{
	OpenWindow():id1(-1),id2(-1){}
	void applyCl(CClient *cl);

	enum EWindow {EXCHANGE_WINDOW, RECRUITMENT_FIRST, RECRUITMENT_ALL, SHIPYARD_WINDOW, THIEVES_GUILD,
	              UNIVERSITY_WINDOW, HILL_FORT_WINDOW, MARKET_WINDOW, PUZZLE_MAP, TAVERN_WINDOW};
	ui8 window;
	si32 id1, id2;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & window;
		h & id1;
		h & id2;
	}
};

struct NewObject  : public CPackForClient
{
	NewObject():subID(0){}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	Obj ID;
	ui32 subID;
	int3 pos;

	ObjectInstanceID id; //used locally, filled during applyGs

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID;
		h & subID;
		h & pos;
	}
};

struct SetAvailableArtifacts  : public CPackForClient
{
	SetAvailableArtifacts():id(0){}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	si32 id; //two variants: id < 0: set artifact pool for Artifact Merchants in towns; id >= 0: set pool for adv. map Black Market (id is the id of Black Market instance then)
	std::vector<const CArtifact *> arts;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id;
		h & arts;
	}
};

struct NewArtifact : public CPackForClient
{
	NewArtifact(){}
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

struct ChangeStackCount : CGarrisonOperationPack
{
	StackLocation sl;
	TQuantity count;
	ui8 absoluteValue; //if not -> count will be added (or subtracted if negative)

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sl;
		h & count;
		h & absoluteValue;
	}
};

struct SetStackType : CGarrisonOperationPack
{
	StackLocation sl;
	const CCreature *type;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sl;
		h & type;
	}
};

struct EraseStack : CGarrisonOperationPack
{
	StackLocation sl;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sl;
	}
};

struct SwapStacks : CGarrisonOperationPack
{
	StackLocation sl1, sl2;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sl1;
		h & sl2;
	}
};

struct InsertNewStack : CGarrisonOperationPack
{
	StackLocation sl;
	CStackBasicDescriptor stack;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & sl;
		h & stack;
	}
};

///moves creatures from src stack to dst slot, may be used for merging/splittint/moving stacks
struct RebalanceStacks : CGarrisonOperationPack
{
	StackLocation src, dst;
	TQuantity count;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & src;
		h & dst;
		h & count;
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

struct PutArtifact : CArtifactOperationPack
{
	ArtifactLocation al;
	ConstTransitivePtr<CArtifactInstance> art;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & al;
		h & art;
	}
};

struct EraseArtifact : CArtifactOperationPack
{
	ArtifactLocation al;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & al;
	}
};

struct MoveArtifact : CArtifactOperationPack
{
	ArtifactLocation src, dst;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & src;
		h & dst;
	}
};

struct AssembledArtifact : CArtifactOperationPack
{
	ArtifactLocation al; //where assembly will be put
	CArtifact *builtArt;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & al;
		h & builtArt;
	}
};

struct DisassembledArtifact : CArtifactOperationPack
{
	ArtifactLocation al;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & al;
	}
};

struct HeroVisit : CPackForClient
{
	const CGHeroInstance *hero;
	const CGObjectInstance *obj;
	PlayerColor player; //if hero was killed during the visit, its color is already reset
	bool starting; //false -> ending

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hero;
		h & obj;
		h & player;
		h & starting;
	}
};

struct NewTurn : public CPackForClient
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
			h & id;
			h & move;
			h & mana;
		}
		bool operator<(const Hero&h)const{return id < h.id;}
	};

	std::set<Hero> heroes; //updates movement and mana points
	std::map<PlayerColor, TResources> res; //player ID => resource value[res_id]
	std::map<ObjectInstanceID, SetAvailableCreatures> cres;//creatures to be placed in towns
	ui32 day;
	ui8 specialWeek; //weekType
	CreatureID creatureid; //for creature weeks

	NewTurn():day(0),specialWeek(0){};

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroes;
		h & cres;
		h & res;
		h & day;
		h & specialWeek;
		h & creatureid;
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
		h & text;
		h & components;
		h & player;
		h & soundID;
	}
	InfoWindow()
	{
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

struct SetObjectProperty : public CPackForClient
{
	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	ObjectInstanceID id;
	ui8 what; // see ObjProperty enum
	ui32 val;
	SetObjectProperty():what(0),val(0){}
	SetObjectProperty(ObjectInstanceID ID, ui8 What, ui32 Val):id(ID),what(What),val(Val){};

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id;
		h & what;
		h & val;
	}
};

struct ChangeObjectVisitors : public CPackForClient
{
	enum VisitMode
	{
		VISITOR_ADD,      // mark hero as one that have visited this object
		VISITOR_ADD_TEAM, // mark team as one that have visited this object
		VISITOR_REMOVE,   // unmark visitor, reversed to ADD
		VISITOR_CLEAR     // clear all visitors from this object (object reset)
	};
	ui32 mode; // uses VisitMode enum
	ObjectInstanceID object;
	ObjectInstanceID hero; // note: hero owner will be also marked as "visited" this object

	DLL_LINKAGE void applyGs(CGameState *gs);

	ChangeObjectVisitors()
		: mode(VISITOR_CLEAR)
	{}

	ChangeObjectVisitors(ui32 mode, ObjectInstanceID object, ObjectInstanceID heroID = ObjectInstanceID(-1)):
		mode(mode),
		object(object),
		hero(heroID)
	{}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & object;
		h & hero;
		h & mode;
	}
};

struct PrepareHeroLevelUp : public CPackForClient
{
	DLL_LINKAGE void applyGs(CGameState *gs);

	const CGHeroInstance *hero;

	/// Do not serialize, used by server only
	std::vector<SecondarySkill> skills;

	PrepareHeroLevelUp():hero(nullptr){}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hero;
	}
};

struct HeroLevelUp : public Query
{
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	const CGHeroInstance *hero;
	PrimarySkill::PrimarySkill primskill;
	std::vector<SecondarySkill> skills;

	HeroLevelUp():hero(nullptr),primskill(PrimarySkill::ATTACK){}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & queryID;
		h & hero;
		h & primskill;
		h & skills;
	}
};

struct CommanderLevelUp : public Query
{
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	const CGHeroInstance *hero;

	std::vector<ui32> skills; //0-5 - secondary skills, val-100 - special skill

	CommanderLevelUp():hero(nullptr){}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & queryID;
		h & hero;
		h & skills;
	}
};

//A dialog that requires making decision by player - it may contain components to choose between or has yes/no options
//Client responds with QueryReply, where answer: 0 - cancel pressed, choice doesn't matter; 1/2/...  - first/second/... component selected and OK pressed
//Until sending reply player won't be allowed to take any actions
struct BlockingDialog : public Query
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
		flags = 0;
		soundID = 0;
		if(yesno) flags |= ALLOW_CANCEL;
		if(Selection) flags |= SELECTION;
	}
	BlockingDialog()
	{
		flags = 0;
		soundID = 0;
	};

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & queryID;
		h & text;
		h & components;
		h & player;
		h & flags;
		h & soundID;
	}
};

struct GarrisonDialog : public Query
{
	GarrisonDialog():removableUnits(false){}
	void applyCl(CClient *cl);
	ObjectInstanceID objid, hid;
	bool removableUnits;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & queryID;
		h & objid;
		h & hid;
		h & removableUnits;
	}
};

struct ExchangeDialog : public Query
{
	ExchangeDialog()
	{
		heroes = {nullptr,nullptr};
	}
	void applyCl(CClient *cl);

	std::array<const CGHeroInstance*, 2> heroes;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & queryID;
		h & heroes;
	}
};

struct TeleportDialog : public Query
{
	TeleportDialog()
		: hero(nullptr), impassable(false)
	{}

	TeleportDialog(const CGHeroInstance *Hero, TeleportChannelID Channel)
		: hero(Hero), channel(Channel), impassable(false)
	{}

	void applyCl(CClient *cl);

	const CGHeroInstance *hero;
	TeleportChannelID channel;
	TTeleportExitsList exits;
	bool impassable;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & queryID;
		h & hero;
		h & channel;
		h & exits;
		h & impassable;
	}
};

struct MapObjectSelectDialog : public Query
{
	PlayerColor player;
	Component icon;
	MetaString title;
	MetaString description;
	std::vector<ObjectInstanceID> objects;

	MapObjectSelectDialog(){};

	void applyCl(CClient * cl);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & queryID;
		h & player;
		h & icon;
		h & title;
		h & description;
		h & objects;
	}
};

struct BattleInfo;
struct BattleStart : public CPackForClient
{
	BattleStart()
		:info(nullptr)
	{}

	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	BattleInfo * info;


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & info;
	}
};

struct BattleNextRound : public CPackForClient
{
	BattleNextRound():round(0){};
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs( CGameState *gs );
	si32 round;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & round;
	}
};

struct BattleSetActiveStack : public CPackForClient
{
	BattleSetActiveStack()
	{
		stack = 0;
		askPlayerInterface = true;
	}

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui32 stack;
	ui8 askPlayerInterface;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stack;
		h & askPlayerInterface;
	}
};

struct BattleResult : public CPackForClient
{
	enum EResult {NORMAL = 0, ESCAPE = 1, SURRENDER = 2};

	BattleResult()
		: result(NORMAL), winner(2)
	{
		exp[0] = 0;
		exp[1] = 0;
	};
	void applyFirstCl(CClient *cl);
	void applyGs(CGameState *gs);

	EResult result;
	ui8 winner; //0 - attacker, 1 - defender, [2 - draw (should be possible?)]
	std::map<ui32,si32> casualties[2]; //first => casualties of attackers - map crid => number
	TExpType exp[2]; //exp for attacker and defender
	std::set<ArtifactInstanceID> artifacts; //artifacts taken from loser to winner - currently unused

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & result;
		h & winner;
		h & casualties[0];
		h & casualties[1];
		h & exp;
		h & artifacts;
	}
};

struct BattleStackMoved : public CPackForClient
{
	ui32 stack;
	std::vector<BattleHex> tilesToMove;
	ui8 distance, teleporting;
	BattleStackMoved()
		:stack(0), distance(0), teleporting(0)
	{};
	void applyFirstCl(CClient *cl);
	void applyGs(CGameState *gs);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stack;
		h & tilesToMove;
		h & distance;
	}
};

struct StacksHealedOrResurrected : public CPackForClient
{
	StacksHealedOrResurrected()
		:lifeDrain(false), tentHealing(false), drainedFrom(0), cure(false)
	{}

	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	std::vector<CHealthInfo> healedStacks;
	bool lifeDrain; //if true, this heal is an effect of life drain or soul steal
	bool tentHealing; //if true, than it's healing via First Aid Tent
	si32 drainedFrom; //if life drain or soul steal - then stack life was drain from, if tentHealing - stack that is a healer
	bool cure; //archangel cast also remove negative effects

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & healedStacks;
		h & lifeDrain;
		h & tentHealing;
		h & drainedFrom;
		h & cure;
	}
};

struct BattleStackAttacked : public CPackForClient
{
	BattleStackAttacked():
		stackAttacked(0), attackerID(0),
		killedAmount(0), damageAmount(0),
		newHealth(),
		flags(0), effect(0), spellID(SpellID::NONE)
	{};
	void applyFirstCl(CClient * cl);
	//void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui32 stackAttacked, attackerID;
	ui32 killedAmount;
	si32 damageAmount;
	CHealthInfo newHealth;
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
		h & stackAttacked;
		h & attackerID;
		h & newHealth;
		h & flags;
		h & killedAmount;
		h & damageAmount;
		h & effect;
		h & healedStacks;
		h & spellID;
	}
	bool operator<(const BattleStackAttacked &b) const
	{
		return stackAttacked < b.stackAttacked;
	}
};

struct BattleAttack : public CPackForClient
{
	BattleAttack()
		: stackAttacking(0), flags(0), spellID(SpellID::NONE)
	{};
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
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & bsa;
		h & stackAttacking;
		h & flags;
		h & spellID;
	}
};

struct StartAction : public CPackForClient
{
	StartAction(){};
	StartAction(const BattleAction &act){ba = act; };
	void applyFirstCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	BattleAction ba;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ba;
	}
};

struct EndAction : public CPackForClient
{
	EndAction(){};
	void applyCl(CClient *cl);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
	}
};

struct BattleSpellCast : public CPackForClient
{
	///custom effect (resistance, reflection, etc)
	struct CustomEffect
	{
		/// WoG AC format
		ui32 effect;
		ui32 stack;
		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & effect;
			h & stack;
		}
	};

	BattleSpellCast()
	{
		side = 0;
		id = 0;
		skill = 0;
		manaGained = 0;
		casterStack = -1;
		castByHero = true;
	};
	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	ui8 side; //which hero did cast spell: 0 - attacker, 1 - defender
	ui32 id; //id of spell
	ui8 skill; //caster's skill level
	ui8 manaGained; //mana channeling ability
	BattleHex tile; //destination tile (may not be set in some global/mass spells
	std::vector<CustomEffect> customEffects;
	std::set<ui32> affectedCres; //ids of creatures affected by this spell, generally used if spell does not set any effect (like dispel or cure)
	si32 casterStack;// -1 if not cated by creature, >=0 caster stack ID
	bool castByHero; //if true - spell has been cast by hero, otherwise by a creature

	std::vector<MetaString> battleLog;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & side;
		h & id;
		h & skill;
		h & manaGained;
		h & tile;
		h & customEffects;
		h & affectedCres;
		h & casterStack;
		h & castByHero;
		h & battleLog;
	}
};

struct SetStackEffect : public CPackForClient
{
	SetStackEffect(){};
	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	std::vector<ui32> stacks; //affected stacks (IDs)

	//regular effects
	std::vector<Bonus> effect; //bonuses to apply
	std::vector<std::pair<ui32, Bonus> > uniqueBonuses; //bonuses per single stack

	//cumulative effects
	std::vector<Bonus> cumulativeEffects; //bonuses to apply
	std::vector<std::pair<ui32, Bonus> > cumulativeUniqueBonuses; //bonuses per single stack

	std::vector<MetaString> battleLog;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stacks;
		h & effect;
		h & uniqueBonuses;
		h & cumulativeEffects;
		h & cumulativeUniqueBonuses;
		h & battleLog;
	}
};

struct StacksInjured : public CPackForClient
{
	StacksInjured(){}
	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	std::vector<BattleStackAttacked> stacks;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stacks;
	}
};

struct BattleResultsApplied : public CPackForClient
{
	BattleResultsApplied(){}

	PlayerColor player1, player2;

	void applyCl(CClient *cl);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player1;
		h & player2;
	}
};

struct ObstaclesRemoved : public CPackForClient
{
	ObstaclesRemoved(){}

	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	std::set<si32> obstacles; //uniqueIDs of removed obstacles

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & obstacles;
	}
};

struct ELF_VISIBILITY CatapultAttack : public CPackForClient
{
	struct AttackInfo
	{
		si16 destinationTile;
		ui8 attackedPart;
		ui8 damageDealt;

		DLL_LINKAGE std::string toString() const;

		template <typename Handler> void serialize(Handler &h, const int version)
		{
			h & destinationTile;
			h & attackedPart;
			h & damageDealt;
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
		h & attackedParts;
		h & attacker;
	}
};

DLL_LINKAGE std::ostream & operator<<(std::ostream & out, const CatapultAttack::AttackInfo & attackInfo);

struct BattleStacksRemoved : public CPackForClient
{
	BattleStacksRemoved(){}

	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyFirstCl(CClient *cl);//inform client before stack objects are destroyed

	std::set<ui32> stackIDs; //IDs of removed stacks

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stackIDs;
	}
};

struct BattleStackAdded : public CPackForClient
{
	BattleStackAdded()
		: side(0), amount(0), pos(0), summoned(0), newStackID(0)
	{};

	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	ui8 side;
	CreatureID creID;
	int amount;
	int pos;
	int summoned; //if true, remove it afterwards

	///Actual stack ID, set on apply, do not serialize
	int newStackID;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & side;
		h & creID;
		h & amount;
		h & pos;
		h & summoned;
	}
};

struct BattleSetStackProperty : public CPackForClient
{
	BattleSetStackProperty()
		: stackID(0), which(CASTS), val(0), absolute(0)
	{};

	enum BattleStackProperty {CASTS, ENCHANTER_COUNTER, UNBIND, CLONED, HAS_CLONE};

	DLL_LINKAGE void applyGs(CGameState *gs);

	int stackID;
	BattleStackProperty which;
	int val;
	int absolute;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stackID;
		h & which;
		h & val;
		h & absolute;
	}
};

///activated at the beginning of turn
struct BattleTriggerEffect : public CPackForClient
{
	BattleTriggerEffect()
		: stackID(0), effect(0), val(0), additionalInfo(0)
	{};

	DLL_LINKAGE void applyGs(CGameState *gs); //effect
	void applyCl(CClient *cl); //play animations & stuff

	int stackID;
	int effect; //use corresponding Bonus type
	int val;
	int additionalInfo;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stackID;
		h & effect;
		h & val;
		h & additionalInfo;
	}
};

struct BattleObstaclePlaced : public CPackForClient
{
	BattleObstaclePlaced(){};

	DLL_LINKAGE void applyGs(CGameState *gs); //effect
	void applyCl(CClient *cl); //play animations & stuff

	std::shared_ptr<CObstacleInstance> obstacle;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & obstacle;
	}
};

struct BattleUpdateGateState : public CPackForClient
{
	BattleUpdateGateState():state(EGateState::NONE){};

	void applyFirstCl(CClient *cl);

	DLL_LINKAGE void applyGs(CGameState *gs);

	EGateState state;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & state;
	}
};

struct ShowInInfobox : public CPackForClient
{
	ShowInInfobox(){};
	PlayerColor player;
	Component c;
	MetaString text;

	void applyCl(CClient *cl);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player;
		h & c;
		h & text;
	}
};

struct AdvmapSpellCast : public CPackForClient
{
	AdvmapSpellCast():caster(nullptr){}
	const CGHeroInstance * caster;//todo: replace with ObjectInstanceID
	SpellID spellID;

	void applyCl(CClient *cl);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & caster;
		h & spellID;
	}
};

struct ShowWorldViewEx : public CPackForClient
{
	PlayerColor player;

	std::vector<ObjectPosInfo> objectPositions;

	ShowWorldViewEx(){}

	void applyCl(CClient *cl);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player;
		h & objectPositions;
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
		packToCommit = nullptr;
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

struct LeaveGame : public CPackForServer
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
	MoveHero():transit(false){};
	MoveHero(const int3 &Dest, ObjectInstanceID HID, bool Transit) : dest(Dest), hid(HID), transit(Transit) {};
	int3 dest;
	ObjectInstanceID hid;
	bool transit;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & dest;
		h & hid;
		h & transit;
	}
};

struct CastleTeleportHero : public CPackForServer
{
	CastleTeleportHero():source(0){};
	CastleTeleportHero(const ObjectInstanceID HID, ObjectInstanceID Dest, ui8 Source ) : dest(Dest), hid(HID), source(Source){};
	ObjectInstanceID dest;
	ObjectInstanceID hid;
	si8 source;//who give teleporting, 1=castle gate

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & dest;
		h & hid;
	}
};

struct ArrangeStacks : public CPackForServer
{
	ArrangeStacks():what(0), val(0){};
	ArrangeStacks(ui8 W, SlotID P1, SlotID P2, ObjectInstanceID ID1, ObjectInstanceID ID2, si32 VAL)
		:what(W),p1(P1),p2(P2),id1(ID1),id2(ID2),val(VAL) {};

	ui8 what; //1 - swap; 2 - merge; 3 - split
	SlotID p1, p2; //positions of first and second stack
	ObjectInstanceID id1, id2; //ids of objects with garrison
	si32 val;
	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & what;
		h & p1;
		h & p2;
		h & id1;
		h & id2;
		h & val;
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
		h & pos;
		h & id;
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
		h & tid;
		h & bid;
	}
};

struct RazeStructure : public BuildStructure
{
	RazeStructure(){};

	bool applyGh(CGameHandler *gh);
};

struct RecruitCreatures : public CPackForServer
{
	RecruitCreatures():amount(0), level(0){};
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
		h & tid;
		h & dst;
		h & crid;
		h & amount;
		h & level;
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
		h & pos;
		h & id;
		h & cid;
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
		h & src;
		h & dst;
	}
};

struct AssembleArtifacts : public CPackForServer
{
	AssembleArtifacts():assemble(false){};
	AssembleArtifacts(ObjectInstanceID _heroID, ArtifactPosition _artifactSlot, bool _assemble, ArtifactID _assembleTo)
		: heroID(_heroID), artifactSlot(_artifactSlot), assemble(_assemble), assembleTo(_assembleTo){};
	ObjectInstanceID heroID;
	ArtifactPosition artifactSlot;
	bool assemble; // True to assemble artifact, false to disassemble.
	ArtifactID assembleTo; // Artifact to assemble into.

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroID;
		h & artifactSlot;
		h & assemble;
		h & assembleTo;
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
		h & hid;
		h & aid;
	}
};

struct TradeOnMarketplace : public CPackForServer
{
	TradeOnMarketplace()
		:market(nullptr), hero(nullptr), mode(EMarketMode::RESOURCE_RESOURCE), r1(0), r2(0), val(0)
	{};

	const CGObjectInstance *market; //todo: replace with ObjectInstanceID
	const CGHeroInstance *hero; //needed when trading artifacts / creatures
	EMarketMode::EMarketMode mode;
	ui32 r1, r2; //mode 0: r1 - sold resource, r2 - bought res (exception: when sacrificing art r1 is art id [todo: make r2 preferred slot?]
	ui32 val; //units of sold resource

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & market;
		h & hero;
		h & mode;
		h & r1;
		h & r2;
		h & val;
	}
};

struct SetFormation : public CPackForServer
{
	SetFormation():formation(0){};
	SetFormation(ObjectInstanceID HID, ui8 Formation):hid(HID),formation(Formation){};
	ObjectInstanceID hid;
	ui8 formation;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid;
		h & formation;
	}
};

struct HireHero : public CPackForServer
{
	HireHero():hid(0){};
	HireHero(si32 HID, ObjectInstanceID TID):hid(HID),tid(TID){};
	si32 hid; //available hero serial
	ObjectInstanceID tid; //town (tavern) id
	PlayerColor player;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid;
		h & tid;
		h & player;
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
	QueryReply(){};
	QueryReply(QueryID QID, const JsonNode & Reply):qid(QID), reply(Reply){};
	QueryID qid;
	PlayerColor player;
	JsonNode reply;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & qid;
		h & player;
		h & reply;
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
		h & hid;
		h & sid;
		h & pos;
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

struct PlayerMessage : public CPackForClient, public CPackForServer
{
	PlayerMessage(){};
	PlayerMessage(PlayerColor Player, const std::string &Text, ObjectInstanceID obj)
		:player(Player),text(Text), currObj(obj)
	{};
	void applyCl(CClient *cl);
	void applyGs(CGameState *gs){};
	bool applyGh(CGameHandler *gh);

	PlayerColor player;
	std::string text;
	ObjectInstanceID currObj; // optional parameter that specifies current object. For cheats :)

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & text;
		h & player;
		h & currObj;
	}
};

struct CenterView : public CPackForClient
{
	CenterView():focusTime(0){};
	void applyCl(CClient *cl);

	PlayerColor player;
	int3 pos;
	ui32 focusTime; //ms

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & pos;
		h & player;
		h & focusTime;
	}
};

/***********************************************************************************************************/

struct CPackForSelectionScreen : public CPack
{
	void apply(CSelectionScreen *selScreen) {} // implemented in CPreGame.cpp
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
		h & playerName;
		h & message;
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
		h & playerName;
		h & connectionID;
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
	RequestOptionsChange()
		:what(0), direction(0), playerID(0)
	{}

	void apply(CSelectionScreen *selScreen); //that functions are implemented in CPreGame.cpp

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & what;
		h & direction;
		h & playerID;
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
