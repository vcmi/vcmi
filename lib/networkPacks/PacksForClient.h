/*
 * PacksForClient.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ArtifactLocation.h"
#include "Component.h"
#include "EInfoWindowMode.h"
#include "EOpenWindowMode.h"
#include "EntityChanges.h"
#include "NetPacksBase.h"
#include "ObjProperty.h"

#include "../CCreatureSet.h"
#include "../MetaString.h"
#include "../ResourceSet.h"
#include "../TurnTimerInfo.h"
#include "../gameState/EVictoryLossCheckResult.h"
#include "../gameState/QuestInfo.h"
#include "../gameState/TavernSlot.h"
#include "../int3.h"
#include "../mapping/CMapDefines.h"
#include "../spells/ViewSpellInt.h"

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
class BattleInfo;

// This one teleport-specific, but has to be available everywhere in callbacks and netpacks
// For now it's will be there till teleports code refactored and moved into own file
using TTeleportExitsList = std::vector<std::pair<ObjectInstanceID, int3>>;

/***********************************************************************************************************/
struct DLL_LINKAGE PackageApplied : public CPackForClient
{
	PackageApplied() = default;
	explicit PackageApplied(ui8 Result)
		: result(Result)
	{
	}
	void visitTyped(ICPackVisitor & visitor) override;

	ui8 result = 0; //0 - something went wrong, request hasn't been realized; 1 - OK
	ui32 packType = 0; //type id of applied package
	ui32 requestID = 0; //an ID given by client to the request that was applied
	PlayerColor player;

	template <typename Handler> void serialize(Handler & h)
	{
		h & result;
		h & packType;
		h & requestID;
		h & player;
	}
};

struct DLL_LINKAGE SystemMessage : public CPackForClient
{
	explicit SystemMessage(std::string Text)
		: text(std::move(Text))
	{
	}
	SystemMessage() = default;

	void visitTyped(ICPackVisitor & visitor) override;

	std::string text;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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
		
	template <typename Handler> void serialize(Handler & h)
	{
		h & player;
		h & turnTimer;
	}
};

struct DLL_LINKAGE PlayerStartsTurn : public Query
{
	void applyGs(CGameState * gs) const;

	PlayerColor player;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & queryID;
		h & player;
	}
};

struct DLL_LINKAGE DaysWithoutTown : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	PlayerColor player;
	std::optional<int32_t> daysWithoutCastle;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & player;
		h & daysWithoutCastle;
	}
};

struct DLL_LINKAGE EntitiesChanged : public CPackForClient
{
	std::vector<EntityChanges> changes;

	void applyGs(CGameState * gs);

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & changes;
	}
};

struct DLL_LINKAGE SetResources : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	void visitTyped(ICPackVisitor & visitor) override;

	bool abs = true; //false - changes by value; 1 - sets to value
	PlayerColor player;
	ResourceSet res; //res[resid] => res amount

	template <typename Handler> void serialize(Handler & h)
	{
		h & abs;
		h & player;
		h & res;
	}
};

struct DLL_LINKAGE SetPrimSkill : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	void visitTyped(ICPackVisitor & visitor) override;

	ui8 abs = 0; //0 - changes by value; 1 - sets to value
	ObjectInstanceID id;
	PrimarySkill which = PrimarySkill::ATTACK;
	si64 val = 0;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	ui8 abs = 0; //0 - changes by value; 1 - sets to value
	ObjectInstanceID id;
	SecondarySkill which;
	ui16 val = 0;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	ui8 flags = 0; //1 - start
	ObjectInstanceID tid;
	ObjectInstanceID hid;

	bool start() const //if hero is entering castle (if false - leaving)
	{
		return flags & 1;
	}

	template <typename Handler> void serialize(Handler & h)
	{
		h & flags;
		h & tid;
		h & hid;
	}
};

struct DLL_LINKAGE ChangeSpells : public CPackForClient
{
	void applyGs(CGameState * gs);

	void visitTyped(ICPackVisitor & visitor) override;

	ui8 learn = 1; //1 - gives spell, 0 - takes
	ObjectInstanceID hid;
	std::set<SpellID> spells;

	template <typename Handler> void serialize(Handler & h)
	{
		h & learn;
		h & hid;
		h & spells;
	}
};

struct DLL_LINKAGE SetMana : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	void visitTyped(ICPackVisitor & visitor) override;

	ObjectInstanceID hid;
	si32 val = 0;
	bool absolute = true;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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
	ETileVisibility mode;
	bool waitForDialogs = false;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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
	bool replenishPoints;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & slotID;
		h & roleID;
		h & player;
		h & hid;
		h & army;
		h & replenishPoints;
	}
};

struct DLL_LINKAGE GiveBonus : public CPackForClient
{
	enum class ETarget : int8_t { OBJECT, PLAYER, BATTLE };
	
	explicit GiveBonus(ETarget Who = ETarget::OBJECT)
		:who(Who)
	{
	}

	void applyGs(CGameState * gs);

	ETarget who = ETarget::OBJECT;
	VariantIdentifier<ObjectInstanceID, PlayerColor, BattleID> id;
	Bonus bonus;
	MetaString bdescr;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & bonus;
		h & id;
		h & bdescr;
		h & who;
		assert(id.getNum() != -1);
	}
};

struct DLL_LINKAGE ChangeObjPos : public CPackForClient
{
	void applyGs(CGameState * gs);

	/// Object to move
	ObjectInstanceID objid;
	/// New position of visitable tile of an object
	int3 nPos;
	/// Player that initiated this action, if any
	PlayerColor initiator;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & objid;
		h & nPos;
		h & initiator;
	}
};

struct DLL_LINKAGE PlayerEndsTurn : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	PlayerColor player;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & player;
	}
};

struct DLL_LINKAGE PlayerEndsGame : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	PlayerColor player;
	EVictoryLossCheckResult victoryLossCheckResult;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & players;
		h & playerConnectionId;
	}
};

struct DLL_LINKAGE RemoveBonus : public CPackForClient
{
	explicit RemoveBonus(GiveBonus::ETarget Who = GiveBonus::ETarget::OBJECT)
		:who(Who)
	{
	}

	void applyGs(CGameState * gs);

	GiveBonus::ETarget who; //who receives bonus
	VariantIdentifier<HeroTypeID, PlayerColor, BattleID, ObjectInstanceID> whoID;

	//vars to identify bonus: its source
	BonusSource source;
	BonusSourceID id; //source id

	//used locally: copy of removed bonus
	Bonus bonus;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & player;
		h & quest;
	}
};

struct DLL_LINKAGE UpdateArtHandlerLists : public CPackForClient
{
	std::map<ArtifactID, int> allocatedArtifacts;

	void applyGs(CGameState * gs) const;
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & allocatedArtifacts;
	}
};

struct DLL_LINKAGE UpdateMapEvents : public CPackForClient
{
	std::list<CMapEvent> events;

	void applyGs(CGameState * gs) const;
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & events;
	}
};

struct DLL_LINKAGE UpdateCastleEvents : public CPackForClient
{
	ObjectInstanceID town;
	std::list<CCastleEvent> events;

	void applyGs(CGameState * gs) const;
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & town;
		h & events;
	}
};

struct DLL_LINKAGE ChangeFormation : public CPackForClient
{
	ObjectInstanceID hid;
	EArmyFormation formation{};

	void applyGs(CGameState * gs) const;
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & hid;
		h & formation;
	}
};

struct DLL_LINKAGE RemoveObject : public CPackForClient
{
	RemoveObject() = default;
	RemoveObject(const ObjectInstanceID & objectID, const PlayerColor & initiator)
		: objectID(objectID)
		, initiator(initiator)
	{
	}

	void applyGs(CGameState * gs);
	void visitTyped(ICPackVisitor & visitor) override;

	/// ID of removed object
	ObjectInstanceID objectID;

	/// Player that initiated this action, if any
	PlayerColor initiator;

	template <typename Handler> void serialize(Handler & h)
	{
		h & objectID;
		h & initiator;
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
	int3 start; //h3m format
	int3 end;
	std::unordered_set<int3> fowRevealed; //revealed tiles
	std::optional<int3> attackedFrom; // Set when stepping into endangered tile.

	void visitTyped(ICPackVisitor & visitor) override;

	bool stopMovement() const
	{
		return result != SUCCESS && result != EMBARK && result != DISEMBARK && result != TELEPORTATION;
	}

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & tid;
		h & creatures;
	}
};

struct DLL_LINKAGE SetHeroesInTown : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	ObjectInstanceID tid; //id of town
	ObjectInstanceID visiting; //id of visiting hero
	ObjectInstanceID garrison; //id of hero in garrison

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & id;
		h & boatId;
		h & player;
	}
};

struct DLL_LINKAGE OpenWindow : public Query
{
	EOpenWindowMode window;
	ObjectInstanceID object;
	ObjectInstanceID visitor;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & queryID;
		h & window;
		h & object;
		h & visitor;
	}
};

struct DLL_LINKAGE NewObject : public CPackForClient
{
	void applyGs(CGameState * gs);

	/// Object ID to create
	MapObjectID ID;
	/// Object secondary ID to create
	MapObjectSubID subID;
	/// Position of visitable tile of created object
	int3 targetPos;
	/// Which player initiated creation of this object
	PlayerColor initiator;

	ObjectInstanceID createdObjectID; //used locally, filled during applyGs

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & ID;
		subID.serializeIdentifier(h, ID);
		h & targetPos;
		h & initiator;
	}
};

struct DLL_LINKAGE SetAvailableArtifacts : public CPackForClient
{
	void applyGs(CGameState * gs) const;

	//two variants: id < 0: set artifact pool for Artifact Merchants in towns; id >= 0: set pool for adv. map Black Market (id is the id of Black Market instance then)
	ObjectInstanceID id;
	std::vector<const CArtifact *> arts;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler>
	void serialize(Handler & h)
	{
		h & moves;
	}
};

struct DLL_LINKAGE BulkSmartRebalanceStacks : CGarrisonOperationPack
{
	std::vector<RebalanceStacks> moves;
	std::vector<ChangeStackCount> changes;

	void applyGs(CGameState * gs);
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler>
	void serialize(Handler & h)
	{
		h & moves;
		h & changes;
	}
};

struct DLL_LINKAGE CArtifactOperationPack : CPackForClient
{
};

struct DLL_LINKAGE PutArtifact : CArtifactOperationPack
{
	PutArtifact() = default;
	explicit PutArtifact(ArtifactLocation & dst, bool askAssemble = true)
		: al(dst), askAssemble(askAssemble)
	{
	}

	ArtifactLocation al;
	bool askAssemble;
	ConstTransitivePtr<CArtifactInstance> art;

	void applyGs(CGameState * gs);
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & art;
	}
};

struct DLL_LINKAGE EraseArtifact : CArtifactOperationPack
{
	ArtifactLocation al;

	void applyGs(CGameState * gs);
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & al;
	}
};

struct DLL_LINKAGE MoveArtifact : CArtifactOperationPack
{
	MoveArtifact() = default;
	MoveArtifact(const PlayerColor & interfaceOwner, const ArtifactLocation & src, const ArtifactLocation & dst, bool askAssemble = true)
		: interfaceOwner(interfaceOwner), src(src), dst(dst), askAssemble(askAssemble)
	{
	}
	PlayerColor interfaceOwner;
	ArtifactLocation src;
	ArtifactLocation dst;
	bool askAssemble = true;

	void applyGs(CGameState * gs);
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & interfaceOwner;
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
		template <typename Handler> void serialize(Handler & h)
		{
			h & srcPos;
			h & dstPos;
		}
	};

	PlayerColor interfaceOwner;
	ObjectInstanceID srcArtHolder;
	ObjectInstanceID dstArtHolder;
	std::optional<SlotID> srcCreature;
	std::optional<SlotID> dstCreature;

	BulkMoveArtifacts()
		: interfaceOwner(PlayerColor::NEUTRAL)
		, srcArtHolder(ObjectInstanceID::NONE)
		, dstArtHolder(ObjectInstanceID::NONE)
		, swap(false)
		, askAssemble(false)
		, srcCreature(std::nullopt)
		, dstCreature(std::nullopt)
	{
	}
	BulkMoveArtifacts(const PlayerColor & interfaceOwner, const ObjectInstanceID srcArtHolder, const ObjectInstanceID dstArtHolder, bool swap)
		: interfaceOwner(interfaceOwner)
		, srcArtHolder(srcArtHolder)
		, dstArtHolder(dstArtHolder)
		, swap(swap)
		, askAssemble(false)
		, srcCreature(std::nullopt)
		, dstCreature(std::nullopt)
	{
	}

	void applyGs(CGameState * gs);

	std::vector<LinkedSlots> artsPack0;
	std::vector<LinkedSlots> artsPack1;
	bool swap;
	bool askAssemble;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & interfaceOwner;
		h & artsPack0;
		h & artsPack1;
		h & srcArtHolder;
		h & dstArtHolder;
		h & srcCreature;
		h & dstCreature;
		h & swap;
		h & askAssemble;
	}
};

struct DLL_LINKAGE AssembledArtifact : CArtifactOperationPack
{
	ArtifactLocation al; //where assembly will be put
	const CArtifact * builtArt;

	void applyGs(CGameState * gs);

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & al;
		h & builtArt;
	}
};

struct DLL_LINKAGE DisassembledArtifact : CArtifactOperationPack
{
	ArtifactLocation al;

	void applyGs(CGameState * gs);

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	struct Hero
	{
		ObjectInstanceID id; //id is a general serial id
		ui32 move;
		ui32 mana;
		template <typename Handler> void serialize(Handler & h)
		{
			h & id;
			h & move;
			h & mana;
		}
		bool operator<(const Hero & h)const { return id < h.id; }
	};

	std::set<Hero> heroes; //updates movement and mana points
	std::map<PlayerColor, ResourceSet> res; //player ID => resource value[res_id]
	std::map<ObjectInstanceID, SetAvailableCreatures> cres;//creatures to be placed in towns
	ui32 day = 0;
	ui8 specialWeek = 0; //weekType
	CreatureID creatureid; //for creature weeks

	NewTurn() = default;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & type;
		h & text;
		h & components;
		h & player;
		h & soundID;
	}
	InfoWindow() = default;
};

struct DLL_LINKAGE SetObjectProperty : public CPackForClient
{
	void applyGs(CGameState * gs) const;
	ObjectInstanceID id;
	ObjProperty what{};

	ObjPropertyID identifier;

	SetObjectProperty() = default;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & id;
		h & what;
		h & identifier;
	}
};

struct DLL_LINKAGE ChangeObjectVisitors : public CPackForClient
{
	enum VisitMode
	{
		VISITOR_ADD,      // mark hero as one that have visited this object
		VISITOR_ADD_TEAM, // mark team as one that have visited this object
		VISITOR_GLOBAL,   // mark player as one that have visited object of this type
		VISITOR_REMOVE,   // unmark visitor, reversed to ADD
		VISITOR_CLEAR     // clear all visitors from this object (object reset)
	};
	VisitMode mode = VISITOR_CLEAR; // uses VisitMode enum
	ObjectInstanceID object;
	ObjectInstanceID hero; // note: hero owner will be also marked as "visited" this object

	void applyGs(CGameState * gs) const;

	void visitTyped(ICPackVisitor & visitor) override;

	ChangeObjectVisitors() = default;

	ChangeObjectVisitors(VisitMode mode, const ObjectInstanceID & object, const ObjectInstanceID & heroID = ObjectInstanceID(-1))
		: mode(mode)
		, object(object)
		, hero(heroID)
	{
	}

	template <typename Handler> void serialize(Handler & h)
	{
		h & object;
		h & hero;
		h & mode;
	}
};

struct DLL_LINKAGE HeroLevelUp : public Query
{
	PlayerColor player;
	ObjectInstanceID heroId;

	PrimarySkill primskill = PrimarySkill::ATTACK;
	std::vector<SecondarySkill> skills;

	void applyGs(CGameState * gs) const;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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
	ObjectInstanceID objid;
	ObjectInstanceID hid;
	bool removableUnits = false;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	TeleportDialog(const ObjectInstanceID & hero, const TeleportChannelID & Channel)
		: hero(hero)
		, channel(Channel)
	{
	}
	ObjectInstanceID hero;
	TeleportChannelID channel;
	TTeleportExitsList exits;
	bool impassable = false;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & queryID;
		h & hero;
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & queryID;
		h & player;
		h & icon;
		h & title;
		h & description;
		h & objects;
	}
};

struct DLL_LINKAGE AdvmapSpellCast : public CPackForClient
{
	ObjectInstanceID casterID;
	SpellID spellID;
	template <typename Handler> void serialize(Handler & h)
	{
		h & casterID;
		h & spellID;
	}

protected:
	void visitTyped(ICPackVisitor & visitor) override;
};

struct DLL_LINKAGE ShowWorldViewEx : public CPackForClient
{
	PlayerColor player;
	bool showTerrain; // TODO: send terrain state

	std::vector<ObjectPosInfo> objectPositions;

	template <typename Handler> void serialize(Handler & h)
	{
		h & player;
		h & showTerrain;
		h & objectPositions;
	}

protected:
	void visitTyped(ICPackVisitor & visitor) override;
};

struct DLL_LINKAGE PlayerMessageClient : public CPackForClient
{
	PlayerMessageClient() = default;
	PlayerMessageClient(const PlayerColor & Player, std::string Text)
		: player(Player)
		, text(std::move(Text))
	{
	}
	void visitTyped(ICPackVisitor & visitor) override;

	PlayerColor player;
	std::string text;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & pos;
		h & player;
		h & focusTime;
	}
};

VCMI_LIB_NAMESPACE_END
