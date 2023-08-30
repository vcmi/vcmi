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

#include "lib/CCreatureHandler.h"
#include "lib/gameState/CGameState.h"
#include "client/CPlayerInterface.h"
#include "client/Client.h"
#include "lib/mapping/CMap.h"
#include "lib/CBuildingHandler.h"
#include "lib/CGeneralTextHandler.h"
#include "lib/CHeroHandler.h"
#include "lib/NetPacks.h"
#include "lib/CArtHandler.h"
#include "lib/GameConstants.h"
#include "lib/CPlayerState.h"
#include "lib/UnlockGuard.h"
#include "lib/battle/BattleInfo.h"

bool CCallback::teleportHero(const CGHeroInstance *who, const CGTownInstance *where)
{
	CastleTeleportHero pack(who->id, where->id, 1);
	sendRequest(&pack);
	return true;
}

bool CCallback::moveHero(const CGHeroInstance *h, int3 dst, bool transit)
{
	MoveHero pack(dst,h->id,transit);
	sendRequest(&pack);
	return true;
}

int CCallback::selectionMade(int selection, QueryID queryID)
{
	JsonNode reply(JsonNode::JsonType::DATA_INTEGER);
	reply.Integer() = selection;
	return sendQueryReply(reply, queryID);
}

int CCallback::sendQueryReply(const JsonNode & reply, QueryID queryID)
{
	ASSERT_IF_CALLED_WITH_PLAYER
	if(queryID == QueryID(-1))
	{
		logGlobal->error("Cannot answer the query -1!");
		return -1;
	}

	QueryReply pack(queryID, reply);
	pack.player = *player;
	return sendRequest(&pack);
}

void CCallback::recruitCreatures(const CGDwelling * obj, const CArmedInstance * dst, CreatureID ID, ui32 amount, si32 level)
{
	// TODO exception for neutral dwellings shouldn't be hardcoded
	if(player != obj->tempOwner && obj->ID != Obj::WAR_MACHINE_FACTORY && obj->ID != Obj::REFUGEE_CAMP)
		return;

	RecruitCreatures pack(obj->id, dst->id, ID, amount, level);
	sendRequest(&pack);
}

bool CCallback::dismissCreature(const CArmedInstance *obj, SlotID stackPos)
{
	if((player && obj->tempOwner != player) || (obj->stacksCount()<2  && obj->needsLastStack()))
		return false;

	DisbandCreature pack(stackPos,obj->id);
	sendRequest(&pack);
	return true;
}

bool CCallback::upgradeCreature(const CArmedInstance *obj, SlotID stackPos, CreatureID newID)
{
	UpgradeCreature pack(stackPos,obj->id,newID);
	sendRequest(&pack);
	return false;
}

void CCallback::endTurn()
{
	logGlobal->trace("Player %d ended his turn.", player->getNum());
	EndTurn pack;
	sendRequest(&pack);
}
int CCallback::swapCreatures(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2)
{
	ArrangeStacks pack(1,p1,p2,s1->id,s2->id,0);
	sendRequest(&pack);
	return 0;
}

int CCallback::mergeStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2)
{
	ArrangeStacks pack(2,p1,p2,s1->id,s2->id,0);
	sendRequest(&pack);
	return 0;
}

int CCallback::splitStack(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2, int val)
{
	ArrangeStacks pack(3,p1,p2,s1->id,s2->id,val);
	sendRequest(&pack);
	return 0;
}

int CCallback::bulkMoveArmy(ObjectInstanceID srcArmy, ObjectInstanceID destArmy, SlotID srcSlot)
{
	BulkMoveArmy pack(srcArmy, destArmy, srcSlot);
	sendRequest(&pack);
	return 0;
}

int CCallback::bulkSplitStack(ObjectInstanceID armyId, SlotID srcSlot, int howMany)
{
	BulkSplitStack pack(armyId, srcSlot, howMany);
	sendRequest(&pack);
	return 0;
}

int CCallback::bulkSmartSplitStack(ObjectInstanceID armyId, SlotID srcSlot)
{
	BulkSmartSplitStack pack(armyId, srcSlot);
	sendRequest(&pack);
	return 0;
}

int CCallback::bulkMergeStacks(ObjectInstanceID armyId, SlotID srcSlot)
{
	BulkMergeStacks pack(armyId, srcSlot);
	sendRequest(&pack);
	return 0;
}

bool CCallback::dismissHero(const CGHeroInstance *hero)
{
	if(player!=hero->tempOwner) return false;

	DismissHero pack(hero->id);
	sendRequest(&pack);
	return true;
}

bool CCallback::swapArtifacts(const ArtifactLocation &l1, const ArtifactLocation &l2)
{
	ExchangeArtifacts ea;
	ea.src = l1;
	ea.dst = l2;
	sendRequest(&ea);
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
bool CCallback::assembleArtifacts (const CGHeroInstance * hero, ArtifactPosition artifactSlot, bool assemble, ArtifactID assembleTo)
{
	if (player != hero->tempOwner)
		return false;

	AssembleArtifacts aa(hero->id, artifactSlot, assemble, assembleTo);
	sendRequest(&aa);
	return true;
}

void CCallback::bulkMoveArtifacts(ObjectInstanceID srcHero, ObjectInstanceID dstHero, bool swap)
{
	BulkExchangeArtifacts bma(srcHero, dstHero, swap);
	sendRequest(&bma);
}

void CCallback::eraseArtifactByClient(const ArtifactLocation & al)
{
	EraseArtifactByClient ea(al);
	sendRequest(&ea);
}

bool CCallback::buildBuilding(const CGTownInstance *town, BuildingID buildingID)
{
	if(town->tempOwner!=player)
		return false;

	if(canBuildStructure(town, buildingID) != EBuildingState::ALLOWED)
		return false;

	BuildStructure pack(town->id,buildingID);
	sendRequest(&pack);
	return true;
}

void CBattleCallback::battleMakeSpellAction(const BattleID & battleID, const BattleAction & action)
{
	assert(action.actionType == EActionType::HERO_SPELL);
	MakeAction mca(action);
	sendRequest(&mca);
}

int CBattleCallback::sendRequest(const CPackForServer * request)
{
	int requestID = cl->sendRequest(request, *getPlayerID());
	if(waitTillRealize)
	{
		logGlobal->trace("We'll wait till request %d is answered.\n", requestID);
		auto gsUnlocker = vstd::makeUnlockSharedGuardIf(CGameState::mutex, unlockGsWhenWaiting);
		CClient::waitingRequest.waitWhileContains(requestID);
	}

	boost::this_thread::interruption_point();
	return requestID;
}

void CCallback::swapGarrisonHero( const CGTownInstance *town )
{
	if(town->tempOwner == *player || (town->garrisonHero && town->garrisonHero->tempOwner == *player ))
	{
		GarrisonHeroSwap pack(town->id);
		sendRequest(&pack);
	}
}

void CCallback::buyArtifact(const CGHeroInstance *hero, ArtifactID aid)
{
	if(hero->tempOwner != *player) return;

	BuyArtifact pack(hero->id,aid);
	sendRequest(&pack);
}

void CCallback::trade(const IMarket * market, EMarketMode mode, ui32 id1, ui32 id2, ui32 val1, const CGHeroInstance * hero)
{
	trade(market, mode, std::vector<ui32>(1, id1), std::vector<ui32>(1, id2), std::vector<ui32>(1, val1), hero);
}

void CCallback::trade(const IMarket * market, EMarketMode mode, const std::vector<ui32> & id1, const std::vector<ui32> & id2, const std::vector<ui32> & val1, const CGHeroInstance * hero)
{
	TradeOnMarketplace pack;
	pack.marketId = dynamic_cast<const CGObjectInstance *>(market)->id;
	pack.heroId = hero ? hero->id : ObjectInstanceID();
	pack.mode = mode;
	pack.r1 = id1;
	pack.r2 = id2;
	pack.val = val1;
	sendRequest(&pack);
}

void CCallback::setFormation(const CGHeroInstance * hero, bool tight)
{
	SetFormation pack(hero->id,tight);
	sendRequest(&pack);
}

void CCallback::recruitHero(const CGObjectInstance *townOrTavern, const CGHeroInstance *hero)
{
	assert(townOrTavern);
	assert(hero);

	HireHero pack(HeroTypeID(hero->subID), townOrTavern->id);
	pack.player = *player;
	sendRequest(&pack);
}

void CCallback::save( const std::string &fname )
{
	cl->save(fname);
}


void CCallback::sendMessage(const std::string &mess, const CGObjectInstance * currentObject)
{
	ASSERT_IF_CALLED_WITH_PLAYER
	PlayerMessage pm(mess, currentObject? currentObject->id : ObjectInstanceID(-1));
	if(player)
		pm.player = *player;
	sendRequest(&pm);
}

void CCallback::buildBoat( const IShipyard *obj )
{
	BuildBoat bb;
	bb.objid = dynamic_cast<const CGObjectInstance*>(obj)->id;
	sendRequest(&bb);
}

CCallback::CCallback(CGameState * GS, std::optional<PlayerColor> Player, CClient * C)
	: CBattleCallback(Player, C)
{
	gs = GS;

	waitTillRealize = false;
	unlockGsWhenWaiting = false;
}

CCallback::~CCallback() = default;

bool CCallback::canMoveBetween(const int3 &a, const int3 &b)
{
	//bidirectional
	return gs->map->canMoveBetween(a, b);
}

std::shared_ptr<const CPathsInfo> CCallback::getPathsInfo(const CGHeroInstance * h)
{
	return cl->getPathsInfo(h);
}

std::optional<PlayerColor> CCallback::getPlayerID() const
{
	return CBattleCallback::getPlayerID();
}

int3 CCallback::getGuardingCreaturePosition(int3 tile)
{
	if (!gs->map->isInTheMap(tile))
		return int3(-1,-1,-1);

	return gs->map->guardingCreaturePositions[tile.z][tile.x][tile.y];
}

void CCallback::dig( const CGObjectInstance *hero )
{
	DigWithHero dwh;
	dwh.id = hero->id;
	sendRequest(&dwh);
}

void CCallback::castSpell(const CGHeroInstance *hero, SpellID spellID, const int3 &pos)
{
	CastAdvSpell cas;
	cas.hid = hero->id;
	cas.sid = spellID;
	cas.pos = pos;
	sendRequest(&cas);
}

int CCallback::mergeOrSwapStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2)
{
	if(s1->getCreature(p1) == s2->getCreature(p2))
		return mergeStacks(s1, s2, p1, p2);
	else
		return swapCreatures(s1, s2, p1, p2);
}

void CCallback::registerBattleInterface(std::shared_ptr<IBattleEventsReceiver> battleEvents)
{
	cl->additionalBattleInts[*player].push_back(battleEvents);
}

void CCallback::unregisterBattleInterface(std::shared_ptr<IBattleEventsReceiver> battleEvents)
{
	cl->additionalBattleInts[*player] -= battleEvents;
}

#if SCRIPTING_ENABLED
scripting::Pool * CBattleCallback::getContextPool() const
{
	return cl->getGlobalContextPool();
}
#endif

CBattleCallback::CBattleCallback(std::optional<PlayerColor> player, CClient * C):
	cl(C),
	player(player)
{
}

void CBattleCallback::battleMakeUnitAction(const BattleID & battleID, const BattleAction & action)
{
	assert(!cl->gs->getBattle(battleID)->tacticDistance);
	MakeAction ma;
	ma.ba = action;
	sendRequest(&ma);
}

void CBattleCallback::battleMakeTacticAction(const BattleID & battleID, const BattleAction & action )
{
	assert(cl->gs->getBattle(battleID)->tacticDistance);
	MakeAction ma;
	ma.ba = action;
	sendRequest(&ma);
}

std::optional<BattleAction> CBattleCallback::makeSurrenderRetreatDecision(const BattleID & battleID, const BattleStateInfoForRetreat & battleState)
{
	return cl->playerint[getPlayerID().value()]->makeSurrenderRetreatDecision(battleID, battleState);
}

std::shared_ptr<CPlayerBattleCallback> CBattleCallback::getBattle(const BattleID & battleID)
{
	return activeBattles.at(battleID);
}

std::optional<PlayerColor> CBattleCallback::getPlayerID() const
{
	return player;
}

void CBattleCallback::onBattleStarted(const IBattleInfo * info)
{
	activeBattles[info->getBattleID()] = std::make_shared<CPlayerBattleCallback>(info, *getPlayerID());
}

void CBattleCallback::onBattleEnded(const BattleID & battleID)
{
	activeBattles.erase(battleID);
}
