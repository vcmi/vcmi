/*
 * IGameActionCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../constants/EntityIdentifiers.h"
#include "../networkPacks/TradeItem.h"
#include "../int3.h"

VCMI_LIB_NAMESPACE_BEGIN

struct ArtifactLocation;

class JsonNode;
class IShipyard;
class CGHeroInstance;
class CGDwelling;
class CGTownInstance;
class CArmedInstance;
class CGObjectInstance;

enum class EMarketMode : int8_t;
enum class EArmyFormation : int8_t;

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
	virtual bool visitTownBuilding(const CGTownInstance *town, BuildingID buildingID)=0;
	virtual void recruitCreatures(const CGDwelling *obj, const CArmedInstance * dst, CreatureID ID, ui32 amount, si32 level=-1)=0;
	virtual bool upgradeCreature(const CArmedInstance *obj, SlotID stackPos, CreatureID newID=CreatureID::NONE)=0; //if newID==-1 then best possible upgrade will be made
	virtual void spellResearch(const CGTownInstance *town, SpellID spellAtSlot, bool accepted)=0;
	virtual void swapGarrisonHero(const CGTownInstance *town)=0;

	virtual void trade(const ObjectInstanceID marketId, EMarketMode mode, TradeItemSell id1, TradeItemBuy id2, ui32 val1, const CGHeroInstance * hero)=0; //mode==0: sell val1 units of id1 resource for id2 resiurce
	virtual void trade(const ObjectInstanceID marketId, EMarketMode mode, const std::vector<TradeItemSell> & id1, const std::vector<TradeItemBuy> & id2, const std::vector<ui32> & val1, const CGHeroInstance * hero)=0;

	virtual int selectionMade(int selection, QueryID queryID) =0;
	virtual int sendQueryReply(std::optional<int32_t> reply, QueryID queryID) =0;
	virtual int swapCreatures(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2)=0;//swaps creatures between two possibly different garrisons // TODO: AI-unsafe code - fix it!
	virtual int mergeStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2)=0;//joins first stack to the second (creatures must be same type)
	virtual int mergeOrSwapStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2) =0; //first goes to the second
	virtual int splitStack(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2, int val)=0;//split creatures from the first stack
	//virtual bool swapArtifacts(const CGHeroInstance * hero1, ui16 pos1, const CGHeroInstance * hero2, ui16 pos2)=0; //swaps artifacts between two given heroes
	virtual bool swapArtifacts(const ArtifactLocation &l1, const ArtifactLocation &l2)=0;
	virtual void scrollBackpackArtifacts(ObjectInstanceID hero, bool left) = 0;
	virtual void sortBackpackArtifactsBySlot(const ObjectInstanceID hero) = 0;
	virtual void sortBackpackArtifactsByCost(const ObjectInstanceID hero) = 0;
	virtual void sortBackpackArtifactsByClass(const ObjectInstanceID hero) = 0;
	virtual void manageHeroCostume(ObjectInstanceID hero, size_t costumeIndex, bool saveCostume) = 0;
	virtual void assembleArtifacts(const ObjectInstanceID & heroID, ArtifactPosition artifactSlot, bool assemble, ArtifactID assembleTo)=0;
	virtual void eraseArtifactByClient(const ArtifactLocation & al)=0;
	virtual bool dismissCreature(const CArmedInstance *obj, SlotID stackPos)=0;
	virtual void saveLocalState(const JsonNode & data)=0;
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
	virtual int bulkSplitAndRebalanceStack(ObjectInstanceID armyId, SlotID srcSlot) = 0;
	virtual int bulkMergeStacks(ObjectInstanceID armyId, SlotID srcSlot) = 0;


	// Moves all artifacts from one hero to another
	virtual void bulkMoveArtifacts(ObjectInstanceID srcHero, ObjectInstanceID dstHero, bool swap, bool equipped, bool backpack) = 0;
};

VCMI_LIB_NAMESPACE_END
