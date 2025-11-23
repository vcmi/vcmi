/*
 * PacksForServer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "ArtifactLocation.h"
#include "NetPacksBase.h"
#include "TradeItem.h"

#include "../int3.h"
#include "../battle/BattleAction.h"

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE GamePause : public CPackForServer
{
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
	}
};

struct DLL_LINKAGE EndTurn : public CPackForServer
{
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & hid;
	}
};

struct DLL_LINKAGE MoveHero : public CPackForServer
{
	MoveHero() = default;
	MoveHero(const std::vector<int3> & path, const ObjectInstanceID & HID, bool Transit)
		: path(path)
		, hid(HID)
		, transit(Transit)
	{
	}
	std::vector<int3> path;
	ObjectInstanceID hid;
	bool transit = false;

	void visitTyped(ICPackVisitor & visitor) override;

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & path;
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler>
	void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler>
	void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler>
	void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & src;
		h & srcOwner;
	}
};

struct DLL_LINKAGE BulkSplitAndRebalanceStack : public CPackForServer
{
	SlotID src;
	ObjectInstanceID srcOwner;

	BulkSplitAndRebalanceStack() = default;

	BulkSplitAndRebalanceStack(const ObjectInstanceID & srcOwner, const SlotID & src)
		: src(src)
		, srcOwner(srcOwner)
	{
	}

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler>
	void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & tid;
		h & bid;
	}
};

struct DLL_LINKAGE VisitTownBuilding : public CPackForServer
{
	VisitTownBuilding() = default;
	VisitTownBuilding(const ObjectInstanceID & TID, const BuildingID BID)
		: tid(TID)
		, bid(BID)
	{
	}
	ObjectInstanceID tid;
	BuildingID bid;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & tid;
		h & bid;
	}
};

struct DLL_LINKAGE RazeStructure : public BuildStructure
{
	void visitTyped(ICPackVisitor & visitor) override;
};

struct DLL_LINKAGE SpellResearch : public CPackForServer
{
	SpellResearch() = default;
	SpellResearch(const ObjectInstanceID & TID, SpellID spellAtSlot, bool accepted)
		: tid(TID), spellAtSlot(spellAtSlot), accepted(accepted)
	{
	}
	ObjectInstanceID tid;
	SpellID spellAtSlot;
	bool accepted;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & tid;
		h & spellAtSlot;
		h & accepted;
	}
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & tid;
	}
};

struct DLL_LINKAGE ExchangeArtifacts : public CPackForServer
{
	ArtifactLocation src, dst;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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
	bool equipped = true;
	bool backpack = true;

	BulkExchangeArtifacts() = default;
	BulkExchangeArtifacts(const ObjectInstanceID & srcHero, const ObjectInstanceID & dstHero, bool swap, bool equipped, bool backpack)
		: srcHero(srcHero)
		, dstHero(dstHero)
		, swap(swap)
		, equipped(equipped)
		, backpack(backpack)
	{
	}

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & srcHero;
		h & dstHero;
		h & swap;
		h & equipped;
		h & backpack;
	}
};

struct DLL_LINKAGE ManageBackpackArtifacts : public CPackForServer
{
	enum class ManageCmd
	{
		SCROLL_LEFT, SCROLL_RIGHT, SORT_BY_SLOT, SORT_BY_CLASS, SORT_BY_COST
	};
	
	ManageBackpackArtifacts() = default;
	ManageBackpackArtifacts(const ObjectInstanceID & artHolder, const ManageCmd & cmd)
		: artHolder(artHolder)
		, cmd(cmd)
	{
	}

	ObjectInstanceID artHolder;
	ManageCmd cmd;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer&>(*this);
		h & artHolder;
		h & cmd;
	}
};

struct DLL_LINKAGE ManageEquippedArtifacts : public CPackForServer
{
	ManageEquippedArtifacts() = default;
	ManageEquippedArtifacts(const ObjectInstanceID & artHolder, const uint32_t costumeIdx, bool saveCostume = false)
		: artHolder(artHolder)
		, costumeIdx(costumeIdx)
		, saveCostume(saveCostume)
	{
	}

	ObjectInstanceID artHolder;
	uint32_t costumeIdx;
	bool saveCostume;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer&>(*this);
		h & artHolder;
		h & costumeIdx;
		h & saveCostume;
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	EMarketMode mode = EMarketMode::RESOURCE_RESOURCE;
	std::vector<TradeItemSell> r1;
	std::vector<TradeItemBuy> r2; //mode 0: r1 - sold resource, r2 - bought res (exception: when sacrificing art r1 is art id [todo: make r2 preferred slot?]
	std::vector<ui32> val; //units of sold resource

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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
	SetFormation(const ObjectInstanceID & HID, EArmyFormation Formation)
		: hid(HID)
		, formation(Formation)
	{
	}
	ObjectInstanceID hid;
	EArmyFormation formation{};

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & hid;
		h & formation;
	}
};

struct DLL_LINKAGE SetTownName : public CPackForServer
{
	SetTownName() = default;
	;
	SetTownName(const ObjectInstanceID & TID, std::string Name)
		: tid(TID)
		, name(Name)
	{
	}
	ObjectInstanceID tid;
	std::string name;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & tid;
		h & name;
	}
};

struct DLL_LINKAGE HireHero : public CPackForServer
{
	HireHero() = default;
	HireHero(HeroTypeID HID, const ObjectInstanceID & TID, const HeroTypeID & NHID)
		: hid(HID)
		, tid(TID)
		, nhid(NHID)
	{
	}
	HeroTypeID hid; //available hero serial
	HeroTypeID nhid; //next hero
	ObjectInstanceID tid; //town (tavern) id
	PlayerColor player;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & hid;
		h & nhid;
		h & tid;
		h & player;
	}
};

struct DLL_LINKAGE BuildBoat : public CPackForServer
{
	ObjectInstanceID objid; //where player wants to buy a boat

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & objid;
	}
};

struct DLL_LINKAGE QueryReply : public CPackForServer
{
	QueryReply() = default;
	QueryReply(const QueryID & QID, std::optional<int32_t> Reply)
		: qid(QID)
		, reply(Reply)
	{
	}
	QueryID qid;
	std::optional<int32_t> reply;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & qid;
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
	BattleID battleID;

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & ba;
		h & battleID;
	}
};

struct DLL_LINKAGE DigWithHero : public CPackForServer
{
	ObjectInstanceID id; //digging hero id

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & hid;
		h & sid;
		h & pos;
	}
};

struct DLL_LINKAGE RequestStatistic : public CPackForServer
{
	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
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

	void visitTyped(ICPackVisitor & visitor) override;

	template <typename Handler> void serialize(Handler & h)
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

	void visitTyped(ICPackVisitor & visitor) override;

	std::string text;
	ObjectInstanceID currObj; // optional parameter that specifies current object. For cheats :)

	template <typename Handler> void serialize(Handler & h)
	{
		h & static_cast<CPackForServer &>(*this);
		h & text;
		h & currObj;
	}
};

VCMI_LIB_NAMESPACE_END
