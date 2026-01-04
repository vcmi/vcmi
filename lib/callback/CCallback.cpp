/*
 * CCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CCallback.h"

#include "../gameState/CGameState.h"
#include "../mapObjects/CGHeroInstance.h"
#include "../mapObjects/CGTownInstance.h"
#include "../mapping/CMap.h"
#include "../networkPacks/PacksForServer.h"
#include "../networkPacks/SaveLocalState.h"

#define ASSERT_IF_CALLED_WITH_PLAYER if(!getPlayerID()) {logGlobal->error(BOOST_CURRENT_FUNCTION); assert(0);}

VCMI_LIB_NAMESPACE_BEGIN

bool CCallback::teleportHero(const CGHeroInstance *who, const CGTownInstance *where)
{
	CastleTeleportHero pack(who->id, where->id, 1);
	sendRequest(pack);
	return true;
}

void CCallback::moveHero(const CGHeroInstance *h, const int3 & destination, bool transit)
{
	MoveHero pack({destination}, h->id, transit);
	sendRequest(pack);
}

void CCallback::moveHero(const CGHeroInstance *h, const std::vector<int3> & path, bool transit)
{
	MoveHero pack(path, h->id, transit);
	sendRequest(pack);
}

int CCallback::selectionMade(int selection, QueryID queryID)
{
	return sendQueryReply(selection, queryID);
}

int CCallback::sendQueryReply(std::optional<int32_t> reply, QueryID queryID)
{
	ASSERT_IF_CALLED_WITH_PLAYER
	if(queryID == QueryID(-1))
	{
		logGlobal->error("Cannot answer the query -1!");
		return -1;
	}

	QueryReply pack(queryID, reply);
	pack.player = *getPlayerID();
	return sendRequest(pack);
}

void CCallback::recruitCreatures(const CGDwelling * obj, const CArmedInstance * dst, CreatureID ID, ui32 amount, si32 level)
{
	// TODO exception for neutral dwellings shouldn't be hardcoded
	if(getPlayerID() != obj->tempOwner && obj->ID != Obj::WAR_MACHINE_FACTORY && obj->ID != Obj::REFUGEE_CAMP)
		return;

	RecruitCreatures pack(obj->id, dst->id, ID, amount, level);
	sendRequest(pack);
}

bool CCallback::dismissCreature(const CArmedInstance *obj, SlotID stackPos)
{
	if((getPlayerID() && obj->tempOwner != getPlayerID()) || (obj->stacksCount()<2  && obj->needsLastStack()))
		return false;

	DisbandCreature pack(stackPos,obj->id);
	sendRequest(pack);
	return true;
}

bool CCallback::upgradeCreature(const CArmedInstance *obj, SlotID stackPos, CreatureID newID)
{
	UpgradeCreature pack(stackPos,obj->id,newID);
	sendRequest(pack);
	return false;
}

void CCallback::endTurn()
{
	logGlobal->trace("Player %d ended his turn.", getPlayerID()->getNum());
	EndTurn pack;
	sendRequest(pack);
}
int CCallback::swapCreatures(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2)
{
	ArrangeStacks pack(1,p1,p2,s1->id,s2->id,0);
	sendRequest(pack);
	return 0;
}

int CCallback::mergeStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2)
{
	ArrangeStacks pack(2,p1,p2,s1->id,s2->id,0);
	sendRequest(pack);
	return 0;
}

int CCallback::splitStack(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2, int val)
{
	ArrangeStacks pack(3,p1,p2,s1->id,s2->id,val);
	sendRequest(pack);
	return 0;
}

int CCallback::bulkMoveArmy(ObjectInstanceID srcArmy, ObjectInstanceID destArmy, SlotID srcSlot)
{
	BulkMoveArmy pack(srcArmy, destArmy, srcSlot);
	sendRequest(pack);
	return 0;
}

int CCallback::bulkSplitStack(ObjectInstanceID armyId, SlotID srcSlot, int howMany)
{
	BulkSplitStack pack(armyId, srcSlot, howMany);
	sendRequest(pack);
	return 0;
}

int CCallback::bulkSplitAndRebalanceStack(ObjectInstanceID armyId, SlotID srcSlot)
{
	BulkSplitAndRebalanceStack pack(armyId, srcSlot);
	sendRequest(pack);
	return 0;
}

int CCallback::bulkMergeStacks(ObjectInstanceID armyId, SlotID srcSlot)
{
	BulkMergeStacks pack(armyId, srcSlot);
	sendRequest(pack);
	return 0;
}

bool CCallback::dismissHero(const CGHeroInstance *hero)
{
	if(getPlayerID()!=hero->tempOwner) return false;

	DismissHero pack(hero->id);
	sendRequest(pack);
	return true;
}

bool CCallback::swapArtifacts(const ArtifactLocation &l1, const ArtifactLocation &l2)
{
	ExchangeArtifacts ea;
	ea.src = l1;
	ea.dst = l2;
	sendRequest(ea);
	return true;
}

/**
 * Assembles or disassembles a combination artifact.
 * @param hero Hero holding the artifact(s).
 * @param artifactSlot The worn slot ID of the combination- or constituent artifact.
 * @param assemble True for assembly operation, false for disassembly.
 * @param assembleTo If assemble is true, this represents the artifact ID of the combination
 * artifact to assemble to. Otherwise it's not used.
 */
void CCallback::assembleArtifacts(const ObjectInstanceID & heroID, ArtifactPosition artifactSlot, bool assemble, ArtifactID assembleTo)
{
	AssembleArtifacts aa(heroID, artifactSlot, assemble, assembleTo);
	sendRequest(aa);
}

void CCallback::bulkMoveArtifacts(ObjectInstanceID srcHero, ObjectInstanceID dstHero, bool swap, bool equipped, bool backpack)
{
	BulkExchangeArtifacts bma(srcHero, dstHero, swap, equipped, backpack);
	sendRequest(bma);
}

void CCallback::scrollBackpackArtifacts(ObjectInstanceID hero, bool left)
{
	ManageBackpackArtifacts mba(hero, ManageBackpackArtifacts::ManageCmd::SCROLL_RIGHT);
	if(left)
		mba.cmd = ManageBackpackArtifacts::ManageCmd::SCROLL_LEFT;
	sendRequest(mba);
}

void CCallback::sortBackpackArtifactsBySlot(const ObjectInstanceID hero)
{
	ManageBackpackArtifacts mba(hero, ManageBackpackArtifacts::ManageCmd::SORT_BY_SLOT);
	sendRequest(mba);
}

void CCallback::sortBackpackArtifactsByCost(const ObjectInstanceID hero)
{
	ManageBackpackArtifacts mba(hero, ManageBackpackArtifacts::ManageCmd::SORT_BY_COST);
	sendRequest(mba);
}

void CCallback::sortBackpackArtifactsByClass(const ObjectInstanceID hero)
{
	ManageBackpackArtifacts mba(hero, ManageBackpackArtifacts::ManageCmd::SORT_BY_CLASS);
	sendRequest(mba);
}

void CCallback::manageHeroCostume(ObjectInstanceID hero, size_t costumeIndex, bool saveCostume)
{
	ManageEquippedArtifacts mea(hero, costumeIndex, saveCostume);
	sendRequest(mea);
}

void CCallback::eraseArtifactByClient(const ArtifactLocation & al)
{
	EraseArtifactByClient ea(al);
	sendRequest(ea);
}

bool CCallback::buildBuilding(const CGTownInstance *town, BuildingID buildingID)
{
	if(town->tempOwner!=getPlayerID())
		return false;

	if(canBuildStructure(town, buildingID) != EBuildingState::ALLOWED)
		return false;

	BuildStructure pack(town->id,buildingID);
	sendRequest(pack);
	return true;
}

bool CCallback::visitTownBuilding(const CGTownInstance *town, BuildingID buildingID)
{
	if(town->tempOwner!=getPlayerID())
		return false;

	VisitTownBuilding pack(town->id, buildingID);
	sendRequest(pack);
	return true;
}

void CCallback::spellResearch( const CGTownInstance *town, SpellID spellAtSlot, bool accepted )
{
	SpellResearch pack(town->id, spellAtSlot, accepted);
	sendRequest(pack);
}

void CCallback::swapGarrisonHero( const CGTownInstance *town )
{
	if(town->tempOwner == getPlayerID() || (town->getGarrisonHero() && town->getGarrisonHero()->tempOwner == getPlayerID() ))
	{
		GarrisonHeroSwap pack(town->id);
		sendRequest(pack);
	}
}

void CCallback::buyArtifact(const CGHeroInstance *hero, ArtifactID aid)
{
	if(hero->tempOwner != getPlayerID()) return;

	BuyArtifact pack(hero->id,aid);
	sendRequest(pack);
}

void CCallback::trade(const ObjectInstanceID marketId, EMarketMode mode, TradeItemSell id1, TradeItemBuy id2, ui32 val1, const CGHeroInstance * hero)
{
	trade(marketId, mode, std::vector(1, id1), std::vector(1, id2), std::vector(1, val1), hero);
}

void CCallback::trade(const ObjectInstanceID marketId, EMarketMode mode, const std::vector<TradeItemSell> & id1, const std::vector<TradeItemBuy> & id2, const std::vector<ui32> & val1, const CGHeroInstance * hero)
{
	TradeOnMarketplace pack;
	pack.marketId = marketId;
	pack.heroId = hero ? hero->id : ObjectInstanceID();
	pack.mode = mode;
	pack.r1 = id1;
	pack.r2 = id2;
	pack.val = val1;
	sendRequest(pack);
}

void CCallback::setFormation(const CGHeroInstance * hero, EArmyFormation mode)
{
	SetFormation pack(hero->id, mode);
	sendRequest(pack);
}

void CCallback::setTownName(const CGTownInstance * town, std::string & name)
{
	SetTownName pack(town->id, name);
	sendRequest(pack);
}

void CCallback::recruitHero(const CGObjectInstance *townOrTavern, const CGHeroInstance *hero, const HeroTypeID & nextHero)
{
	assert(townOrTavern);
	assert(hero);

	HireHero pack(hero->getHeroTypeID(), townOrTavern->id, nextHero);
	pack.player = *getPlayerID();
	sendRequest(pack);
}

void CCallback::saveLocalState(const JsonNode & data)
{
	SaveLocalState state;
	state.data = data;
	state.player = *getPlayerID();

	sendRequest(state);
}

void CCallback::save( const std::string &fname )
{
	SaveGame save_game(fname);
	sendRequest(save_game);
}

void CCallback::gamePause(bool pause)
{
	if(pause)
	{
		GamePause pack;
		pack.player = *getPlayerID();
		sendRequest(pack);
	}
	else
	{
		sendQueryReply(0, QueryID::CLIENT);
	}
}

void CCallback::sendMessage(const std::string &mess, const CGObjectInstance * currentObject)
{
	ASSERT_IF_CALLED_WITH_PLAYER
	PlayerMessage pm(mess, currentObject? currentObject->id : ObjectInstanceID(-1));
	if(getPlayerID())
		pm.player = *getPlayerID();
	sendRequest(pm);
}

void CCallback::buildBoat( const IShipyard *obj )
{
	BuildBoat bb;
	bb.objid = dynamic_cast<const CGObjectInstance*>(obj)->id;
	sendRequest(bb);
}

CCallback::CCallback(std::shared_ptr<CGameState> gamestate, std::optional<PlayerColor> Player, IClient * C)
	: CBattleCallback(Player, C)
	, gamestate(gamestate)
{
	waitTillRealize = false;
	unlockGsWhenWaiting = false;
}

CCallback::~CCallback() = default;

bool CCallback::canMoveBetween(const int3 &a, const int3 &b)
{
	//bidirectional
	return gameState().getMap().canMoveBetween(a, b);
}

std::optional<PlayerColor> CCallback::getPlayerID() const
{
	return CBattleCallback::getPlayerID();
}

int3 CCallback::getGuardingCreaturePosition(int3 tile)
{
	if (!gameState().getMap().isInTheMap(tile))
		return int3(-1,-1,-1);

	return gameState().getMap().guardingCreaturePositions[tile];
}

void CCallback::dig( const CGObjectInstance *hero )
{
	DigWithHero dwh;
	dwh.id = hero->id;
	sendRequest(dwh);
}

void CCallback::castSpell(const CGHeroInstance *hero, SpellID spellID, const int3 &pos)
{
	CastAdvSpell cas;
	cas.hid = hero->id;
	cas.sid = spellID;
	cas.pos = pos;
	sendRequest(cas);
}

void CCallback::requestStatistic()
{
	RequestStatistic sr;
	sendRequest(sr);
}

int CCallback::mergeOrSwapStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2)
{
	if(s1->getCreature(p1) == s2->getCreature(p2))
		return mergeStacks(s1, s2, p1, p2);
	else
		return swapCreatures(s1, s2, p1, p2);
}

VCMI_LIB_NAMESPACE_END
