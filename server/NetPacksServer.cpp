#include "stdafx.h"
#include "../lib/NetPacks.h"
#include "CGameHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/IGameCallback.h"
#include "../lib/map.h"
#include "../lib/CGameState.h"
#include "../lib/BattleState.h"
#include "../lib/BattleAction.h"


#define PLAYER_OWNS(id) (gh->getPlayerAt(c)==gh->getOwner(id))
#define ERROR_AND_RETURN												\
	do { if(c) {														\
			SystemMessage temp_message("You are not allowed to perform this action!"); \
			*c << &temp_message; \
		}																\
		tlog1<<"Player is not allowed to perform this action!\n";		\
		return false;} while(0)

#define WRONG_PLAYER_MSG(expectedplayer) do {std::ostringstream oss;\
			oss << "You were identified as player " << (int)gh->getPlayerAt(c) << " while expecting " << (int)expectedplayer;\
			tlog1 << oss.str() << std::endl; \
			if(c) { SystemMessage temp_message(oss.str()); *c << &temp_message; } } while(0)

#define ERROR_IF_NOT_OWNS(id)	do{if(!PLAYER_OWNS(id)){WRONG_PLAYER_MSG(gh->getOwner(id)); ERROR_AND_RETURN; }}while(0)
#define ERROR_IF_NOT(player)	do{if(player != gh->getPlayerAt(c)){WRONG_PLAYER_MSG(player); ERROR_AND_RETURN; }}while(0)
#define COMPLAIN_AND_RETURN(txt)	{ gh->complain(txt); ERROR_AND_RETURN; }

/*
 * NetPacksServer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */


CGameState* CPackForServer::GS(CGameHandler *gh)
{
	return gh->gs;
}

bool SaveGame::applyGh( CGameHandler *gh )
{
	//gh->sendMessageTo(*c,"Saving...");
	gh->save(fname);
	gh->sendMessageTo(*c,"Game has been saved as " + fname);
	return true;
}

bool CommitPackage::applyGh( CGameHandler *gh )
{
	gh->sendAndApply(packToCommit);
	return true;
}

bool CloseServer::applyGh( CGameHandler *gh )
{
	gh->close();
	return true;
}

bool EndTurn::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT(GS(gh)->currentPlayer);
	gh->states.setFlag(GS(gh)->currentPlayer,&PlayerStatus::makingTurn,false);
	return true;
}

bool DismissHero::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid);
	return gh->removeObject(hid);
}

bool MoveHero::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid);
	return gh->moveHero(hid,dest,0,gh->getPlayerAt(c));
}

bool CastleTeleportHero::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid);
	
	return gh->teleportHero(hid,dest,source,gh->getPlayerAt(c));
}

bool ArrangeStacks::applyGh( CGameHandler *gh )
{
	//checks for owning in the gh func
	return gh->arrangeStacks(id1,id2,what,p1,p2,val,gh->getPlayerAt(c));
}

bool DisbandCreature::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(id);
	return gh->disbandCreature(id,pos);
}

bool BuildStructure::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(tid);
	return gh->buildStructure(tid,bid);
}

bool RecruitCreatures::applyGh( CGameHandler *gh )
{
	return gh->recruitCreatures(tid,crid,amount,level);
}

bool UpgradeCreature::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(id);
	return gh->upgradeCreature(id,pos,cid);
}

bool GarrisonHeroSwap::applyGh( CGameHandler *gh )
{
	const CGTownInstance * town = gh->getTown(tid);
	if (!PLAYER_OWNS(tid) && !( town->garrisonHero && PLAYER_OWNS(town->garrisonHero->id) ) )
		ERROR_AND_RETURN;//neither town nor garrisoned hero (if present) is ours 
	return gh->garrisonSwap(tid);
}

bool ExchangeArtifacts::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid1);//second hero can be ally
	return gh->moveArtifact(hid1,hid2,slot1,slot2);
}

bool AssembleArtifacts::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(heroID);
	return gh->assembleArtifacts(heroID, artifactSlot, assemble, assembleTo);
}

bool BuyArtifact::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid);
	return gh->buyArtifact(hid,aid);
}

bool TradeOnMarketplace::applyGh( CGameHandler *gh )
{
	//market must be owned or visited
	const IMarket *m = IMarket::castFrom(market);

	if(!m)
		COMPLAIN_AND_RETURN("market is not-a-market! :/");

	ui8 player = market->tempOwner;

	if(player >= PLAYER_LIMIT)
		player = gh->getTile(market->visitablePos())->visitableObjects.back()->tempOwner;

	if(player >= PLAYER_LIMIT)
		COMPLAIN_AND_RETURN("No player can use this market!");

	if(hero && (player != hero->tempOwner || hero->visitablePos() != market->visitablePos()))
		COMPLAIN_AND_RETURN("This hero can't use this marketplace!");

	ERROR_IF_NOT(player);

	switch(mode)
	{
	case RESOURCE_RESOURCE:
		return gh->tradeResources(m, val, player, r1, r2);
	case RESOURCE_PLAYER:
		return gh->sendResources(val, player, r1, r2);
	case CREATURE_RESOURCE:
		if(!hero)
			COMPLAIN_AND_RETURN("Only hero can sell creatures!");
		return gh->sellCreatures(val, m, hero, r1, r2);
	case RESOURCE_ARTIFACT:
		if(!hero)
			COMPLAIN_AND_RETURN("Only hero can buy artifacts!");
		return gh->buyArtifact(m, hero, r1, r2);
	case ARTIFACT_RESOURCE:
		if(!hero)
			COMPLAIN_AND_RETURN("Only hero can sell artifacts!");
		return gh->sellArtifact(m, hero, r1, r2);
	case CREATURE_UNDEAD:
		return gh->transformInUndead(m, hero, r1);
	case RESOURCE_SKILL:
		return gh->buySecSkill(m, hero, r2);
	case CREATURE_EXP:
		return gh->sacrificeCreatures(m, hero, r1, val);
	case ARTIFACT_EXP:
		return gh->sacrificeArtifact(m, hero, r1);
	default:
		COMPLAIN_AND_RETURN("Unknown exchange mode!");
	}
}

bool SetFormation::applyGh( CGameHandler *gh )
{	
	ERROR_IF_NOT_OWNS(hid);
	return gh->setFormation(hid,formation);
}

bool HireHero::applyGh( CGameHandler *gh )
{
	const CGObjectInstance *obj = gh->getObj(tid);

	if(obj->ID == TOWNI_TYPE)
		ERROR_IF_NOT_OWNS(tid);
	//TODO check for visiting hero

	return gh->hireHero(obj, hid,player);
}

bool BuildBoat::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(objid);
	return gh->buildBoat(objid);
}

bool QueryReply::applyGh( CGameHandler *gh )
{
	//TODO - check if player matches the query
	return gh->queryReply(qid,answer);
}

bool MakeAction::applyGh( CGameHandler *gh )
{
	const BattleInfo *b = GS(gh)->curB;
	if(!b) ERROR_AND_RETURN;
	
	if(b->tacticDistance)
	{
		if(ba.actionType != BattleAction::WALK  &&  ba.actionType != BattleAction::END_TACTIC_PHASE)
			ERROR_AND_RETURN;
		if(gh->connections[b->sides[b->tacticsSide]] != c) 
			ERROR_AND_RETURN;
	}
	else if(gh->connections[b->getStack(b->activeStack)->owner] != c) 
		ERROR_AND_RETURN;

	return gh->makeBattleAction(ba);
}

bool MakeCustomAction::applyGh( CGameHandler *gh )
{
	if(!GS(gh)->curB) ERROR_AND_RETURN;
	if(gh->connections[GS(gh)->curB->getStack(GS(gh)->curB->activeStack)->owner] != c) ERROR_AND_RETURN;
	return gh->makeCustomAction(ba);
}

bool DigWithHero::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(id);
	return gh->dig(gh->getHero(id));
}

bool CastAdvSpell::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid);
	return gh->castSpell(gh->getHero(hid), sid, pos);
}

bool PlayerMessage::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT(player);
	if(gh->getPlayerAt(c) != player) ERROR_AND_RETURN;
	gh->playerMessage(player,text);
	return true;
}

bool SetSelection::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT(player);
	if(!gh->getObj(id))
	{
		tlog1 << "No such object...\n";
		ERROR_AND_RETURN;
	}
	gh->sendAndApply(this);
	return true;
}
