#include "../lib/NetPacks.h"
#include "CGameHandler.h"


#define PLAYER_OWNS(id) (gh->getPlayerAt(c)==gh->getOwner(id))
#define ERROR_AND_RETURN	{if(c) *c << &SystemMessage("You are not allowed to perform this action!");	\
							tlog1<<"Player is not allowed to perform this action!\n";	\
							return false;}
#define ERROR_IF_NOT_OWNS(id)	if(!PLAYER_OWNS(id)) ERROR_AND_RETURN

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

bool CloseServer::applyGh( CGameHandler *gh )
{
	gh->close();
	return true;
}

bool EndTurn::applyGh( CGameHandler *gh )
{
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

bool ArrangeStacks::applyGh( CGameHandler *gh )
{
	//checks for owning in the gh func
	return gh->arrangeStacks(id1,id2,what,p1,p2,val);
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
	ERROR_IF_NOT_OWNS(tid);
	return gh->recruitCreatures(tid,crid,amount);
}

bool UpgradeCreature::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(id);
	return gh->upgradeCreature(id,pos,cid);
}

bool GarrisonHeroSwap::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(tid);
	return gh->garrisonSwap(tid);
}

bool ExchangeArtifacts::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid1);
	ERROR_IF_NOT_OWNS(hid2);
	return gh->swapArtifacts(hid1,hid2,slot1,slot2);
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
	if(gh->getPlayerAt(c) != player) ERROR_AND_RETURN;
	return gh->tradeResources(val,player,r1,r2);
}

bool SetFormation::applyGh( CGameHandler *gh )
{	
	ERROR_IF_NOT_OWNS(hid);
	return gh->setFormation(hid,formation);
}

bool HireHero::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(tid);
	return gh->hireHero(tid,hid);
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
	if(!GS(gh)->curB) ERROR_AND_RETURN;
	if(gh->connections[GS(gh)->curB->getStack(GS(gh)->curB->activeStack)->owner] != c) ERROR_AND_RETURN;
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
	if(gh->getPlayerAt(c) != player) ERROR_AND_RETURN;
	gh->playerMessage(player,text);
	return true;
}

bool SetSelection::applyGh( CGameHandler *gh )
{
	if(gh->getPlayerAt(c) != player) ERROR_AND_RETURN;
	if(!gh->getObj(id)) ERROR_AND_RETURN;
	gh->sendAndApply(this);
	return true;
}