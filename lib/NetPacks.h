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

#include "ConstTransitivePtr.h"
#include "MetaString.h"
#include "ResourceSet.h"
#include "TurnTimerInfo.h"
#include "int3.h"

#include "battle/BattleAction.h"
#include "battle/CObstacleInstance.h"
#include "gameState/EVictoryLossCheckResult.h"
#include "gameState/TavernSlot.h"
#include "gameState/QuestInfo.h"
#include "mapObjects/CGHeroInstance.h"
#include "mapping/CMapDefines.h"
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

// This one teleport-specific, but has to be available everywhere in callbacks and netpacks
// For now it's will be there till teleports code refactored and moved into own file
using TTeleportExitsList = std::vector<std::pair<ObjectInstanceID, int3>>;

struct DLL_LINKAGE Query : public CPackForClient
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

	DLL_LINKAGE const CStackInstance * getStack();
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & army;
		h & slot;
	}
};

/***********************************************************************************************************/
struct DLL_LINKAGE PackageApplied : public CPackForClient
{
	PackageApplied() = default;
	PackageApplied(ui8 Result)
		: result(Result)
	{
	}
	virtual void visitTyped(ICPackVisitor & visitor) override;

	ui8 result = 0; //0 - something went wrong, request hasn't been realized; 1 - OK
	ui32 packType = 0; //type id of applied package
	ui32 requestID = 0; //an ID given by client to the request that was applied
	PlayerColor player;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & result;
		h & packType;
		h & requestID;
		h & player;
	}
};

struct DLL_LINKAGE SystemMessage : public CPackForClient
{
	SystemMessage(std::string Text)
		: text(std::move(Text))
	{
	}
	SystemMessage() = default;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	std::string text;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & text;
	}
};

struct DLL_LINKAGE PlayerBlocked : public CPackForClient
{
	enum EReason { UPCOMING_BATTLE, ONGOING_MOVEMENT };
	enum EMode { BLOCKADE_STARTED, BLOCKADE_ENDED };

	EReason reason = UPCOMING_BATTLE;
	EMode startOrEnd = BLOCKADE_STARTED;
	PlayerColor player;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & reason;
		h & startOrEnd;
		h & player;
	}
};

struct DLL_LINKAGE PlayerCheated : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	PlayerColor player;
	bool losingCheatCode = false;
	bool winningCheatCode = false;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & player;
		h & losingCheatCode;
		h & winningCheatCode;
	}
};

struct DLL_LINKAGE TurnTimeUpdate : public CPackForClient
{
	void applyGs(CGameState * gs) const;
	
	PlayerColor player;
	TurnTimerInfo turnTimer;
		
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & player;
		h & turnTimer;
	}
};

struct DLL_LINKAGE YourTurn : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	PlayerColor player;
	std::optional<ui8> daysWithoutCastle;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & player;
		h & daysWithoutCastle;
	}
};

struct DLL_LINKAGE EntitiesChanged : public CPackForClient
{
	std::vector<EntityChanges> changes;

	void applyGs(CGameState * gs);

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & changes;
	}
};

struct DLL_LINKAGE SetResources : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	bool abs = true; //false - changes by value; 1 - sets to value
	PlayerColor player;
	TResources res; //res[resid] => res amount

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & abs;
		h & player;
		h & res;
	}
};

struct DLL_LINKAGE SetPrimSkill : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	ui8 abs = 0; //0 - changes by value; 1 - sets to value
	ObjectInstanceID id;
	PrimarySkill::PrimarySkill which = PrimarySkill::ATTACK;
	si64 val = 0;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & abs;
		h & id;
		h & which;
		h & val;
	}
};

struct DLL_LINKAGE SetSecSkill : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	ui8 abs = 0; //0 - changes by value; 1 - sets to value
	ObjectInstanceID id;
	SecondarySkill which;
	ui16 val = 0;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & abs;
		h & id;
		h & which;
		h & val;
	}
};

struct DLL_LINKAGE HeroVisitCastle : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	ui8 flags = 0; //1 - start
	ObjectInstanceID tid, hid;

	bool start() const //if hero is entering castle (if false - leaving)
	{
		return flags & 1;
	}

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & flags;
		h & tid;
		h & hid;
	}
};

struct DLL_LINKAGE ChangeSpells : public CPackForClient
{
	void applyGs(CGameState * gs);

	virtual void visitTyped(ICPackVisitor & visitor) override;

	ui8 learn = 1; //1 - gives spell, 0 - takes
	ObjectInstanceID hid;
	std::set<SpellID> spells;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & learn;
		h & hid;
		h & spells;
	}
};

struct DLL_LINKAGE SetMana : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	ObjectInstanceID hid;
	si32 val = 0;
	bool absolute = true;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & val;
		h & hid;
		h & absolute;
	}
};

struct DLL_LINKAGE SetMovePoints : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	ObjectInstanceID hid;
	si32 val = 0;
	bool absolute = true;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & val;
		h & hid;
		h & absolute;
	}
};

struct DLL_LINKAGE FoWChange : public CPackForClient
{
	void applyGs(CGameState * gs);

	std::unordered_set<int3> tiles;
	PlayerColor player;
	ui8 mode = 0; //mode==0 - hide, mode==1 - reveal
	bool waitForDialogs = false;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & tiles;
		h & player;
		h & mode;
		h & waitForDialogs;
	}
};

struct DLL_LINKAGE SetAvailableHero : public CPackForClient
{
	SetAvailableHero()
	{
		army.clearSlots();
	}
	void applyGs(CGameState * gs);

	TavernHeroSlot slotID;
	TavernSlotRole roleID;
	PlayerColor player;
	HeroTypeID hid; //HeroTypeID::NONE if no hero
	CSimpleArmy army;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & slotID;
		h & roleID;
		h & player;
		h & hid;
		h & army;
	}
};

struct DLL_LINKAGE GiveBonus : public CPackForClient
{
	enum class ETarget : ui8 { HERO, PLAYER, TOWN, BATTLE };
	
	GiveBonus(ETarget Who = ETarget::HERO)
		:who(Who)
	{
	}

	void applyGs(CGameState * gs);

	ETarget who = ETarget::HERO; //who receives bonus
	si32 id = 0; //hero. town or player id - whoever receives it
	Bonus bonus;
	MetaString bdescr;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & bonus;
		h & id;
		h & bdescr;
		h & who;
		assert(id != -1);
	}
};

struct DLL_LINKAGE ChangeObjPos : public CPackForClient
{
	void applyGs(CGameState * gs);

	/// Object to move
	ObjectInstanceID objid;
	/// New position of visitable tile of an object
	int3 nPos;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & objid;
		h & nPos;
	}
};

struct DLL_LINKAGE PlayerEndsGame : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	PlayerColor player;
	EVictoryLossCheckResult victoryLossCheckResult;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & player;
		h & victoryLossCheckResult;
	}
};

struct DLL_LINKAGE PlayerReinitInterface : public CPackForClient
{
	void applyGs(CGameState * gs);

	std::vector<PlayerColor> players;
	ui8 playerConnectionId; //PLAYER_AI for AI player

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & players;
		h & playerConnectionId;
	}
};

struct DLL_LINKAGE RemoveBonus : public CPackForClient
{
	RemoveBonus(GiveBonus::ETarget Who = GiveBonus::ETarget::HERO)
		:who(Who)
	{
	}

	void applyGs(CGameState * gs);

	GiveBonus::ETarget who; //who receives bonus
	ui32 whoID = 0; //hero, town or player id - whoever loses bonus

	//vars to identify bonus: its source
	ui8 source = 0;
	ui32 id = 0; //source id

	//used locally: copy of removed bonus
	Bonus bonus;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & source;
		h & id;
		h & who;
		h & whoID;
	}
};

struct DLL_LINKAGE SetCommanderProperty : public CPackForClient
{
	enum ECommanderProperty { ALIVE, BONUS, SECONDARY_SKILL, EXPERIENCE, SPECIAL_SKILL };

	void applyGs(CGameState * gs);

	ObjectInstanceID heroid;

	ECommanderProperty which = ALIVE;
	TExpType amount = 0; //0 for dead, >0 for alive
	si32 additionalInfo = 0; //for secondary skills choice
	Bonus accumulatedBonus;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & heroid;
		h & which;
		h & amount;
		h & additionalInfo;
		h & accumulatedBonus;
	}
};

struct DLL_LINKAGE AddQuest : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	PlayerColor player;
	QuestInfo quest;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & player;
		h & quest;
	}
};

struct DLL_LINKAGE UpdateArtHandlerLists : public CPackForClient
{
	std::vector<CArtifact *> treasures, minors, majors, relics;

	void applyGs(CGameState * gs) const;
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & treasures;
		h & minors;
		h & majors;
		h & relics;
	}
};

struct DLL_LINKAGE UpdateMapEvents : public CPackForClient
{
	std::list<CMapEvent> events;

	void applyGs(CGameState * gs) const;
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & events;
	}
};

struct DLL_LINKAGE UpdateCastleEvents : public CPackForClient
{
	ObjectInstanceID town;
	std::list<CCastleEvent> events;

	void applyGs(CGameState * gs) const;
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & town;
		h & events;
	}
};

struct DLL_LINKAGE ChangeFormation : public CPackForClient
{
	ObjectInstanceID hid;
	ui8 formation = 0;

	void applyGs(CGameState * gs) const;
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & hid;
		h & formation;
	}
};

struct DLL_LINKAGE RemoveObject : public CPackForClient
{
	RemoveObject() = default;
	RemoveObject(const ObjectInstanceID & ID)
		: id(ID)
	{
	}

	void applyGs(CGameState * gs);
	virtual void visitTyped(ICPackVisitor & visitor) override;

	ObjectInstanceID id;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & id;
	}
};

struct DLL_LINKAGE TryMoveHero : public CPackForClient
{
	void applyGs(CGameState * gs);

	enum EResult
	{
		FAILED,
		SUCCESS,
		TELEPORTATION,
		BLOCKING_VISIT,
		EMBARK,
		DISEMBARK
	};

	ObjectInstanceID id;
	ui32 movePoints = 0;
	EResult result = FAILED; //uses EResult
	int3 start, end; //h3m format
	std::unordered_set<int3> fowRevealed; //revealed tiles
	std::optional<int3> attackedFrom; // Set when stepping into endangered tile.

	virtual void visitTyped(ICPackVisitor & visitor) override;

	bool stopMovement() const
	{
		return result != SUCCESS && result != EMBARK && result != DISEMBARK && result != TELEPORTATION;
	}

	template <typename Handler> void serialize(Handler & h, const int version)
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

struct DLL_LINKAGE NewStructures : public CPackForClient
{
	void applyGs(CGameState * gs);

	ObjectInstanceID tid;
	std::set<BuildingID> bid;
	si16 builded = 0;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & tid;
		h & bid;
		h & builded;
	}
};

struct DLL_LINKAGE RazeStructures : public CPackForClient
{
	void applyGs(CGameState * gs);

	ObjectInstanceID tid;
	std::set<BuildingID> bid;
	si16 destroyed = 0;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & tid;
		h & bid;
		h & destroyed;
	}
};

struct DLL_LINKAGE SetAvailableCreatures : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	ObjectInstanceID tid;
	std::vector<std::pair<ui32, std::vector<CreatureID> > > creatures;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & tid;
		h & creatures;
	}
};

struct DLL_LINKAGE SetHeroesInTown : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	ObjectInstanceID tid, visiting, garrison; //id of town, visiting hero, hero in garrison

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & tid;
		h & visiting;
		h & garrison;
	}
};

struct DLL_LINKAGE HeroRecruited : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	HeroTypeID hid; //subID of hero
	ObjectInstanceID tid;
	ObjectInstanceID boatId;
	int3 tile;
	PlayerColor player;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & hid;
		h & tid;
		h & boatId;
		h & tile;
		h & player;
	}
};

struct DLL_LINKAGE GiveHero : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	ObjectInstanceID id; //object id
	ObjectInstanceID boatId;
	PlayerColor player;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & id;
		h & boatId;
		h & player;
	}
};

struct DLL_LINKAGE OpenWindow : public CPackForClient
{
	EOpenWindowMode window;
	si32 id1 = -1;
	si32 id2 = -1;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & window;
		h & id1;
		h & id2;
	}
};

struct DLL_LINKAGE NewObject : public CPackForClient
{
	void applyGs(CGameState * gs);

	/// Object ID to create
	Obj ID;
	/// Object secondary ID to create
	ui32 subID = 0;
	/// Position of visitable tile of created object
	int3 targetPos;

	ObjectInstanceID createdObjectID; //used locally, filled during applyGs

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & ID;
		h & subID;
		h & targetPos;
	}
};

struct DLL_LINKAGE SetAvailableArtifacts : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	si32 id = 0; //two variants: id < 0: set artifact pool for Artifact Merchants in towns; id >= 0: set pool for adv. map Black Market (id is the id of Black Market instance then)
	std::vector<const CArtifact *> arts;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & id;
		h & arts;
	}
};

struct DLL_LINKAGE CGarrisonOperationPack : CPackForClient
{
};

struct DLL_LINKAGE ChangeStackCount : CGarrisonOperationPack
{
	ObjectInstanceID army;
	SlotID slot;
	TQuantity count;
	bool absoluteValue; //if not -> count will be added (or subtracted if negative)

	void applyGs(CGameState * gs);

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & army;
		h & slot;
		h & count;
		h & absoluteValue;
	}
};

struct DLL_LINKAGE SetStackType : CGarrisonOperationPack
{
	ObjectInstanceID army;
	SlotID slot;
	CreatureID type;

	void applyGs(CGameState * gs);

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & army;
		h & slot;
		h & type;
	}
};

struct DLL_LINKAGE EraseStack : CGarrisonOperationPack
{
	ObjectInstanceID army;
	SlotID slot;

	void applyGs(CGameState * gs);
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & army;
		h & slot;
	}
};

struct DLL_LINKAGE SwapStacks : CGarrisonOperationPack
{
	ObjectInstanceID srcArmy;
	ObjectInstanceID dstArmy;
	SlotID srcSlot;
	SlotID dstSlot;

	void applyGs(CGameState * gs);
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & srcArmy;
		h & dstArmy;
		h & srcSlot;
		h & dstSlot;
	}
};

struct DLL_LINKAGE InsertNewStack : CGarrisonOperationPack
{
	ObjectInstanceID army;
	SlotID slot;
	CreatureID type;
	TQuantity count = 0;

	void applyGs(CGameState * gs);
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & army;
		h & slot;
		h & type;
		h & count;
	}
};

///moves creatures from src stack to dst slot, may be used for merging/splittint/moving stacks
struct DLL_LINKAGE RebalanceStacks : CGarrisonOperationPack
{
	ObjectInstanceID srcArmy;
	ObjectInstanceID dstArmy;
	SlotID srcSlot;
	SlotID dstSlot;

	TQuantity count;

	void applyGs(CGameState * gs);
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & srcArmy;
		h & dstArmy;
		h & srcSlot;
		h & dstSlot;
		h & count;
	}
};

struct DLL_LINKAGE BulkRebalanceStacks : CGarrisonOperationPack
{
	std::vector<RebalanceStacks> moves;

	void applyGs(CGameState * gs);
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & moves;
	}
};

struct DLL_LINKAGE BulkSmartRebalanceStacks : CGarrisonOperationPack
{
	std::vector<RebalanceStacks> moves;
	std::vector<ChangeStackCount> changes;

	void applyGs(CGameState * gs);
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & moves;
		h & changes;
	}
};

struct GetEngagedHeroIds
{
	std::optional<ObjectInstanceID> operator()(const ConstTransitivePtr<CGHeroInstance> & h) const
	{
		return h->id;
	}
	std::optional<ObjectInstanceID> operator()(const ConstTransitivePtr<CStackInstance> & s) const
	{
		if(s->armyObj && s->armyObj->ID == Obj::HERO)
			return s->armyObj->id;
		return std::optional<ObjectInstanceID>();
	}
};

struct DLL_LINKAGE CArtifactOperationPack : CPackForClient
{
};

struct DLL_LINKAGE PutArtifact : CArtifactOperationPack
{
	PutArtifact() = default;
	PutArtifact(ArtifactLocation * dst, bool askAssemble = true)
		: al(*dst), askAssemble(askAssemble)
	{
	}

	ArtifactLocation al;
	bool askAssemble = false;
	ConstTransitivePtr<CArtifactInstance> art;

	void applyGs(CGameState * gs);
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & al;
		h & askAssemble;
		h & art;
	}
};

struct DLL_LINKAGE NewArtifact : public CArtifactOperationPack
{
	ConstTransitivePtr<CArtifactInstance> art;

	void applyGs(CGameState * gs);
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & art;
	}
};

struct DLL_LINKAGE EraseArtifact : CArtifactOperationPack
{
	ArtifactLocation al;

	void applyGs(CGameState * gs);
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & al;
	}
};

struct DLL_LINKAGE MoveArtifact : CArtifactOperationPack
{
	MoveArtifact() = default;
	MoveArtifact(ArtifactLocation * src, ArtifactLocation * dst, bool askAssemble = true)
		: src(*src), dst(*dst), askAssemble(askAssemble)
	{
	}
	ArtifactLocation src, dst;
	bool askAssemble = true;

	void applyGs(CGameState * gs);
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & src;
		h & dst;
		h & askAssemble;
	}
};

struct DLL_LINKAGE BulkMoveArtifacts : CArtifactOperationPack
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
		: swap(false)
	{
	}
	BulkMoveArtifacts(TArtHolder srcArtHolder, TArtHolder dstArtHolder, bool swap)
		: srcArtHolder(std::move(std::move(srcArtHolder)))
		, dstArtHolder(std::move(std::move(dstArtHolder)))
		, swap(swap)
	{
	}

	void applyGs(CGameState * gs);

	std::vector<LinkedSlots> artsPack0;
	std::vector<LinkedSlots> artsPack1;
	bool swap;
	CArtifactSet * getSrcHolderArtSet();
	CArtifactSet * getDstHolderArtSet();

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & artsPack0;
		h & artsPack1;
		h & srcArtHolder;
		h & dstArtHolder;
		h & swap;
	}
};

struct DLL_LINKAGE AssembledArtifact : CArtifactOperationPack
{
	ArtifactLocation al; //where assembly will be put
	CArtifact * builtArt;

	void applyGs(CGameState * gs);

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & al;
		h & builtArt;
	}
};

struct DLL_LINKAGE DisassembledArtifact : CArtifactOperationPack
{
	ArtifactLocation al;

	void applyGs(CGameState * gs);

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & al;
	}
};

struct DLL_LINKAGE HeroVisit : public CPackForClient
{
	PlayerColor player;
	ObjectInstanceID heroId;
	ObjectInstanceID objId;

	bool starting; //false -> ending

	void applyGs(CGameState * gs);

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & player;
		h & heroId;
		h & objId;
		h & starting;
	}
};

struct DLL_LINKAGE NewTurn : public CPackForClient
{
	enum weekType { NORMAL, DOUBLE_GROWTH, BONUS_GROWTH, DEITYOFFIRE, PLAGUE, NO_ACTION };

	void applyGs(CGameState * gs);

	virtual void visitTyped(ICPackVisitor & visitor) override;

	struct Hero
	{
		ObjectInstanceID id;
		ui32 move, mana; //id is a general serial id
		template <typename Handler> void serialize(Handler & h, const int version)
		{
			h & id;
			h & move;
			h & mana;
		}
		bool operator<(const Hero & h)const { return id < h.id; }
	};

	std::set<Hero> heroes; //updates movement and mana points
	std::map<PlayerColor, TResources> res; //player ID => resource value[res_id]
	std::map<ObjectInstanceID, SetAvailableCreatures> cres;//creatures to be placed in towns
	ui32 day = 0;
	ui8 specialWeek = 0; //weekType
	CreatureID creatureid; //for creature weeks

	NewTurn() = default;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & heroes;
		h & cres;
		h & res;
		h & day;
		h & specialWeek;
		h & creatureid;
	}
};

struct DLL_LINKAGE InfoWindow : public CPackForClient //103  - displays simple info window
{
	EInfoWindowMode type = EInfoWindowMode::MODAL;
	MetaString text;
	std::vector<Component> components;
	PlayerColor player;
	ui16 soundID = 0;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & type;
		h & text;
		h & components;
		h & player;
		h & soundID;
	}
	InfoWindow() = default;
};

namespace ObjProperty
{
	enum
	{
		OWNER = 1, BLOCKVIS = 2, PRIMARY_STACK_COUNT = 3, VISITORS = 4, VISITED = 5, ID = 6, AVAILABLE_CREATURE = 7, SUBID = 8,
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

struct DLL_LINKAGE SetObjectProperty : public CPackForClient
{
	void applyGs(CGameState * gs) const;
	ObjectInstanceID id;
	ui8 what = 0; // see ObjProperty enum
	ui32 val = 0;
	SetObjectProperty() = default;
	SetObjectProperty(const ObjectInstanceID & ID, ui8 What, ui32 Val)
		: id(ID)
		, what(What)
		, val(Val)
	{
	}

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & id;
		h & what;
		h & val;
	}
};

struct DLL_LINKAGE ChangeObjectVisitors : public CPackForClient
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

	void applyGs(CGameState * gs) const;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	ChangeObjectVisitors() = default;

	ChangeObjectVisitors(ui32 mode, const ObjectInstanceID & object, const ObjectInstanceID & heroID = ObjectInstanceID(-1))
		: mode(mode)
		, object(object)
		, hero(heroID)
	{
	}

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & object;
		h & hero;
		h & mode;
	}
};

struct DLL_LINKAGE PrepareHeroLevelUp : public CPackForClient
{
	ObjectInstanceID heroId;

	/// Do not serialize, used by server only
	std::vector<SecondarySkill> skills;

	void applyGs(CGameState * gs);

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & heroId;
	}
};

struct DLL_LINKAGE HeroLevelUp : public Query
{
	PlayerColor player;
	ObjectInstanceID heroId;

	PrimarySkill::PrimarySkill primskill = PrimarySkill::ATTACK;
	std::vector<SecondarySkill> skills;

	void applyGs(CGameState * gs) const;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & queryID;
		h & player;
		h & heroId;
		h & primskill;
		h & skills;
	}
};

struct DLL_LINKAGE CommanderLevelUp : public Query
{
	PlayerColor player;
	ObjectInstanceID heroId;

	std::vector<ui32> skills; //0-5 - secondary skills, val-100 - special skill

	void applyGs(CGameState * gs) const;

	virtual void visitTyped(ICPackVisitor & visitor) override;

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
struct DLL_LINKAGE BlockingDialog : public Query
{
	enum { ALLOW_CANCEL = 1, SELECTION = 2 };
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

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & queryID;
		h & text;
		h & components;
		h & player;
		h & flags;
		h & soundID;
	}
};

struct DLL_LINKAGE GarrisonDialog : public Query
{
	ObjectInstanceID objid, hid;
	bool removableUnits = false;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & queryID;
		h & objid;
		h & hid;
		h & removableUnits;
	}
};

struct DLL_LINKAGE ExchangeDialog : public Query
{
	PlayerColor player;

	ObjectInstanceID hero1;
	ObjectInstanceID hero2;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & queryID;
		h & player;
		h & hero1;
		h & hero2;
	}
};

struct DLL_LINKAGE TeleportDialog : public Query
{
	TeleportDialog() = default;

	TeleportDialog(const PlayerColor & Player, const TeleportChannelID & Channel)
		: player(Player)
		, channel(Channel)
	{
	}
	PlayerColor player;
	TeleportChannelID channel;
	TTeleportExitsList exits;
	bool impassable = false;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & queryID;
		h & player;
		h & channel;
		h & exits;
		h & impassable;
	}
};

struct DLL_LINKAGE MapObjectSelectDialog : public Query
{
	PlayerColor player;
	Component icon;
	MetaString title;
	MetaString description;
	std::vector<ObjectInstanceID> objects;

	virtual void visitTyped(ICPackVisitor & visitor) override;

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
struct DLL_LINKAGE BattleStart : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	BattleInfo * info = nullptr;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & info;
	}
};

struct DLL_LINKAGE BattleNextRound : public CPackForClient
{
	void applyGs(CGameState * gs) const;
	si32 round = 0;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & round;
	}
};

struct DLL_LINKAGE BattleSetActiveStack : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	ui32 stack = 0;
	ui8 askPlayerInterface = true;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & stack;
		h & askPlayerInterface;
	}
};

struct DLL_LINKAGE BattleResultAccepted : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	struct HeroBattleResults
	{
		HeroBattleResults()
			: hero(nullptr), army(nullptr), exp(0) {}

		CGHeroInstance * hero;
		CArmedInstance * army;
		TExpType exp;

		template <typename Handler> void serialize(Handler & h, const int version)
		{
			h & hero;
			h & army;
			h & exp;
		}
	};
	std::array<HeroBattleResults, 2> heroResult;
	ui8 winnerSide;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & heroResult;
		h & winnerSide;
	}
};

struct DLL_LINKAGE BattleResult : public Query
{
	enum EResult { NORMAL = 0, ESCAPE = 1, SURRENDER = 2 };

	void applyFirstCl(CClient * cl);

	EResult result = NORMAL;
	ui8 winner = 2; //0 - attacker, 1 - defender, [2 - draw (should be possible?)]
	std::map<ui32, si32> casualties[2]; //first => casualties of attackers - map crid => number
	TExpType exp[2] = {0, 0}; //exp for attacker and defender
	std::set<ArtifactInstanceID> artifacts; //artifacts taken from loser to winner - currently unused

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & queryID;
		h & result;
		h & winner;
		h & casualties[0];
		h & casualties[1];
		h & exp;
		h & artifacts;
	}
};

struct DLL_LINKAGE BattleLogMessage : public CPackForClient
{
	std::vector<MetaString> lines;

	void applyGs(CGameState * gs);
	void applyBattle(IBattleState * battleState);

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & lines;
	}
};

struct DLL_LINKAGE BattleStackMoved : public CPackForClient
{
	ui32 stack = 0;
	std::vector<BattleHex> tilesToMove;
	int distance = 0;
	bool teleporting = false;

	void applyGs(CGameState * gs);
	void applyBattle(IBattleState * battleState);

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & stack;
		h & tilesToMove;
		h & distance;
		h & teleporting;
	}
};

struct DLL_LINKAGE BattleUnitsChanged : public CPackForClient
{
	void applyGs(CGameState * gs);
	void applyBattle(IBattleState * battleState);

	std::vector<UnitChanges> changedStacks;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & changedStacks;
	}
};

struct BattleStackAttacked
{
	DLL_LINKAGE void applyGs(CGameState * gs);
	DLL_LINKAGE void applyBattle(IBattleState * battleState);

	ui32 stackAttacked = 0, attackerID = 0;
	ui32 killedAmount = 0;
	int64_t damageAmount = 0;
	UnitChanges newState;
	enum EFlags { KILLED = 1, SECONDARY = 2, REBIRTH = 4, CLONE_KILLED = 8, SPELL_EFFECT = 16, FIRE_SHIELD = 32, };
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

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & stackAttacked;
		h & attackerID;
		h & newState;
		h & flags;
		h & killedAmount;
		h & damageAmount;
		h & spellID;
	}
	bool operator<(const BattleStackAttacked & b) const
	{
		return stackAttacked < b.stackAttacked;
	}
};

struct DLL_LINKAGE BattleAttack : public CPackForClient
{
	void applyGs(CGameState * gs);
	BattleUnitsChanged attackerChanges;

	std::vector<BattleStackAttacked> bsa;
	ui32 stackAttacking = 0;
	ui32 flags = 0; //uses Eflags (below)
	enum EFlags { SHOT = 1, COUNTER = 2, LUCKY = 4, UNLUCKY = 8, BALLISTA_DOUBLE_DMG = 16, DEATH_BLOW = 32, SPELL_LIKE = 64, LIFE_DRAIN = 128 };

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

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & bsa;
		h & stackAttacking;
		h & flags;
		h & tile;
		h & spellID;
		h & attackerChanges;
	}
};

struct DLL_LINKAGE StartAction : public CPackForClient
{
	StartAction() = default;
	StartAction(BattleAction act)
		: ba(std::move(act))
	{
	}
	void applyFirstCl(CClient * cl);
	void applyGs(CGameState * gs);

	BattleAction ba;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & ba;
	}
};

struct DLL_LINKAGE EndAction : public CPackForClient
{
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
	}
};

struct DLL_LINKAGE BattleSpellCast : public CPackForClient
{
	void applyGs(CGameState * gs) const;
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

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
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

struct DLL_LINKAGE SetStackEffect : public CPackForClient
{
	void applyGs(CGameState * gs);
	void applyBattle(IBattleState * battleState);
	std::vector<std::pair<ui32, std::vector<Bonus>>> toAdd;
	std::vector<std::pair<ui32, std::vector<Bonus>>> toUpdate;
	std::vector<std::pair<ui32, std::vector<Bonus>>> toRemove;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & toAdd;
		h & toUpdate;
		h & toRemove;
	}
};

struct DLL_LINKAGE StacksInjured : public CPackForClient
{
	void applyGs(CGameState * gs);
	void applyBattle(IBattleState * battleState);

	std::vector<BattleStackAttacked> stacks;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & stacks;
	}
};

struct DLL_LINKAGE BattleResultsApplied : public CPackForClient
{
	PlayerColor player1, player2;
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & player1;
		h & player2;
	}
};

struct DLL_LINKAGE BattleObstaclesChanged : public CPackForClient
{
	void applyGs(CGameState * gs);
	void applyBattle(IBattleState * battleState);

	std::vector<ObstacleChanges> changes;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & changes;
	}
};

struct DLL_LINKAGE CatapultAttack : public CPackForClient
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

	CatapultAttack();
	~CatapultAttack() override;

	void applyGs(CGameState * gs);
	void applyBattle(IBattleState * battleState);

	std::vector< AttackInfo > attackedParts;
	int attacker = -1; //if -1, then a spell caused this

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & attackedParts;
		h & attacker;
	}
};

struct DLL_LINKAGE BattleSetStackProperty : public CPackForClient
{
	enum BattleStackProperty { CASTS, ENCHANTER_COUNTER, UNBIND, CLONED, HAS_CLONE };

	void applyGs(CGameState * gs) const;

	int stackID = 0;
	BattleStackProperty which = CASTS;
	int val = 0;
	int absolute = 0;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & stackID;
		h & which;
		h & val;
		h & absolute;
	}

protected:
	virtual void visitTyped(ICPackVisitor & visitor) override;
};

///activated at the beginning of turn
struct DLL_LINKAGE BattleTriggerEffect : public CPackForClient
{
	void applyGs(CGameState * gs) const; //effect

	int stackID = 0;
	int effect = 0; //use corresponding Bonus type
	int val = 0;
	int additionalInfo = 0;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & stackID;
		h & effect;
		h & val;
		h & additionalInfo;
	}

protected:
	virtual void visitTyped(ICPackVisitor & visitor) override;
};

struct DLL_LINKAGE BattleUpdateGateState : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	EGateState state = EGateState::NONE;
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & state;
	}

protected:
	virtual void visitTyped(ICPackVisitor & visitor) override;
};

struct DLL_LINKAGE AdvmapSpellCast : public CPackForClient
{
	ObjectInstanceID casterID;
	SpellID spellID;
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & casterID;
		h & spellID;
	}

protected:
	virtual void visitTyped(ICPackVisitor & visitor) override;
};

struct DLL_LINKAGE ShowWorldViewEx : public CPackForClient
{
	PlayerColor player;
	bool showTerrain; // TODO: send terrain state

	std::vector<ObjectPosInfo> objectPositions;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & player;
		h & showTerrain;
		h & objectPositions;
	}

protected:
	virtual void visitTyped(ICPackVisitor & visitor) override;
};

/***********************************************************************************************************/

struct DLL_LINKAGE EndTurn : public CPackForServer
{
	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
	}
};

struct DLL_LINKAGE DismissHero : public CPackForServer
{
	DismissHero() = default;
	DismissHero(const ObjectInstanceID & HID)
		: hid(HID)
	{
	}
	ObjectInstanceID hid;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & hid;
	}
};

struct DLL_LINKAGE MoveHero : public CPackForServer
{
	MoveHero() = default;
	MoveHero(const int3 & Dest, const ObjectInstanceID & HID, bool Transit)
		: dest(Dest)
		, hid(HID)
		, transit(Transit)
	{
	}
	int3 dest;
	ObjectInstanceID hid;
	bool transit = false;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & dest;
		h & hid;
		h & transit;
	}
};

struct DLL_LINKAGE CastleTeleportHero : public CPackForServer
{
	CastleTeleportHero() = default;
	CastleTeleportHero(const ObjectInstanceID & HID, const ObjectInstanceID & Dest, ui8 Source)
		: dest(Dest)
		, hid(HID)
		, source(Source)
	{
	}
	ObjectInstanceID dest;
	ObjectInstanceID hid;
	si8 source = 0; //who give teleporting, 1=castle gate

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & dest;
		h & hid;
	}
};

struct DLL_LINKAGE ArrangeStacks : public CPackForServer
{
	ArrangeStacks() = default;
	ArrangeStacks(ui8 W, const SlotID & P1, const SlotID & P2, const ObjectInstanceID & ID1, const ObjectInstanceID & ID2, si32 VAL)
		: what(W)
		, p1(P1)
		, p2(P2)
		, id1(ID1)
		, id2(ID2)
		, val(VAL)
	{
	}

	ui8 what = 0; //1 - swap; 2 - merge; 3 - split
	SlotID p1, p2; //positions of first and second stack
	ObjectInstanceID id1, id2; //ids of objects with garrison
	si32 val = 0;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
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

struct DLL_LINKAGE BulkMoveArmy : public CPackForServer
{
	SlotID srcSlot;
	ObjectInstanceID srcArmy;
	ObjectInstanceID destArmy;

	BulkMoveArmy() = default;

	BulkMoveArmy(const ObjectInstanceID & srcArmy, const ObjectInstanceID & destArmy, const SlotID & srcSlot)
		: srcArmy(srcArmy)
		, destArmy(destArmy)
		, srcSlot(srcSlot)
	{
	}

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & srcSlot;
		h & srcArmy;
		h & destArmy;
	}
};

struct DLL_LINKAGE BulkSplitStack : public CPackForServer
{
	SlotID src;
	ObjectInstanceID srcOwner;
	si32 amount = 0;

	BulkSplitStack() = default;

	BulkSplitStack(const ObjectInstanceID & srcOwner, const SlotID & src, si32 howMany)
		: src(src)
		, srcOwner(srcOwner)
		, amount(howMany)
	{
	}

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & src;
		h & srcOwner;
		h & amount;
	}
};

struct DLL_LINKAGE BulkMergeStacks : public CPackForServer
{
	SlotID src;
	ObjectInstanceID srcOwner;

	BulkMergeStacks() = default;

	BulkMergeStacks(const ObjectInstanceID & srcOwner, const SlotID & src)
		: src(src)
		, srcOwner(srcOwner)
	{
	}

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & src;
		h & srcOwner;
	}
};

struct DLL_LINKAGE BulkSmartSplitStack : public CPackForServer
{
	SlotID src;
	ObjectInstanceID srcOwner;

	BulkSmartSplitStack() = default;

	BulkSmartSplitStack(const ObjectInstanceID & srcOwner, const SlotID & src)
		: src(src)
		, srcOwner(srcOwner)
	{
	}

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & src;
		h & srcOwner;
	}
};

struct DLL_LINKAGE DisbandCreature : public CPackForServer
{
	DisbandCreature() = default;
	DisbandCreature(const SlotID & Pos, const ObjectInstanceID & ID)
		: pos(Pos)
		, id(ID)
	{
	}
	SlotID pos; //stack pos
	ObjectInstanceID id; //object id

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & pos;
		h & id;
	}
};

struct DLL_LINKAGE BuildStructure : public CPackForServer
{
	BuildStructure() = default;
	BuildStructure(const ObjectInstanceID & TID, const BuildingID & BID)
		: tid(TID)
		, bid(BID)
	{
	}
	ObjectInstanceID tid; //town id
	BuildingID bid; //structure id

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & tid;
		h & bid;
	}
};

struct DLL_LINKAGE RazeStructure : public BuildStructure
{
	virtual void visitTyped(ICPackVisitor & visitor) override;
};

struct DLL_LINKAGE RecruitCreatures : public CPackForServer
{
	RecruitCreatures() = default;
	RecruitCreatures(const ObjectInstanceID & TID, const ObjectInstanceID & DST, const CreatureID & CRID, si32 Amount, si32 Level)
		: tid(TID)
		, dst(DST)
		, crid(CRID)
		, amount(Amount)
		, level(Level)
	{
	}
	ObjectInstanceID tid; //dwelling id, or town
	ObjectInstanceID dst; //destination ID, e.g. hero
	CreatureID crid;
	ui32 amount = 0; //creature amount
	si32 level = 0; //dwelling level to buy from, -1 if any

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & tid;
		h & dst;
		h & crid;
		h & amount;
		h & level;
	}
};

struct DLL_LINKAGE UpgradeCreature : public CPackForServer
{
	UpgradeCreature() = default;
	UpgradeCreature(const SlotID & Pos, const ObjectInstanceID & ID, const CreatureID & CRID)
		: pos(Pos)
		, id(ID)
		, cid(CRID)
	{
	}
	SlotID pos; //stack pos
	ObjectInstanceID id; //object id
	CreatureID cid; //id of type to which we want make upgrade

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & pos;
		h & id;
		h & cid;
	}
};

struct DLL_LINKAGE GarrisonHeroSwap : public CPackForServer
{
	GarrisonHeroSwap() = default;
	GarrisonHeroSwap(const ObjectInstanceID & TID)
		: tid(TID)
	{
	}
	ObjectInstanceID tid;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & tid;
	}
};

struct DLL_LINKAGE ExchangeArtifacts : public CPackForServer
{
	ArtifactLocation src, dst;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & src;
		h & dst;
	}
};

struct DLL_LINKAGE BulkExchangeArtifacts : public CPackForServer
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

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & srcHero;
		h & dstHero;
		h & swap;
	}
};

struct DLL_LINKAGE AssembleArtifacts : public CPackForServer
{
	AssembleArtifacts() = default;
	AssembleArtifacts(const ObjectInstanceID & _heroID, const ArtifactPosition & _artifactSlot, bool _assemble, const ArtifactID & _assembleTo)
		: heroID(_heroID)
		, artifactSlot(_artifactSlot)
		, assemble(_assemble)
		, assembleTo(_assembleTo)
	{
	}
	ObjectInstanceID heroID;
	ArtifactPosition artifactSlot;
	bool assemble = false; // True to assemble artifact, false to disassemble.
	ArtifactID assembleTo; // Artifact to assemble into.

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & heroID;
		h & artifactSlot;
		h & assemble;
		h & assembleTo;
	}
};

struct DLL_LINKAGE EraseArtifactByClient : public CPackForServer
{
	EraseArtifactByClient() = default;
	EraseArtifactByClient(const ArtifactLocation & al)
		: al(al)
	{
	}
	ArtifactLocation al;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer&>(*this);
		h & al;
	}
};

struct DLL_LINKAGE BuyArtifact : public CPackForServer
{
	BuyArtifact() = default;
	BuyArtifact(const ObjectInstanceID & HID, const ArtifactID & AID)
		: hid(HID)
		, aid(AID)
	{
	}
	ObjectInstanceID hid;
	ArtifactID aid;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & hid;
		h & aid;
	}
};

struct DLL_LINKAGE TradeOnMarketplace : public CPackForServer
{
	ObjectInstanceID marketId;
	ObjectInstanceID heroId;

	EMarketMode::EMarketMode mode = EMarketMode::RESOURCE_RESOURCE;
	std::vector<ui32> r1, r2; //mode 0: r1 - sold resource, r2 - bought res (exception: when sacrificing art r1 is art id [todo: make r2 preferred slot?]
	std::vector<ui32> val; //units of sold resource

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
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

struct DLL_LINKAGE SetFormation : public CPackForServer
{
	SetFormation() = default;
	;
	SetFormation(const ObjectInstanceID & HID, ui8 Formation)
		: hid(HID)
		, formation(Formation)
	{
	}
	ObjectInstanceID hid;
	ui8 formation = 0;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & hid;
		h & formation;
	}
};

struct DLL_LINKAGE HireHero : public CPackForServer
{
	HireHero() = default;
	HireHero(HeroTypeID HID, const ObjectInstanceID & TID)
		: hid(HID)
		, tid(TID)
	{
	}
	HeroTypeID hid; //available hero serial
	ObjectInstanceID tid; //town (tavern) id
	PlayerColor player;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & hid;
		h & tid;
		h & player;
	}
};

struct DLL_LINKAGE BuildBoat : public CPackForServer
{
	ObjectInstanceID objid; //where player wants to buy a boat

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & objid;
	}
};

struct DLL_LINKAGE QueryReply : public CPackForServer
{
	QueryReply() = default;
	QueryReply(const QueryID & QID, const JsonNode & Reply)
		: qid(QID)
		, reply(Reply)
	{
	}
	QueryID qid;
	PlayerColor player;
	JsonNode reply;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & qid;
		h & player;
		h & reply;
	}
};

struct DLL_LINKAGE MakeAction : public CPackForServer
{
	MakeAction() = default;
	MakeAction(BattleAction BA)
		: ba(std::move(BA))
	{
	}
	BattleAction ba;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & ba;
	}
};

struct DLL_LINKAGE MakeCustomAction : public CPackForServer
{
	MakeCustomAction() = default;
	MakeCustomAction(BattleAction BA)
		: ba(std::move(BA))
	{
	}
	BattleAction ba;

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & ba;
	}
};

struct DLL_LINKAGE DigWithHero : public CPackForServer
{
	ObjectInstanceID id; //digging hero id

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & id;
	}
};

struct DLL_LINKAGE CastAdvSpell : public CPackForServer
{
	ObjectInstanceID hid; //hero id
	SpellID sid; //spell id
	int3 pos; //selected tile (not always used)

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & hid;
		h & sid;
		h & pos;
	}
};

/***********************************************************************************************************/

struct DLL_LINKAGE SaveGame : public CPackForServer
{
	SaveGame() = default;
	SaveGame(std::string Fname)
		: fname(std::move(Fname))
	{
	}
	std::string fname;

	void applyGs(CGameState * gs) {};

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & fname;
	}
};

struct DLL_LINKAGE PlayerMessage : public CPackForServer
{
	PlayerMessage() = default;
	PlayerMessage(std::string Text, const ObjectInstanceID & obj)
		: text(std::move(Text))
		, currObj(obj)
	{
	}

	void applyGs(CGameState * gs) {};

	virtual void visitTyped(ICPackVisitor & visitor) override;

	std::string text;
	ObjectInstanceID currObj; // optional parameter that specifies current object. For cheats :)

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & static_cast<CPackForServer &>(*this);
		h & text;
		h & currObj;
	}
};

struct DLL_LINKAGE PlayerMessageClient : public CPackForClient
{
	PlayerMessageClient() = default;
	PlayerMessageClient(const PlayerColor & Player, std::string Text)
		: player(Player)
		, text(std::move(Text))
	{
	}
	virtual void visitTyped(ICPackVisitor & visitor) override;

	PlayerColor player;
	std::string text;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & player;
		h & text;
	}
};

struct DLL_LINKAGE CenterView : public CPackForClient
{
	PlayerColor player;
	int3 pos;
	ui32 focusTime = 0; //ms

	virtual void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & pos;
		h & player;
		h & focusTime;
	}
};

VCMI_LIB_NAMESPACE_END
