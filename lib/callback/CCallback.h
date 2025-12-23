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

#include "CPlayerSpecificInfoCallback.h"
#include "CBattleCallback.h"
#include "IGameActionCallback.h"

// in static AI build this file gets included into libvcmi
#ifdef STATIC_AI
VCMI_LIB_USING_NAMESPACE
#endif

VCMI_LIB_NAMESPACE_BEGIN

class IBattleEventsReceiver;

class DLL_LINKAGE CCallback : public CPlayerSpecificInfoCallback, public CBattleCallback, public IGameActionCallback
{
	std::shared_ptr<CGameState> gamestate;

	CGameState & gameState() final { return *gamestate; }
	const CGameState & gameState() const final { return *gamestate; }

public:
	CCallback(std::shared_ptr<CGameState> gamestate, std::optional<PlayerColor> Player, IClient * C);
	virtual ~CCallback();

	//client-specific functionalities (pathfinding)
	virtual bool canMoveBetween(const int3 &a, const int3 &b);
	virtual int3 getGuardingCreaturePosition(int3 tile);

	std::optional<PlayerColor> getPlayerID() const override;

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
	int bulkSplitAndRebalanceStack(ObjectInstanceID armyId, SlotID srcSlot) override;
	int bulkMergeStacks(ObjectInstanceID armyId, SlotID srcSlot) override;
	bool dismissHero(const CGHeroInstance * hero) override;
	bool swapArtifacts(const ArtifactLocation &l1, const ArtifactLocation &l2) override;
	void assembleArtifacts(const ObjectInstanceID & heroID, ArtifactPosition artifactSlot, bool assemble, ArtifactID assembleTo) override;
	void bulkMoveArtifacts(ObjectInstanceID srcHero, ObjectInstanceID dstHero, bool swap, bool equipped = true, bool backpack = true) override;
	void scrollBackpackArtifacts(ObjectInstanceID hero, bool left) override;
	void sortBackpackArtifactsBySlot(const ObjectInstanceID hero) override;
	void sortBackpackArtifactsByCost(const ObjectInstanceID hero) override;
	void sortBackpackArtifactsByClass(const ObjectInstanceID hero) override;
	void manageHeroCostume(ObjectInstanceID hero, size_t costumeIdx, bool saveCostume) override;
	void eraseArtifactByClient(const ArtifactLocation & al) override;
	bool buildBuilding(const CGTownInstance *town, BuildingID buildingID) override;
	bool visitTownBuilding(const CGTownInstance *town, BuildingID buildingID) override;
	void recruitCreatures(const CGDwelling * obj, const CArmedInstance * dst, CreatureID ID, ui32 amount, si32 level=-1) override;
	bool dismissCreature(const CArmedInstance *obj, SlotID stackPos) override;
	bool upgradeCreature(const CArmedInstance *obj, SlotID stackPos, CreatureID newID=CreatureID::NONE) override;
	void saveLocalState(const JsonNode & data) override;
	void endTurn() override;
	void spellResearch(const CGTownInstance *town, SpellID spellAtSlot, bool accepted) override;
	void swapGarrisonHero(const CGTownInstance *town) override;
	void buyArtifact(const CGHeroInstance *hero, ArtifactID aid) override;
	void trade(const ObjectInstanceID marketId, EMarketMode mode, TradeItemSell id1, TradeItemBuy id2, ui32 val1, const CGHeroInstance * hero = nullptr) override;
	void trade(const ObjectInstanceID marketId, EMarketMode mode, const std::vector<TradeItemSell> & id1, const std::vector<TradeItemBuy> & id2, const std::vector<ui32> & val1, const CGHeroInstance * hero = nullptr) override;
	void setFormation(const CGHeroInstance * hero, EArmyFormation mode) override;
	void setTownName(const CGTownInstance * town, std::string & name) override;
	void recruitHero(const CGObjectInstance *townOrTavern, const CGHeroInstance *hero, const HeroTypeID & nextHero=HeroTypeID::NONE) override;
	void save(const std::string &fname) override;
	void sendMessage(const std::string &mess, const CGObjectInstance * currentObject = nullptr) override;
	void gamePause(bool pause) override;
	void buildBoat(const IShipyard *obj) override;
	void dig(const CGObjectInstance *hero) override;
	void castSpell(const CGHeroInstance *hero, SpellID spellID, const int3 &pos = int3(-1, -1, -1)) override;
	void requestStatistic() override;

//friends
	friend class CClient;
};

VCMI_LIB_NAMESPACE_END
