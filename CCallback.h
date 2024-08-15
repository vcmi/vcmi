/*
 * CCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "lib/CGameInfoCallback.h"
#include "lib/battle/CPlayerBattleCallback.h"
#include "lib/int3.h" // for int3
#include "lib/networkPacks/TradeItem.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGHeroInstance;
class CGameState;
struct CPath;
class CGObjectInstance;
class CArmedInstance;
class BattleAction;
class CGTownInstance;
class IShipyard;
struct CGPathNode;
struct CGPath;
struct CPathsInfo;
class PathfinderConfig;
struct CPack;
struct CPackForServer;
class IBattleEventsReceiver;
class IGameEventsReceiver;
struct ArtifactLocation;
class BattleStateInfoForRetreat;
class IMarket;

VCMI_LIB_NAMESPACE_END

// in static AI build this file gets included into libvcmi
#ifdef STATIC_AI
VCMI_LIB_USING_NAMESPACE
#endif

class CClient;
struct lua_State;

class IBattleCallback
{
public:
	virtual ~IBattleCallback() = default;

	bool waitTillRealize = false; //if true, request functions will return after they are realized by server
	bool unlockGsWhenWaiting = false;//if true after sending each request, gs mutex will be unlocked so the changes can be applied; NOTICE caller must have gs mx locked prior to any call to actiob callback!
	//battle
	virtual void battleMakeSpellAction(const BattleID & battleID, const BattleAction & action) = 0;
	virtual void battleMakeUnitAction(const BattleID & battleID, const BattleAction & action) = 0;
	virtual void battleMakeTacticAction(const BattleID & battleID, const BattleAction & action) = 0;
	virtual std::optional<BattleAction> makeSurrenderRetreatDecision(const BattleID & battleID, const BattleStateInfoForRetreat & battleState) = 0;

	virtual std::shared_ptr<CPlayerBattleCallback> getBattle(const BattleID & battleID) = 0;
	virtual std::optional<PlayerColor> getPlayerID() const = 0;
};

class IGameActionCallback
{
public:
	//hero
	virtual void moveHero(const CGHeroInstance *h, const std::vector<int3> & path, bool transit) =0; //moves hero alongside provided path
	virtual void moveHero(const CGHeroInstance *h, const int3 & destination, bool transit) =0; //moves hero alongside provided path
	virtual bool dismissHero(const CGHeroInstance * hero)=0; //dismisses given hero; true - successfully, false - not successfully
	virtual void dig(const CGObjectInstance *hero)=0;
	virtual void castSpell(const CGHeroInstance *hero, SpellID spellID, const int3 &pos = int3(-1, -1, -1))=0; //cast adventure map spell

	//town
	virtual void recruitHero(const CGObjectInstance *townOrTavern, const CGHeroInstance *hero, const HeroTypeID & nextHero=HeroTypeID::NONE)=0;
	virtual bool buildBuilding(const CGTownInstance *town, BuildingID buildingID)=0;
	virtual bool triggerTownSpecialBuildingAction(const CGTownInstance *town, BuildingSubID::EBuildingSubID subBuildingID)=0;
	virtual void recruitCreatures(const CGDwelling *obj, const CArmedInstance * dst, CreatureID ID, ui32 amount, si32 level=-1)=0;
	virtual bool upgradeCreature(const CArmedInstance *obj, SlotID stackPos, CreatureID newID=CreatureID::NONE)=0; //if newID==-1 then best possible upgrade will be made
	virtual void swapGarrisonHero(const CGTownInstance *town)=0;

	virtual void trade(const IMarket * market, EMarketMode mode, TradeItemSell id1, TradeItemBuy id2, ui32 val1, const CGHeroInstance * hero = nullptr)=0; //mode==0: sell val1 units of id1 resource for id2 resiurce
	virtual void trade(const IMarket * market, EMarketMode mode, const std::vector<TradeItemSell> & id1, const std::vector<TradeItemBuy> & id2, const std::vector<ui32> & val1, const CGHeroInstance * hero = nullptr)=0;

	virtual int selectionMade(int selection, QueryID queryID) =0;
	virtual int sendQueryReply(std::optional<int32_t> reply, QueryID queryID) =0;
	virtual int swapCreatures(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2)=0;//swaps creatures between two possibly different garrisons // TODO: AI-unsafe code - fix it!
	virtual int mergeStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2)=0;//joins first stack to the second (creatures must be same type)
	virtual int mergeOrSwapStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2) =0; //first goes to the second
	virtual int splitStack(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2, int val)=0;//split creatures from the first stack
	//virtual bool swapArtifacts(const CGHeroInstance * hero1, ui16 pos1, const CGHeroInstance * hero2, ui16 pos2)=0; //swaps artifacts between two given heroes
	virtual bool swapArtifacts(const ArtifactLocation &l1, const ArtifactLocation &l2)=0;
	virtual void scrollBackpackArtifacts(ObjectInstanceID hero, bool left) = 0;
	virtual void manageHeroCostume(ObjectInstanceID hero, size_t costumeIndex, bool saveCostume) = 0;
	virtual void assembleArtifacts(const ObjectInstanceID & heroID, ArtifactPosition artifactSlot, bool assemble, ArtifactID assembleTo)=0;
	virtual void eraseArtifactByClient(const ArtifactLocation & al)=0;
	virtual bool dismissCreature(const CArmedInstance *obj, SlotID stackPos)=0;
	virtual void endTurn()=0;
	virtual void buyArtifact(const CGHeroInstance *hero, ArtifactID aid)=0; //used to buy artifacts in towns (including spell book in the guild and war machines in blacksmith)
	virtual void setFormation(const CGHeroInstance * hero, EArmyFormation mode)=0;

	virtual void save(const std::string &fname) = 0;
	virtual void sendMessage(const std::string &mess, const CGObjectInstance * currentObject = nullptr) = 0;
	virtual void gamePause(bool pause) = 0;
	virtual void buildBoat(const IShipyard *obj) = 0;

	// To implement high-level army management bulk actions
	virtual int bulkMoveArmy(ObjectInstanceID srcArmy, ObjectInstanceID destArmy, SlotID srcSlot) = 0;
	virtual int bulkSplitStack(ObjectInstanceID armyId, SlotID srcSlot, int howMany = 1) = 0;
	virtual int bulkSmartSplitStack(ObjectInstanceID armyId, SlotID srcSlot) = 0;
	virtual int bulkMergeStacks(ObjectInstanceID armyId, SlotID srcSlot) = 0;
	
	
	// Moves all artifacts from one hero to another
	virtual void bulkMoveArtifacts(ObjectInstanceID srcHero, ObjectInstanceID dstHero, bool swap, bool equipped, bool backpack) = 0;
};

class CBattleCallback : public IBattleCallback
{
	std::map<BattleID, std::shared_ptr<CPlayerBattleCallback>> activeBattles;

	std::optional<PlayerColor> player;

protected:
	int sendRequest(const CPackForServer * request); //returns requestID (that'll be matched to requestID in PackageApplied)
	CClient *cl;

public:
	CBattleCallback(std::optional<PlayerColor> player, CClient * C);
	void battleMakeSpellAction(const BattleID & battleID, const BattleAction & action) override;//for casting spells by hero - DO NOT use it for moving active stack
	void battleMakeUnitAction(const BattleID & battleID, const BattleAction & action) override;
	void battleMakeTacticAction(const BattleID & battleID, const BattleAction & action) override; // performs tactic phase actions
	std::optional<BattleAction> makeSurrenderRetreatDecision(const BattleID & battleID, const BattleStateInfoForRetreat & battleState) override;

	std::shared_ptr<CPlayerBattleCallback> getBattle(const BattleID & battleID) override;
	std::optional<PlayerColor> getPlayerID() const override;

	void onBattleStarted(const IBattleInfo * info);
	void onBattleEnded(const BattleID & battleID);

	friend class CCallback;
	friend class CClient;
};

class CCallback : public CPlayerSpecificInfoCallback, public CBattleCallback, public IGameActionCallback
{
public:
	CCallback(CGameState * GS, std::optional<PlayerColor> Player, CClient * C);
	virtual ~CCallback();

	//client-specific functionalities (pathfinding)
	virtual bool canMoveBetween(const int3 &a, const int3 &b);
	virtual int3 getGuardingCreaturePosition(int3 tile);
	virtual std::shared_ptr<const CPathsInfo> getPathsInfo(const CGHeroInstance * h);

	std::optional<PlayerColor> getPlayerID() const override;

	//Set of metrhods that allows adding more interfaces for this player that'll receive game event call-ins.
	void registerBattleInterface(std::shared_ptr<IBattleEventsReceiver> battleEvents);
	void unregisterBattleInterface(std::shared_ptr<IBattleEventsReceiver> battleEvents);

//commands
	void moveHero(const CGHeroInstance *h, const std::vector<int3> & path, bool transit) override;
	void moveHero(const CGHeroInstance *h, const int3 & destination, bool transit) override;
	bool teleportHero(const CGHeroInstance *who, const CGTownInstance *where);
	int selectionMade(int selection, QueryID queryID) override;
	int sendQueryReply(std::optional<int32_t> reply, QueryID queryID) override;
	int swapCreatures(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2) override;
	int mergeOrSwapStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2) override; //first goes to the second
	int mergeStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2) override; //first goes to the second
	int splitStack(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2, int val) override;
	int bulkMoveArmy(ObjectInstanceID srcArmy, ObjectInstanceID destArmy, SlotID srcSlot) override;
	int bulkSplitStack(ObjectInstanceID armyId, SlotID srcSlot, int howMany = 1) override;
	int bulkSmartSplitStack(ObjectInstanceID armyId, SlotID srcSlot) override;
	int bulkMergeStacks(ObjectInstanceID armyId, SlotID srcSlot) override;
	bool dismissHero(const CGHeroInstance * hero) override;
	bool swapArtifacts(const ArtifactLocation &l1, const ArtifactLocation &l2) override;
	void assembleArtifacts(const ObjectInstanceID & heroID, ArtifactPosition artifactSlot, bool assemble, ArtifactID assembleTo) override;
	void bulkMoveArtifacts(ObjectInstanceID srcHero, ObjectInstanceID dstHero, bool swap, bool equipped = true, bool backpack = true) override;
	void scrollBackpackArtifacts(ObjectInstanceID hero, bool left) override;
	void manageHeroCostume(ObjectInstanceID hero, size_t costumeIdx, bool saveCostume) override;
	void eraseArtifactByClient(const ArtifactLocation & al) override;
	bool buildBuilding(const CGTownInstance *town, BuildingID buildingID) override;
	bool triggerTownSpecialBuildingAction(const CGTownInstance *town, BuildingSubID::EBuildingSubID subBuildingID) override;
	void recruitCreatures(const CGDwelling * obj, const CArmedInstance * dst, CreatureID ID, ui32 amount, si32 level=-1) override;
	bool dismissCreature(const CArmedInstance *obj, SlotID stackPos) override;
	bool upgradeCreature(const CArmedInstance *obj, SlotID stackPos, CreatureID newID=CreatureID::NONE) override;
	void endTurn() override;
	void swapGarrisonHero(const CGTownInstance *town) override;
	void buyArtifact(const CGHeroInstance *hero, ArtifactID aid) override;
	void trade(const IMarket * market, EMarketMode mode, TradeItemSell id1, TradeItemBuy id2, ui32 val1, const CGHeroInstance * hero = nullptr) override;
	void trade(const IMarket * market, EMarketMode mode, const std::vector<TradeItemSell> & id1, const std::vector<TradeItemBuy> & id2, const std::vector<ui32> & val1, const CGHeroInstance * hero = nullptr) override;
	void setFormation(const CGHeroInstance * hero, EArmyFormation mode) override;
	void recruitHero(const CGObjectInstance *townOrTavern, const CGHeroInstance *hero, const HeroTypeID & nextHero=HeroTypeID::NONE) override;
	void save(const std::string &fname) override;
	void sendMessage(const std::string &mess, const CGObjectInstance * currentObject = nullptr) override;
	void gamePause(bool pause) override;
	void buildBoat(const IShipyard *obj) override;
	void dig(const CGObjectInstance *hero) override;
	void castSpell(const CGHeroInstance *hero, SpellID spellID, const int3 &pos = int3(-1, -1, -1)) override;

//friends
	friend class CClient;
};
