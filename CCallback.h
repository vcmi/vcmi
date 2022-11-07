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

VCMI_LIB_NAMESPACE_END

class CClient;
struct lua_State;

class IBattleCallback
{
public:
	virtual ~IBattleCallback() = default;

	bool waitTillRealize; //if true, request functions will return after they are realized by server
	bool unlockGsWhenWaiting;//if true after sending each request, gs mutex will be unlocked so the changes can be applied; NOTICE caller must have gs mx locked prior to any call to actiob callback!
	//battle
	virtual int battleMakeAction(const BattleAction * action) = 0;//for casting spells by hero - DO NOT use it for moving active stack
	virtual bool battleMakeTacticAction(BattleAction * action) = 0; // performs tactic phase actions
	virtual boost::optional<BattleAction> makeSurrenderRetreatDecision(const BattleStateInfoForRetreat & battleState) = 0;
};

class IGameActionCallback
{
public:
	//hero
	virtual bool moveHero(const CGHeroInstance *h, int3 dst, bool transit) =0; //dst must be free, neighbouring tile (this function can move hero only by one tile)
	virtual bool dismissHero(const CGHeroInstance * hero)=0; //dismisses given hero; true - successfuly, false - not successfuly
	virtual void dig(const CGObjectInstance *hero)=0;
	virtual void castSpell(const CGHeroInstance *hero, SpellID spellID, const int3 &pos = int3(-1, -1, -1))=0; //cast adventure map spell

	//town
	virtual void recruitHero(const CGObjectInstance *townOrTavern, const CGHeroInstance *hero)=0;
	virtual bool buildBuilding(const CGTownInstance *town, BuildingID buildingID)=0;
	virtual void recruitCreatures(const CGDwelling *obj, const CArmedInstance * dst, CreatureID ID, ui32 amount, si32 level=-1)=0;
	virtual bool upgradeCreature(const CArmedInstance *obj, SlotID stackPos, CreatureID newID=CreatureID::NONE)=0; //if newID==-1 then best possible upgrade will be made
	virtual void swapGarrisonHero(const CGTownInstance *town)=0;

	virtual void trade(const CGObjectInstance * market, EMarketMode::EMarketMode mode, ui32 id1, ui32 id2, ui32 val1, const CGHeroInstance * hero = nullptr)=0; //mode==0: sell val1 units of id1 resource for id2 resiurce
	virtual void trade(const CGObjectInstance * market, EMarketMode::EMarketMode mode, const std::vector<ui32> & id1, const std::vector<ui32> & id2, const std::vector<ui32> & val1, const CGHeroInstance * hero = nullptr)=0;

	virtual int selectionMade(int selection, QueryID queryID) =0;
	virtual int sendQueryReply(const JsonNode & reply, QueryID queryID) =0;
	virtual int swapCreatures(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2)=0;//swaps creatures between two possibly different garrisons // TODO: AI-unsafe code - fix it!
	virtual int mergeStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2)=0;//joins first stack to the second (creatures must be same type)
	virtual int mergeOrSwapStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2) =0; //first goes to the second
	virtual int splitStack(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2, int val)=0;//split creatures from the first stack
	//virtual bool swapArtifacts(const CGHeroInstance * hero1, ui16 pos1, const CGHeroInstance * hero2, ui16 pos2)=0; //swaps artifacts between two given heroes
	virtual bool swapArtifacts(const ArtifactLocation &l1, const ArtifactLocation &l2)=0;
	virtual bool assembleArtifacts(const CGHeroInstance * hero, ArtifactPosition artifactSlot, bool assemble, ArtifactID assembleTo)=0;
	virtual bool dismissCreature(const CArmedInstance *obj, SlotID stackPos)=0;
	virtual void endTurn()=0;
	virtual void buyArtifact(const CGHeroInstance *hero, ArtifactID aid)=0; //used to buy artifacts in towns (including spell book in the guild and war machines in blacksmith)
	virtual void setFormation(const CGHeroInstance * hero, bool tight)=0;

	virtual void save(const std::string &fname) = 0;
	virtual void sendMessage(const std::string &mess, const CGObjectInstance * currentObject = nullptr) = 0;
	virtual void buildBoat(const IShipyard *obj) = 0;

	// To implement high-level army management bulk actions
	virtual int bulkMoveArmy(ObjectInstanceID srcArmy, ObjectInstanceID destArmy, SlotID srcSlot) = 0;
	virtual int bulkSplitStack(ObjectInstanceID armyId, SlotID srcSlot, int howMany = 1) = 0;
	virtual int bulkSmartSplitStack(ObjectInstanceID armyId, SlotID srcSlot) = 0;
	virtual int bulkMergeStacks(ObjectInstanceID armyId, SlotID srcSlot) = 0;
};

class CBattleCallback : public IBattleCallback, public CPlayerBattleCallback
{
protected:
	int sendRequest(const CPackForServer * request); //returns requestID (that'll be matched to requestID in PackageApplied)
	CClient *cl;

public:
	CBattleCallback(boost::optional<PlayerColor> Player, CClient *C);
	int battleMakeAction(const BattleAction * action) override;//for casting spells by hero - DO NOT use it for moving active stack
	bool battleMakeTacticAction(BattleAction * action) override; // performs tactic phase actions
	boost::optional<BattleAction> makeSurrenderRetreatDecision(const BattleStateInfoForRetreat & battleState) override;

#if SCRIPTING_ENABLED
	scripting::Pool * getContextPool() const override;
#endif

	friend class CCallback;
	friend class CClient;
};

class CCallback : public CPlayerSpecificInfoCallback,
	public IGameActionCallback,
	public CBattleCallback
{
public:
	CCallback(CGameState * GS, boost::optional<PlayerColor> Player, CClient *C);
	virtual ~CCallback();

	//client-specific functionalities (pathfinding)
	virtual bool canMoveBetween(const int3 &a, const int3 &b);
	virtual int3 getGuardingCreaturePosition(int3 tile);
	virtual std::shared_ptr<const CPathsInfo> getPathsInfo(const CGHeroInstance * h);

	virtual void calculatePaths(const CGHeroInstance *hero, CPathsInfo &out);

	//Set of metrhods that allows adding more interfaces for this player that'll receive game event call-ins.
	void registerBattleInterface(std::shared_ptr<IBattleEventsReceiver> battleEvents);
	void unregisterBattleInterface(std::shared_ptr<IBattleEventsReceiver> battleEvents);

//commands
	bool moveHero(const CGHeroInstance *h, int3 dst, bool transit = false) override; //dst must be free, neighbouring tile (this function can move hero only by one tile)
	bool teleportHero(const CGHeroInstance *who, const CGTownInstance *where);
	int selectionMade(int selection, QueryID queryID) override;
	int sendQueryReply(const JsonNode & reply, QueryID queryID) override;
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
	bool assembleArtifacts(const CGHeroInstance * hero, ArtifactPosition artifactSlot, bool assemble, ArtifactID assembleTo) override;
	bool buildBuilding(const CGTownInstance *town, BuildingID buildingID) override;
	void recruitCreatures(const CGDwelling * obj, const CArmedInstance * dst, CreatureID ID, ui32 amount, si32 level=-1) override;
	bool dismissCreature(const CArmedInstance *obj, SlotID stackPos) override;
	bool upgradeCreature(const CArmedInstance *obj, SlotID stackPos, CreatureID newID=CreatureID::NONE) override;
	void endTurn() override;
	void swapGarrisonHero(const CGTownInstance *town) override;
	void buyArtifact(const CGHeroInstance *hero, ArtifactID aid) override;
	void trade(const CGObjectInstance * market, EMarketMode::EMarketMode mode, ui32 id1, ui32 id2, ui32 val1, const CGHeroInstance * hero = nullptr) override;
	void trade(const CGObjectInstance * market, EMarketMode::EMarketMode mode, const std::vector<ui32> & id1, const std::vector<ui32> & id2, const std::vector<ui32> & val1, const CGHeroInstance * hero = nullptr) override;
	void setFormation(const CGHeroInstance * hero, bool tight) override;
	void recruitHero(const CGObjectInstance *townOrTavern, const CGHeroInstance *hero) override;
	void save(const std::string &fname) override;
	void sendMessage(const std::string &mess, const CGObjectInstance * currentObject = nullptr) override;
	void buildBoat(const IShipyard *obj) override;
	void dig(const CGObjectInstance *hero) override;
	void castSpell(const CGHeroInstance *hero, SpellID spellID, const int3 &pos = int3(-1, -1, -1)) override;

//friends
	friend class CClient;
};
