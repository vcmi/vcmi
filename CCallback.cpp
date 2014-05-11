#include "StdInc.h"
#include "CCallback.h"

#include "lib/CCreatureHandler.h"
#include "client/CGameInfo.h"
#include "lib/CGameState.h"
#include "lib/BattleState.h"
#include "client/CPlayerInterface.h"
#include "client/Client.h"
#include "lib/mapping/CMap.h"
#include "lib/CBuildingHandler.h"
#include "lib/CDefObjInfoHandler.h"
#include "lib/CGeneralTextHandler.h"
#include "lib/CHeroHandler.h"
#include "lib/CObjectHandler.h"
#include "lib/Connection.h"
#include "lib/NetPacks.h"
#include "client/mapHandler.h"
#include "lib/CSpellHandler.h"
#include "lib/CArtHandler.h"
#include "lib/GameConstants.h"
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#include "lib/UnlockGuard.h"

/*
 * CCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

template <ui16 N> bool isType(CPack *pack)
{
	return pack->getType() == N;
}

bool CCallback::teleportHero(const CGHeroInstance *who, const CGTownInstance *where)
{
	CastleTeleportHero pack(who->id, where->id, 1);
	sendRequest(&pack);
	return true;
}

bool CCallback::moveHero(const CGHeroInstance *h, int3 dst)
{
	MoveHero pack(dst,h->id);
	sendRequest(&pack);
	return true;
}

int CCallback::selectionMade(int selection, QueryID queryID)
{
	ASSERT_IF_CALLED_WITH_PLAYER
	if(queryID == QueryID(-1))
	{
        logGlobal->errorStream() << "Cannot answer the query -1!";
		return false;
	}

	QueryReply pack(queryID,selection);
	pack.player = *player;
	return sendRequest(&pack);
}

void CCallback::recruitCreatures(const CGObjectInstance *obj, CreatureID ID, ui32 amount, si32 level/*=-1*/)
{
	if(player!=obj->tempOwner  &&  obj->ID != Obj::WAR_MACHINE_FACTORY)
		return;

	RecruitCreatures pack(obj->id,ID,amount,level);
	sendRequest(&pack);
}

bool CCallback::dismissCreature(const CArmedInstance *obj, SlotID stackPos)
{
	if(((player>=0)  &&  obj->tempOwner != player) || (obj->stacksCount()<2  && obj->needsLastStack()))
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
    logGlobal->traceStream() << "Player " << *player << " ended his turn.";
	EndTurn pack;
	sendRequest(&pack); //report that we ended turn
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

bool CCallback::dismissHero(const CGHeroInstance *hero)
{
	if(player!=hero->tempOwner) return false;

	DismissHero pack(hero->id);
	sendRequest(&pack);
	return true;
}

// int CCallback::getMySerial() const
// {
// 	boost::shared_lock<boost::shared_mutex> lock(*gs->mx);
// 	return gs->players[player].serial;
// }

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

bool CCallback::buildBuilding(const CGTownInstance *town, BuildingID buildingID)
{
	if(town->tempOwner!=player)
		return false;

	if(!canBuildStructure(town, buildingID))
		return false;

	BuildStructure pack(town->id,buildingID);
	sendRequest(&pack);
	return true;
}

int CBattleCallback::battleMakeAction(BattleAction* action)
{
	assert(action->actionType == Battle::HERO_SPELL);
	MakeCustomAction mca(*action);
	sendRequest(&mca);
	return 0;
}

int CBattleCallback::sendRequest(const CPack *request)
{
	int requestID = cl->sendRequest(request, *player);
	if(waitTillRealize)
	{
        logGlobal->traceStream() << boost::format("We'll wait till request %d is answered.\n") % requestID;
		auto gsUnlocker = vstd::makeUnlockSharedGuardIf(getGsMutex(), unlockGsWhenWaiting);
		cl->waitingRequest.waitWhileContains(requestID);
	}

	boost::this_thread::interruption_point();
	return requestID;
}

void CCallback::swapGarrisonHero( const CGTownInstance *town )
{
	if(town->tempOwner == *player
	   || (town->garrisonHero && town->garrisonHero->tempOwner == *player ))
	{
		GarrisonHeroSwap pack(town->id);
		sendRequest(&pack);
	}
}

void CCallback::buyArtifact(const CGHeroInstance *hero, ArtifactID aid)
{
	if(hero->tempOwner != player) return;

	BuyArtifact pack(hero->id,aid);
	sendRequest(&pack);
}

void CCallback::trade(const CGObjectInstance *market, EMarketMode::EMarketMode mode, int id1, int id2, int val1, const CGHeroInstance *hero/* = nullptr*/)
{
	TradeOnMarketplace pack;
	pack.market = market;
	pack.hero = hero;
	pack.mode = mode;
	pack.r1 = id1;
	pack.r2 = id2;
	pack.val = val1;
	sendRequest(&pack);
}

void CCallback::setFormation(const CGHeroInstance * hero, bool tight)
{
	const_cast<CGHeroInstance*>(hero)-> formation = tight;
	SetFormation pack(hero->id,tight);
	sendRequest(&pack);
}

void CCallback::setSelection(const CArmedInstance * obj)
{
	if(!player || obj->getOwner() != *player)
	{
		logGlobal->errorStream() << boost::format("Cannot set selection to the object that is not owned. Object owner is %s, callback player %s") % obj->getOwner() % player;
		return;
	}

	SetSelection ss;
	ss.player = *player;
	ss.id = obj->id;
	sendRequest(&(CPackForClient&)ss);

	if(obj->getOwner() != *player)
	{
		// Cf. bug #1679 http://bugs.vcmi.eu/view.php?id=1679
		logGlobal->warnStream() << "The selection request became invalid because of event that occurred after it was made. Object owner is now " << obj->getOwner();
		throw std::runtime_error("setSelection not allowed");
	}

	if(obj->ID == Obj::HERO)
	{
		if(cl->pathInfo->hero != obj) //calculate new paths only if we selected a different hero
			cl->calculatePaths(static_cast<const CGHeroInstance *>(obj));

		//nasty workaround. TODO: nice workaround
		cl->gs->getPlayer(*player)->currentSelection = obj->id;
	}
}

void CCallback::recruitHero(const CGObjectInstance *townOrTavern, const CGHeroInstance *hero)
{
	assert(townOrTavern);
	assert(hero);
	ui8 i=0;
	for(; i<gs->players[*player].availableHeroes.size(); i++)
	{
		if(gs->players[*player].availableHeroes[i] == hero)
		{
			HireHero pack(i, townOrTavern->id);
			pack.player = *player;
			sendRequest(&pack);
			return;
		}
	}
}

void CCallback::save( const std::string &fname )
{
	cl->save(fname);
}


void CCallback::sendMessage(const std::string &mess)
{
	ASSERT_IF_CALLED_WITH_PLAYER
	PlayerMessage pm(*player, mess);
	sendRequest(&(CPackForClient&)pm);
}

void CCallback::buildBoat(const IShipyard *obj, int subID)
{
	BuildBoat bb;
	bb.objid = obj->o->id;
	bb.subID = subID;

	sendRequest(&bb);
}

CCallback::CCallback( CGameState * GS, boost::optional<PlayerColor> Player, CClient *C )
	:CBattleCallback(GS, Player, C)
{
	waitTillRealize = false;
	unlockGsWhenWaiting = false;
}

CCallback::~CCallback()
{
//trivial, but required. Don`t remove.
}


const CGPathNode * CCallback::getPathInfo( int3 tile )
{
	if (!gs->map->isInTheMap(tile))
		return nullptr;

	validatePaths();
	return &cl->pathInfo->nodes[tile.x][tile.y][tile.z];
}

int CCallback::getDistance( int3 tile )
{
	CGPath ret;
	if (getPath2 (tile, ret))
		return ret.nodes.size();
	else
		return 255;
}

bool CCallback::canMoveBetween(const int3 &a, const int3 &b)
{
	//TODO: merge with Pathfinder::canMoveBetween
	return gs->checkForVisitableDir(a, b) && gs->checkForVisitableDir(b, a);
}

bool CCallback::getPath2( int3 dest, CGPath &ret )
{
	if (!gs->map->isInTheMap(dest))
		return false;

	validatePaths();

	boost::unique_lock<boost::mutex> pathLock(cl->pathMx);
	return cl->pathInfo->getPath(dest, ret);
}

int CCallback::getMovementCost(const CGHeroInstance * hero, int3 dest)
{
	return gs->getMovementCost(hero, hero->visitablePos(), dest, hero->hasBonusOfType (Bonus::FLYING_MOVEMENT), hero->movement);
}

int3 CCallback::getGuardingCreaturePosition(int3 tile)
{
	if (!gs->map->isInTheMap(tile))
		return int3(-1,-1,-1);

	validatePaths();

	boost::unique_lock<boost::mutex> pathLock(cl->pathMx);
	return gs->map->guardingCreaturePositions[tile.x][tile.y][tile.z];
}

void CCallback::recalculatePaths()
{
	cl->calculatePaths(cl->IGameCallback::getSelectedHero(*player));
}

void CCallback::calculatePaths( const CGHeroInstance *hero, CPathsInfo &out, int3 src /*= int3(-1,-1,-1)*/, int movement /*= -1*/ )
{
	gs->calculatePaths(hero, out, src, movement);
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

void CCallback::unregisterAllInterfaces()
{
	cl->playerint.clear();
	cl->battleints.clear();
}

void CCallback::validatePaths()
{
	ASSERT_IF_CALLED_WITH_PLAYER
	const CGHeroInstance *h = cl->IGameCallback::getSelectedHero(*player);
	if(h  && ( cl->pathInfo->hero != h							//wrong hero
		       || cl->pathInfo->hpos != h->getPosition(false)  //wrong hero position
		       || !cl->pathInfo->isValid)) //paths invalidated by game event
	{
		recalculatePaths();
	}
}

int CCallback::mergeOrSwapStacks(const CArmedInstance *s1, const CArmedInstance *s2, SlotID p1, SlotID p2)
{
	if(s1->getCreature(p1) == s2->getCreature(p2))
		return mergeStacks(s1, s2, p1, p2);
	else
		return swapCreatures(s1, s2, p1, p2);
}

void CCallback::registerGameInterface(shared_ptr<IGameEventsReceiver> gameEvents)
{
	cl->additionalPlayerInts[*player].push_back(gameEvents);
}

void CCallback::registerBattleInterface(shared_ptr<IBattleEventsReceiver> battleEvents)
{
	cl->additionalBattleInts[*player].push_back(battleEvents);
}

void CCallback::unregisterGameInterface(shared_ptr<IGameEventsReceiver> gameEvents)
{
	cl->additionalPlayerInts[*player] -= gameEvents;
}

void CCallback::unregisterBattleInterface(shared_ptr<IBattleEventsReceiver> battleEvents)
{
	cl->additionalBattleInts[*player] -= battleEvents;
}

CBattleCallback::CBattleCallback(CGameState *GS, boost::optional<PlayerColor> Player, CClient *C )
{
	gs = GS;
	player = Player;
	cl = C;
}

bool CBattleCallback::battleMakeTacticAction( BattleAction * action )
{
	assert(cl->gs->curB->tacticDistance);
	MakeAction ma;
	ma.ba = *action;
	sendRequest(&ma);
	return true;
}
