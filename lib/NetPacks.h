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
#include "mapObjects/CGHeroInstance.h"
#include "ConstTransitivePtr.h"
#include "int3.h"
#include "ResourceSet.h"
#include "CGameStateFwd.h"
#include "mapping/CMapDefines.h"
#include "battle/CObstacleInstance.h"

#include "spells/ViewSpellInt.h"

class CClient;
class CGameHandler;

VCMI_LIB_NAMESPACE_BEGIN

class CGameState;
class CArtifact;
class CGObjectInstance;
class CArtifactInstance;
struct StackLocation;
struct ArtSlotInfo;
struct QuestInfo;
class IBattleState;

struct Query : public CPackForClient
{
	QueryID queryID; // equals to -1 if it is not an actual query (and should not be answered)
};

struct StackLocation
{
	ConstTransitivePtr<CArmedInstance> army;
	SlotID slot;

	StackLocation() = default;
	StackLocation(const CArmedInstance * Army, const SlotID & Slot)
		: army(const_cast<CArmedInstance *>(Army))  //we are allowed here to const cast -> change will go through one of our packages... do not abuse!
		, slot(Slot)
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
	PackageApplied() = default;
	PackageApplied(ui8 Result)
		: result(Result)
	{
	}
	void applyCl(CClient *cl);

	ui8 result = 0; //0 - something went wrong, request hasn't been realized; 1 - OK
	ui32 packType = 0; //type id of applied package
	ui32 requestID = 0; //an ID given by client to the request that was applied
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
	SystemMessage(std::string Text)
		: text(std::move(Text))
	{
	}
	SystemMessage() = default;
	void applyCl(CClient *cl);

	std::string text;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & text;
	}
};

struct PlayerBlocked : public CPackForClient
{
	void applyCl(CClient *cl);

	enum EReason { UPCOMING_BATTLE, ONGOING_MOVEMENT };
	enum EMode { BLOCKADE_STARTED, BLOCKADE_ENDED };

	EReason reason = UPCOMING_BATTLE;
	EMode startOrEnd = BLOCKADE_STARTED;
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
	DLL_LINKAGE void applyGs(CGameState * gs) const;

	PlayerColor player;
	bool losingCheatCode = false;
	bool winningCheatCode = false;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player;
		h & losingCheatCode;
		h & winningCheatCode;
	}
};

struct YourTurn : public CPackForClient
{
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

	PlayerColor player;
	boost::optional<ui8> daysWithoutCastle;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player;
		h & daysWithoutCastle;
	}
};

struct EntitiesChanged: public CPackForClient
{
	std::vector<EntityChanges> changes;

	void applyCl(CClient * cl);
	DLL_LINKAGE void applyGs(CGameState * gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & changes;
	}
};

struct SetResources : public CPackForClient
{
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

	bool abs = true; //false - changes by value; 1 - sets to value
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
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

	ui8 abs = 0; //0 - changes by value; 1 - sets to value
	ObjectInstanceID id;
	PrimarySkill::PrimarySkill which = PrimarySkill::ATTACK;
	si64 val = 0;

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
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

	ui8 abs = 0; //0 - changes by value; 1 - sets to value
	ObjectInstanceID id;
	SecondarySkill which;
	ui16 val = 0;

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
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

	ui8 flags = 0; //1 - start
	ObjectInstanceID tid, hid;

	bool start() const //if hero is entering castle (if false - leaving)
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
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ui8 learn = 1; //1 - gives spell, 0 - takes
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
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

	ObjectInstanceID hid;
	si32 val = 0;
	bool absolute = true;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & val;
		h & hid;
		h & absolute;
	}
};

struct SetMovePoints : public CPackForClient
{
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

	ObjectInstanceID hid;
	si32 val = 0;
	bool absolute = true;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & val;
		h & hid;
		h & absolute;
	}
};

struct FoWChange : public CPackForClient
{
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	std::unordered_set<int3, struct ShashInt3 > tiles;
	PlayerColor player;
	ui8 mode = 0; //mode==0 - hide, mode==1 - reveal
	bool waitForDialogs = false;
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
		for(auto & i : army)
			i.clear();
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
		: who(Who)
	{
	}
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	enum {HERO, PLAYER, TOWN};
	ui8 who = 0; //who receives bonus, uses enum above
	si32 id = 0; //hero. town or player id - whoever receives it
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
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID objid;
	int3 nPos;
	ui8 flags = 0; //bit flags: 1 - redraw

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & objid;
		h & nPos;
		h & flags;
	}
};

struct PlayerEndsGame : public CPackForClient
{
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

	PlayerColor player;
	EVictoryLossCheckResult victoryLossCheckResult;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player;
		h & victoryLossCheckResult;
	}
};

struct PlayerReinitInterface : public CPackForClient
{
	void applyCl(CClient * cl);
	DLL_LINKAGE void applyGs(CGameState *gs);
	
	std::vector<PlayerColor> players;
	ui8 playerConnectionId; //PLAYER_AI for AI player
	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & players;
		h & playerConnectionId;
	}
};

struct RemoveBonus :  public CPackForClient
{
	RemoveBonus(ui8 Who = 0)
		: who(Who)
	{
	}

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	enum {HERO, PLAYER, TOWN};
	ui8 who; //who receives bonus, uses enum above
	ui32 whoID = 0; //hero, town or player id - whoever loses bonus

	//vars to identify bonus: its source
	ui8 source = 0;
	ui32 id = 0; //source id

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

struct SetCommanderProperty : public CPackForClient
{
	enum ECommanderProperty {ALIVE, BONUS, SECONDARY_SKILL, EXPERIENCE, SPECIAL_SKILL};

	void applyCl(CClient *cl){}
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID heroid;

	ECommanderProperty which = ALIVE;
	TExpType amount = 0; //0 for dead, >0 for alive
	si32 additionalInfo = 0; //for secondary skills choice
	Bonus accumulatedBonus;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroid;
		h & which;
		h & amount;
		h & additionalInfo;
		h & accumulatedBonus;
	}
};

struct AddQuest : public CPackForClient
{
	void applyCl(CClient *cl){}
	DLL_LINKAGE void applyGs(CGameState * gs) const;

	PlayerColor player;
	QuestInfo quest;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player;
		h & quest;
	}
};

struct UpdateArtHandlerLists : public CPackForClient
{
	std::vector<CArtifact*> treasures, minors, majors, relics;

	DLL_LINKAGE void applyGs(CGameState * gs) const;
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

	std::list<CMapEvent> events;
	DLL_LINKAGE void applyGs(CGameState * gs) const;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & events;
	}
};

struct UpdateCastleEvents : public CPackForClient
{

	ObjectInstanceID town;
	std::list<CCastleEvent> events;

	DLL_LINKAGE void applyGs(CGameState * gs) const;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & town;
		h & events;
	}
};

struct ChangeFormation : public CPackForClient
{

	ObjectInstanceID hid;
	ui8 formation = 0;

	DLL_LINKAGE void applyGs(CGameState * gs) const;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hid;
		h & formation;
	}
};

struct RemoveObject : public CPackForClient
{
	RemoveObject() = default;
	RemoveObject(const ObjectInstanceID & ID)
		: id(ID)
	{}
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
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	void applyGs(CGameState *gs);

	enum EResult
	{
		FAILED,
		SUCCESS,
		TELEPORTATION,
		RESERVED_,
		BLOCKING_VISIT,
		EMBARK,
		DISEMBARK
	};

	ObjectInstanceID id;
	ui32 movePoints = 0;
	EResult result = FAILED; //uses EResult
	int3 start, end; //h3m format
	std::unordered_set<int3, ShashInt3> fowRevealed; //revealed tiles
	boost::optional<int3> attackedFrom; // Set when stepping into endangered tile.

	bool humanKnows = false; //used locally during applying to client

	bool stopMovement() const;

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
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID tid;
	std::set<BuildingID> bid;
	si16 builded = 0;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid;
		h & bid;
		h & builded;
	}
};

struct RazeStructures : public CPackForClient
{
	void applyCl (CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	ObjectInstanceID tid;
	std::set<BuildingID> bid;
	si16 destroyed = 0;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & tid;
		h & bid;
		h & destroyed;
	}
};

struct SetAvailableCreatures : public CPackForClient
{
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

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
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

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
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

	si32 hid = -1; //subID of hero
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
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

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
	void applyCl(CClient *cl);

	enum EWindow {EXCHANGE_WINDOW, RECRUITMENT_FIRST, RECRUITMENT_ALL, SHIPYARD_WINDOW, THIEVES_GUILD,
	              UNIVERSITY_WINDOW, HILL_FORT_WINDOW, MARKET_WINDOW, PUZZLE_MAP, TAVERN_WINDOW};
	ui8 window;
	si32 id1 = -1;
	si32 id2 = -1;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & window;
		h & id1;
		h & id2;
	}
};

struct NewObject : public CPackForClient
{
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	Obj ID;
	ui32 subID = 0;
	int3 pos;

	ObjectInstanceID id; //used locally, filled during applyGs

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ID;
		h & subID;
		h & pos;
	}
};

struct SetAvailableArtifacts : public CPackForClient
{
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

	si32 id = 0; //two variants: id < 0: set artifact pool for Artifact Merchants in towns; id >= 0: set pool for adv. map Black Market (id is the id of Black Market instance then)
	std::vector<const CArtifact *> arts;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & id;
		h & arts;
	}
};

struct NewArtifact : public CPackForClient
{
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

struct ChangeStackCount : CGarrisonOperationPack
{
	ObjectInstanceID army;
	SlotID slot;
	TQuantity count;
	bool absoluteValue; //if not -> count will be added (or subtracted if negative)

	void applyCl(CClient * cl);
	DLL_LINKAGE void applyGs(CGameState * gs);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & army;
		h & slot;
		h & count;
		h & absoluteValue;
	}
};

struct SetStackType : CGarrisonOperationPack
{
	ObjectInstanceID army;
	SlotID slot;
	CreatureID type;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & army;
		h & slot;
		h & type;
	}
};

struct EraseStack : CGarrisonOperationPack
{
	ObjectInstanceID army;
	SlotID slot;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & army;
		h & slot;
	}
};

struct SwapStacks : CGarrisonOperationPack
{
	ObjectInstanceID srcArmy;
	ObjectInstanceID dstArmy;
	SlotID srcSlot;
	SlotID dstSlot;

	void applyCl(CClient * cl);
	DLL_LINKAGE void applyGs(CGameState * gs);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & srcArmy;
		h & dstArmy;
		h & srcSlot;
		h & dstSlot;
	}
};

struct InsertNewStack : CGarrisonOperationPack
{
	ObjectInstanceID army;
	SlotID slot;
	CreatureID type;
	TQuantity count = 0;

	void applyCl(CClient * cl);
	DLL_LINKAGE void applyGs(CGameState * gs);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & army;
		h & slot;
		h & type;
		h & count;
	}
};

///moves creatures from src stack to dst slot, may be used for merging/splittint/moving stacks
struct RebalanceStacks : CGarrisonOperationPack
{
	ObjectInstanceID srcArmy;
	ObjectInstanceID dstArmy;
	SlotID srcSlot;
	SlotID dstSlot;

	TQuantity count;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & srcArmy;
		h & dstArmy;
		h & srcSlot;
		h & dstSlot;
		h & count;
	}
};

struct BulkRebalanceStacks : CGarrisonOperationPack
{
	std::vector<RebalanceStacks> moves;

	void applyCl(CClient * cl);
	DLL_LINKAGE void applyGs(CGameState * gs);

	template <typename Handler> 
	void serialize(Handler & h, const int version)
	{
		h & moves;
	}
};

struct BulkSmartRebalanceStacks : CGarrisonOperationPack
{
	std::vector<RebalanceStacks> moves;
	std::vector<ChangeStackCount> changes;

	void applyCl(CClient * cl);
	DLL_LINKAGE void applyGs(CGameState * gs);

	template <typename Handler> 
	void serialize(Handler & h, const int version)
	{
		h & moves;
		h & changes;
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

struct CArtifactOperationPack : CPackForClient
{
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
	MoveArtifact() = default;
	MoveArtifact(ArtifactLocation * src, ArtifactLocation * dst, bool askAssemble = true)
		: src(*src), dst(*dst), askAssemble(askAssemble)
	{}
	ArtifactLocation src, dst;
	bool askAssemble = true;

	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & src;
		h & dst;
		h & askAssemble;
	}
};

struct BulkMoveArtifacts : CArtifactOperationPack
{
	struct LinkedSlots
	{
		ArtifactPosition srcPos;
		ArtifactPosition dstPos;

		LinkedSlots() = default;
		LinkedSlots(const ArtifactPosition & srcPos, const ArtifactPosition & dstPos)
			: srcPos(srcPos)
			, dstPos(dstPos)
		{
		}
		template <typename Handler> void serialize(Handler & h, const int version)
		{
			h & srcPos;
			h & dstPos;
		}
	};

	TArtHolder srcArtHolder;
	TArtHolder dstArtHolder;

	BulkMoveArtifacts()
		: swap(false) {}
	BulkMoveArtifacts(TArtHolder srcArtHolder, TArtHolder dstArtHolder, bool swap)
		: srcArtHolder(std::move(std::move(srcArtHolder)))
		, dstArtHolder(std::move(std::move(dstArtHolder)))
		, swap(swap)
	{
	}

	void applyCl(CClient * cl);
	DLL_LINKAGE void applyGs(CGameState * gs);

	std::vector<LinkedSlots> artsPack0;
	std::vector<LinkedSlots> artsPack1;
	bool swap;
	CArtifactSet * getSrcHolderArtSet();
	CArtifactSet * getDstHolderArtSet();
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & artsPack0;
		h & artsPack1;
		h & srcArtHolder;
		h & dstArtHolder;
		h & swap;
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

struct HeroVisit : public CPackForClient
{
	PlayerColor player;
	ObjectInstanceID heroId;
	ObjectInstanceID objId;

	bool starting; //false -> ending

	void applyCl(CClient * cl);
	DLL_LINKAGE void applyGs(CGameState * gs);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & player;
		h & heroId;
		h & objId;
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
	ui32 day = 0;
	ui8 specialWeek = 0; //weekType
	CreatureID creatureid; //for creature weeks

	NewTurn() = default;

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
	ui16 soundID = 0;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & text;
		h & components;
		h & player;
		h & soundID;
	}
	InfoWindow() = default;
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
		REWARD_RANDOMIZE, REWARD_SELECT, REWARD_CLEARED
	};
}

struct SetObjectProperty : public CPackForClient
{
	DLL_LINKAGE void applyGs(CGameState * gs) const;
	void applyCl(CClient *cl);

	ObjectInstanceID id;
	ui8 what = 0; // see ObjProperty enum
	ui32 val = 0;
	SetObjectProperty() = default;
	SetObjectProperty(const ObjectInstanceID & ID, ui8 What, ui32 Val)
		: id(ID)
		, what(What)
		, val(Val)
	{}

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
	ui32 mode = VISITOR_CLEAR; // uses VisitMode enum
	ObjectInstanceID object;
	ObjectInstanceID hero; // note: hero owner will be also marked as "visited" this object

	DLL_LINKAGE void applyGs(CGameState * gs) const;

	ChangeObjectVisitors() = default;

	ChangeObjectVisitors(ui32 mode, const ObjectInstanceID & object, const ObjectInstanceID & heroID = ObjectInstanceID(-1))
		: mode(mode)
		, object(object)
		, hero(heroID)
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
	ObjectInstanceID heroId;

	/// Do not serialize, used by server only
	std::vector<SecondarySkill> skills;

	DLL_LINKAGE void applyGs(CGameState * gs);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & heroId;
	}
};

struct HeroLevelUp : public Query
{
	PlayerColor player;
	ObjectInstanceID heroId;

	PrimarySkill::PrimarySkill primskill = PrimarySkill::ATTACK;
	std::vector<SecondarySkill> skills;

	void applyCl(CClient * cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & queryID;
		h & player;
		h & heroId;
		h & primskill;
		h & skills;
	}
};

struct CommanderLevelUp : public Query
{
	PlayerColor player;
	ObjectInstanceID heroId;

	std::vector<ui32> skills; //0-5 - secondary skills, val-100 - special skill

	void applyCl(CClient * cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & queryID;
		h & player;
		h & heroId;
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
	ui8 flags = 0;
	ui16 soundID = 0;

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
		if(yesno) flags |= ALLOW_CANCEL;
		if(Selection) flags |= SELECTION;
	}
	BlockingDialog() = default;

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
	void applyCl(CClient *cl);
	ObjectInstanceID objid, hid;
	bool removableUnits = false;

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
	void applyCl(CClient *cl);

	PlayerColor player;

	ObjectInstanceID hero1;
	ObjectInstanceID hero2;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & queryID;
		h & player;
		h & hero1;
		h & hero2;
	}
};

struct TeleportDialog : public Query
{
	TeleportDialog() = default;

	TeleportDialog(const PlayerColor & Player, const TeleportChannelID & Channel)
		: player(Player)
		, channel(Channel)
	{}

	void applyCl(CClient *cl);

	PlayerColor player;
	TeleportChannelID channel;
	TTeleportExitsList exits;
	bool impassable = false;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & queryID;
		h & player;
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

class BattleInfo;
struct BattleStart : public CPackForClient
{
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

	BattleInfo * info = nullptr;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & info;
	}
};

struct BattleNextRound : public CPackForClient
{
	void applyFirstCl(CClient *cl);
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;
	si32 round = 0;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & round;
	}
};

struct BattleSetActiveStack : public CPackForClient
{
	void applyCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState * gs) const;

	ui32 stack = 0;
	ui8 askPlayerInterface = true;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stack;
		h & askPlayerInterface;
	}
};

struct BattleResult : public CPackForClient
{
	enum EResult {NORMAL = 0, ESCAPE = 1, SURRENDER = 2};

	void applyFirstCl(CClient *cl);
	void applyGs(CGameState *gs);

	EResult result = NORMAL;
	ui8 winner = 2; //0 - attacker, 1 - defender, [2 - draw (should be possible?)]
	std::map<ui32,si32> casualties[2]; //first => casualties of attackers - map crid => number
	TExpType exp[2] = {0, 0}; //exp for attacker and defender
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

struct BattleLogMessage : public CPackForClient
{
	std::vector<MetaString> lines;

	void applyCl(CClient * cl);
	DLL_LINKAGE void applyGs(CGameState * gs);
	DLL_LINKAGE void applyBattle(IBattleState * battleState);

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & lines;
	}
};

struct BattleStackMoved : public CPackForClient
{
	ui32 stack = 0;
	std::vector<BattleHex> tilesToMove;
	int distance = 0;
	bool teleporting = false;
	void applyFirstCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);
	DLL_LINKAGE void applyBattle(IBattleState * battleState);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stack;
		h & tilesToMove;
		h & distance;
		h & teleporting;
	}
};

struct BattleUnitsChanged : public CPackForClient
{
	DLL_LINKAGE void applyGs(CGameState *gs);
	DLL_LINKAGE void applyBattle(IBattleState * battleState);
	void applyCl(CClient *cl);

	std::vector<UnitChanges> changedStacks;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & changedStacks;
	}
};

struct BattleStackAttacked
{
	DLL_LINKAGE void applyGs(CGameState *gs);
	DLL_LINKAGE void applyBattle(IBattleState * battleState);

	ui32 stackAttacked = 0, attackerID = 0;
	ui32 killedAmount = 0;
	int64_t damageAmount = 0;
	UnitChanges newState;
	enum EFlags {KILLED = 1, SECONDARY = 2, REBIRTH = 4, CLONE_KILLED = 8, SPELL_EFFECT = 16, FIRE_SHIELD = 32, };
	ui32 flags = 0; //uses EFlags (above)
	SpellID spellID = SpellID::NONE; //only if flag SPELL_EFFECT is set

	bool killed() const//if target stack was killed
	{
		return flags & KILLED || flags & CLONE_KILLED;
	}
	bool cloneKilled() const
	{
		return flags & CLONE_KILLED;
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
	bool fireShield() const
	{
		return flags & FIRE_SHIELD;
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stackAttacked;
		h & attackerID;
		h & newState;
		h & flags;
		h & killedAmount;
		h & damageAmount;
		h & spellID;
	}
	bool operator<(const BattleStackAttacked &b) const
	{
		return stackAttacked < b.stackAttacked;
	}
};

struct BattleAttack : public CPackForClient
{
	void applyFirstCl(CClient *cl);
	DLL_LINKAGE void applyGs(CGameState *gs);
	void applyCl(CClient *cl);

	BattleUnitsChanged attackerChanges;

	std::vector<BattleStackAttacked> bsa;
	ui32 stackAttacking = 0;
	ui32 flags = 0; //uses Eflags (below)
	enum EFlags{SHOT = 1, COUNTER = 2, LUCKY = 4, UNLUCKY = 8, BALLISTA_DOUBLE_DMG = 16, DEATH_BLOW = 32, SPELL_LIKE = 64, LIFE_DRAIN = 128};

	BattleHex tile;
	SpellID spellID = SpellID::NONE; //for SPELL_LIKE

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
	bool lifeDrain() const
	{
		return flags & LIFE_DRAIN;
	}
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & bsa;
		h & stackAttacking;
		h & flags;
		h & tile;
		h & spellID;
		h & attackerChanges;
	}
};

struct StartAction : public CPackForClient
{
	StartAction() = default;
	StartAction(BattleAction act)
		: ba(std::move(act))
	{}
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
	void applyCl(CClient *cl);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
	}
};

struct BattleSpellCast : public CPackForClient
{
	DLL_LINKAGE void applyGs(CGameState * gs) const;
	void applyCl(CClient *cl);

	bool activeCast = true;
	ui8 side = 0; //which hero did cast spell: 0 - attacker, 1 - defender
	SpellID spellID; //id of spell
	ui8 manaGained = 0; //mana channeling ability
	BattleHex tile; //destination tile (may not be set in some global/mass spells
	std::set<ui32> affectedCres; //ids of creatures affected by this spell, generally used if spell does not set any effect (like dispel or cure)
	std::set<ui32> resistedCres; // creatures that resisted the spell (e.g. Dwarves)
	std::set<ui32> reflectedCres; // creatures that reflected the spell (e.g. Magic Mirror spell)
	si32 casterStack = -1; // -1 if not cated by creature, >=0 caster stack ID
	bool castByHero = true; //if true - spell has been cast by hero, otherwise by a creature

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & side;
		h & spellID;
		h & manaGained;
		h & tile;
		h & affectedCres;
		h & resistedCres;
		h & reflectedCres;
		h & casterStack;
		h & castByHero;
		h & activeCast;
	}
};

struct SetStackEffect : public CPackForClient
{
	DLL_LINKAGE void applyGs(CGameState * gs);
	DLL_LINKAGE void applyBattle(IBattleState * battleState);
	void applyCl(CClient * cl);

	std::vector<std::pair<ui32, std::vector<Bonus>>> toAdd;
	std::vector<std::pair<ui32, std::vector<Bonus>>> toUpdate;
	std::vector<std::pair<ui32, std::vector<Bonus>>> toRemove;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & toAdd;
		h & toUpdate;
		h & toRemove;
	}
};

struct StacksInjured : public CPackForClient
{
	DLL_LINKAGE void applyGs(CGameState * gs);
	DLL_LINKAGE void applyBattle(IBattleState * battleState);

	void applyCl(CClient * cl);

	std::vector<BattleStackAttacked> stacks;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & stacks;
	}
};

struct BattleResultsApplied : public CPackForClient
{
	PlayerColor player1, player2;

	void applyCl(CClient *cl);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player1;
		h & player2;
	}
};

struct BattleObstaclesChanged : public CPackForClient
{
	DLL_LINKAGE void applyGs(CGameState * gs);
	DLL_LINKAGE void applyBattle(IBattleState * battleState);
	void applyCl(CClient * cl);

	std::vector<ObstacleChanges> changes;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & changes;
	}
};

struct ELF_VISIBILITY CatapultAttack : public CPackForClient
{
	struct AttackInfo
	{
		si16 destinationTile;
		EWallPart attackedPart;
		ui8 damageDealt;

		template <typename Handler> void serialize(Handler & h, const int version)
		{
			h & destinationTile;
			h & attackedPart;
			h & damageDealt;
		}
	};

	DLL_LINKAGE CatapultAttack();
	DLL_LINKAGE ~CatapultAttack() override;

	DLL_LINKAGE void applyGs(CGameState * gs);
	DLL_LINKAGE void applyBattle(IBattleState * battleState);
	void applyCl(CClient * cl);

	std::vector< AttackInfo > attackedParts;
	int attacker = -1; //if -1, then a spell caused this

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & attackedParts;
		h & attacker;
	}
};

struct BattleSetStackProperty : public CPackForClient
{
	enum BattleStackProperty {CASTS, ENCHANTER_COUNTER, UNBIND, CLONED, HAS_CLONE};

	DLL_LINKAGE void applyGs(CGameState * gs) const;

	int stackID = 0;
	BattleStackProperty which = CASTS;
	int val = 0;
	int absolute = 0;

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
	DLL_LINKAGE void applyGs(CGameState * gs) const; //effect
	void applyCl(CClient *cl); //play animations & stuff

	int stackID = 0;
	int effect = 0; //use corresponding Bonus type
	int val = 0;
	int additionalInfo = 0;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & stackID;
		h & effect;
		h & val;
		h & additionalInfo;
	}
};

struct BattleUpdateGateState : public CPackForClient
{
	void applyFirstCl(CClient *cl);

	DLL_LINKAGE void applyGs(CGameState * gs) const;

	EGateState state = EGateState::NONE;
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & state;
	}
};

struct ShowInInfobox : public CPackForClient
{
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
	ObjectInstanceID casterID;
	SpellID spellID;

	void applyCl(CClient *cl);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & casterID;
		h & spellID;
	}
};

struct ShowWorldViewEx : public CPackForClient
{
	PlayerColor player;

	std::vector<ObjectPosInfo> objectPositions;

	void applyCl(CClient *cl);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player;
		h & objectPositions;
	}
};

/***********************************************************************************************************/

struct EndTurn : public CPackForServer
{
	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
	}
};

struct DismissHero : public CPackForServer
{
	DismissHero() = default;
	DismissHero(const ObjectInstanceID & HID)
		: hid(HID)
	{}
	ObjectInstanceID hid;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & hid;
	}
};

struct MoveHero : public CPackForServer
{
	MoveHero() = default;
	MoveHero(const int3 & Dest, const ObjectInstanceID & HID, bool Transit)
		: dest(Dest)
		, hid(HID)
		, transit(Transit)
	{}
	int3 dest;
	ObjectInstanceID hid;
	bool transit = false;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & dest;
		h & hid;
		h & transit;
	}
};

struct CastleTeleportHero : public CPackForServer
{
	CastleTeleportHero() = default;
	CastleTeleportHero(const ObjectInstanceID & HID, const ObjectInstanceID & Dest, ui8 Source)
		: dest(Dest)
		, hid(HID)
		, source(Source)
	{}
	ObjectInstanceID dest;
	ObjectInstanceID hid;
	si8 source = 0; //who give teleporting, 1=castle gate

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & dest;
		h & hid;
	}
};

struct ArrangeStacks : public CPackForServer
{
	ArrangeStacks() = default;
	ArrangeStacks(ui8 W, const SlotID & P1, const SlotID & P2, const ObjectInstanceID & ID1, const ObjectInstanceID & ID2, si32 VAL)
		: what(W)
		, p1(P1)
		, p2(P2)
		, id1(ID1)
		, id2(ID2)
		, val(VAL)
	{}

	ui8 what = 0; //1 - swap; 2 - merge; 3 - split
	SlotID p1, p2; //positions of first and second stack
	ObjectInstanceID id1, id2; //ids of objects with garrison
	si32 val = 0;
	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & what;
		h & p1;
		h & p2;
		h & id1;
		h & id2;
		h & val;
	}
};

struct BulkMoveArmy : public CPackForServer
{
	SlotID srcSlot;
	ObjectInstanceID srcArmy;
	ObjectInstanceID destArmy;

	BulkMoveArmy() = default;

	BulkMoveArmy(const ObjectInstanceID & srcArmy, const ObjectInstanceID & destArmy, const SlotID & srcSlot)
		: srcArmy(srcArmy)
		, destArmy(destArmy)
		, srcSlot(srcSlot)
	{}

	bool applyGh(CGameHandler * gh);

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer&>(*this);
		h & srcSlot;
		h & srcArmy;
		h & destArmy;
	}
};

struct BulkSplitStack : public CPackForServer
{
	SlotID src;
	ObjectInstanceID srcOwner;
	si32 amount = 0;

	BulkSplitStack() = default;

	BulkSplitStack(const ObjectInstanceID & srcOwner, const SlotID & src, si32 howMany)
		: src(src)
		, srcOwner(srcOwner)
		, amount(howMany)
	{}

	bool applyGh(CGameHandler * gh);

	template <typename Handler> 
	void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer&>(*this);
		h & src;
		h & srcOwner;
		h & amount;
	}
};

struct BulkMergeStacks : public CPackForServer
{
	SlotID src;
	ObjectInstanceID srcOwner;

	BulkMergeStacks() = default;

	BulkMergeStacks(const ObjectInstanceID & srcOwner, const SlotID & src)
		: src(src)
		, srcOwner(srcOwner)
	{}

	bool applyGh(CGameHandler * gh);

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer&>(*this);
		h & src;
		h & srcOwner;
	}
};

struct BulkSmartSplitStack : public CPackForServer
{
	SlotID src;
	ObjectInstanceID srcOwner;

	BulkSmartSplitStack() = default;

	BulkSmartSplitStack(const ObjectInstanceID & srcOwner, const SlotID & src)
		: src(src)
		, srcOwner(srcOwner)
	{}

	bool applyGh(CGameHandler * gh);

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer&>(*this);
		h & src;
		h & srcOwner;
	}
};

struct DisbandCreature : public CPackForServer
{
	DisbandCreature() = default;
	DisbandCreature(const SlotID & Pos, const ObjectInstanceID & ID)
		: pos(Pos)
		, id(ID)
	{}
	SlotID pos; //stack pos
	ObjectInstanceID id; //object id

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & pos;
		h & id;
	}
};

struct BuildStructure : public CPackForServer
{
	BuildStructure() = default;
	BuildStructure(const ObjectInstanceID & TID, const BuildingID & BID)
		: tid(TID)
		, bid(BID)
	{}
	ObjectInstanceID tid; //town id
	BuildingID bid; //structure id

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & tid;
		h & bid;
	}
};

struct RazeStructure : public BuildStructure
{
	bool applyGh(CGameHandler *gh);
};

struct RecruitCreatures : public CPackForServer
{
	RecruitCreatures() = default;
	RecruitCreatures(const ObjectInstanceID & TID, const ObjectInstanceID & DST, const CreatureID & CRID, si32 Amount, si32 Level)
		: tid(TID)
		, dst(DST)
		, crid(CRID)
		, amount(Amount)
		, level(Level)
	{}
	ObjectInstanceID tid; //dwelling id, or town
	ObjectInstanceID dst; //destination ID, e.g. hero
	CreatureID crid;
	ui32 amount = 0; //creature amount
	si32 level = 0; //dwelling level to buy from, -1 if any
	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & tid;
		h & dst;
		h & crid;
		h & amount;
		h & level;
	}
};

struct UpgradeCreature : public CPackForServer
{
	UpgradeCreature() = default;
	UpgradeCreature(const SlotID & Pos, const ObjectInstanceID & ID, const CreatureID & CRID)
		: pos(Pos)
		, id(ID)
		, cid(CRID)
	{}
	SlotID pos; //stack pos
	ObjectInstanceID id; //object id
	CreatureID cid; //id of type to which we want make upgrade

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & pos;
		h & id;
		h & cid;
	}
};

struct GarrisonHeroSwap : public CPackForServer
{
	GarrisonHeroSwap() = default;
	GarrisonHeroSwap(const ObjectInstanceID & TID)
		: tid(TID)
	{}
	ObjectInstanceID tid;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & tid;
	}
};

struct ExchangeArtifacts : public CPackForServer
{
	ArtifactLocation src, dst;
	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & src;
		h & dst;
	}
};

struct BulkExchangeArtifacts : public CPackForServer
{
	ObjectInstanceID srcHero;
	ObjectInstanceID dstHero;
	bool swap = false;

	BulkExchangeArtifacts() = default;
	BulkExchangeArtifacts(const ObjectInstanceID & srcHero, const ObjectInstanceID & dstHero, bool swap)
		: srcHero(srcHero)
		, dstHero(dstHero)
		, swap(swap)
	{
	}

	bool applyGh(CGameHandler * gh);
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer&>(*this);
		h & srcHero;
		h & dstHero;
		h & swap;
	}
};

struct AssembleArtifacts : public CPackForServer
{
	AssembleArtifacts() = default;
	AssembleArtifacts(const ObjectInstanceID & _heroID, const ArtifactPosition & _artifactSlot, bool _assemble, const ArtifactID & _assembleTo)
		: heroID(_heroID)
		, artifactSlot(_artifactSlot)
		, assemble(_assemble)
		, assembleTo(_assembleTo)
	{}
	ObjectInstanceID heroID;
	ArtifactPosition artifactSlot;
	bool assemble = false; // True to assemble artifact, false to disassemble.
	ArtifactID assembleTo; // Artifact to assemble into.

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & heroID;
		h & artifactSlot;
		h & assemble;
		h & assembleTo;
	}
};

struct BuyArtifact : public CPackForServer
{
	BuyArtifact() = default;
	BuyArtifact(const ObjectInstanceID & HID, const ArtifactID & AID)
		: hid(HID)
		, aid(AID)
	{}
	ObjectInstanceID hid;
	ArtifactID aid;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & hid;
		h & aid;
	}
};

struct TradeOnMarketplace : public CPackForServer
{
	ObjectInstanceID marketId;
	ObjectInstanceID heroId;

	EMarketMode::EMarketMode mode = EMarketMode::RESOURCE_RESOURCE;
	std::vector<ui32> r1, r2; //mode 0: r1 - sold resource, r2 - bought res (exception: when sacrificing art r1 is art id [todo: make r2 preferred slot?]
	std::vector<ui32> val; //units of sold resource

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & marketId;
		h & heroId;
		h & mode;
		h & r1;
		h & r2;
		h & val;
	}
};

struct SetFormation : public CPackForServer
{
	SetFormation() = default;
	;
	SetFormation(const ObjectInstanceID & HID, ui8 Formation)
		: hid(HID)
		, formation(Formation)
	{}
	ObjectInstanceID hid;
	ui8 formation = 0;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & hid;
		h & formation;
	}
};

struct HireHero : public CPackForServer
{
	HireHero() = default;
	HireHero(si32 HID, const ObjectInstanceID & TID)
		: hid(HID)
		, tid(TID)
	{}
	si32 hid = 0; //available hero serial
	ObjectInstanceID tid; //town (tavern) id
	PlayerColor player;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & hid;
		h & tid;
		h & player;
	}
};

struct BuildBoat : public CPackForServer
{
	ObjectInstanceID objid; //where player wants to buy a boat

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & objid;
	}
};

struct QueryReply : public CPackForServer
{
	QueryReply() = default;
	QueryReply(const QueryID & QID, const JsonNode & Reply)
		: qid(QID)
		, reply(Reply)
	{}
	QueryID qid;
	PlayerColor player;
	JsonNode reply;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & qid;
		h & player;
		h & reply;
	}
};

struct MakeAction : public CPackForServer
{
	MakeAction() = default;
	MakeAction(BattleAction BA)
		: ba(std::move(BA))
	{}
	BattleAction ba;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & ba;
	}
};

struct MakeCustomAction : public CPackForServer
{
	MakeCustomAction() = default;
	MakeCustomAction(BattleAction BA)
		: ba(std::move(BA))
	{}
	BattleAction ba;

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & ba;
	}
};

struct DigWithHero : public CPackForServer
{
	ObjectInstanceID id; //digging hero id

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & id;
	}
};

struct CastAdvSpell : public CPackForServer
{
	ObjectInstanceID hid; //hero id
	SpellID sid; //spell id
	int3 pos; //selected tile (not always used)

	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & hid;
		h & sid;
		h & pos;
	}
};

/***********************************************************************************************************/

struct SaveGame : public CPackForServer
{
	SaveGame() = default;
	SaveGame(std::string Fname)
		: fname(std::move(Fname))
	{}
	std::string fname;

	void applyGs(CGameState *gs){}
	bool applyGh(CGameHandler *gh);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & fname;
	}
};

// TODO: Eventually we should re-merge both SaveGame and PlayerMessage
struct SaveGameClient : public CPackForClient
{
	SaveGameClient() = default;
	SaveGameClient(std::string Fname)
		: fname(std::move(Fname))
	{}
	std::string fname;

	void applyCl(CClient *cl);
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & fname;
	}
};

struct PlayerMessage : public CPackForServer
{
	PlayerMessage() = default;
	PlayerMessage(std::string Text, const ObjectInstanceID & obj)
		: text(std::move(Text))
		, currObj(obj)
	{}
	void applyGs(CGameState *gs){}
	bool applyGh(CGameHandler *gh);

	std::string text;
	ObjectInstanceID currObj; // optional parameter that specifies current object. For cheats :)

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & text;
		h & currObj;
	}
};

struct PlayerMessageClient : public CPackForClient
{
	PlayerMessageClient() = default;
	PlayerMessageClient(const PlayerColor & Player, std::string Text)
		: player(Player)
		, text(std::move(Text))
	{}
	void applyCl(CClient *cl);

	PlayerColor player;
	std::string text;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & player;
		h & text;
	}
};

struct CenterView : public CPackForClient
{
	void applyCl(CClient *cl);

	PlayerColor player;
	int3 pos;
	ui32 focusTime = 0; //ms

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & pos;
		h & player;
		h & focusTime;
	}
};

VCMI_LIB_NAMESPACE_END
