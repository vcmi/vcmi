#include "../lib/NetPacks.h"
#include "CGameHandler.h"


#define PLAYER_OWNS(id) (gh->getPlayerAt(c)==gh->getOwner(id))
#define ERROR_AND_RETURN	{if(c) *c << &SystemMessage("You don't own this object!");	\
							tlog1<<"Player is not allowed to perform this action!\n";	\
							return;}
#define ERROR_IF_NOT_OWNS(id)	if(!PLAYER_OWNS(id)) ERROR_AND_RETURN

CGameState* CPackForServer::GS(CGameHandler *gh)
{
	return gh->gs;
}

void SaveGame::applyGh( CGameHandler *gh )
{
	gh->sendMessageTo(*c,"Saving...");
	gh->save(fname);
	gh->sendMessageTo(*c,"Game has been succesfully saved!");
}

void CloseServer::applyGh( CGameHandler *gh )
{
	gh->close();
}

void EndTurn::applyGh( CGameHandler *gh )
{
	gh->states.setFlag(GS(gh)->currentPlayer,&PlayerStatus::makingTurn,false);
}

void DismissHero::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid);
	gh->removeObject(hid);
}

void MoveHero::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid);
	gh->moveHero(hid,dest,0,gh->getPlayerAt(c));
}

void ArrangeStacks::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(id1);
	ERROR_IF_NOT_OWNS(id2);
	gh->arrangeStacks(id1,id2,what,p1,p2,val);
}

void DisbandCreature::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(id);
	gh->disbandCreature(id,pos);
}

void BuildStructure::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(tid);
	gh->buildStructure(tid,bid);
}

void RecruitCreatures::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(tid);
	gh->recruitCreatures(tid,crid,amount);
}

void UpgradeCreature::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(id);
	gh->upgradeCreature(id,pos,cid);
}

void GarrisonHeroSwap::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(tid);
	gh->garrisonSwap(tid);
}

void ExchangeArtifacts::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid1);
	ERROR_IF_NOT_OWNS(hid2);
	gh->swapArtifacts(hid1,hid2,slot1,slot2);
}

void BuyArtifact::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(hid);
	gh->buyArtifact(hid,aid);
}

void TradeOnMarketplace::applyGh( CGameHandler *gh )
{
	if(gh->getPlayerAt(c) != player) ERROR_AND_RETURN;
	gh->tradeResources(val,player,r1,r2);
}

void SetFormation::applyGh( CGameHandler *gh )
{	
	ERROR_IF_NOT_OWNS(hid);
	gh->setFormation(hid,formation);
}

void HireHero::applyGh( CGameHandler *gh )
{
	ERROR_IF_NOT_OWNS(tid);
	gh->hireHero(tid,hid);
}

void QueryReply::applyGh( CGameHandler *gh )
{
	gh->queryReply(qid,answer);
}

void MakeAction::applyGh( CGameHandler *gh )
{
	if(gh->getPlayerAt(c) != GS(gh)->curB->getStack(GS(gh)->curB->activeStack)->owner) ERROR_AND_RETURN;
	gh->makeBattleAction(ba);
}

void MakeCustomAction::applyGh( CGameHandler *gh )
{
	if(gh->getPlayerAt(c) != GS(gh)->curB->getStack(GS(gh)->curB->activeStack)->owner) ERROR_AND_RETURN;
	gh->makeCustomAction(ba);
}

void PlayerMessage::applyGh( CGameHandler *gh )
{
	if(gh->getPlayerAt(c) != player) ERROR_AND_RETURN;
	gh->playerMessage(player,text);
}

void SetSelection::applyGh( CGameHandler *gh )
{
	if(gh->getPlayerAt(c) != player) ERROR_AND_RETURN;
	gh->sendToAllClients(this);
}