#include "BattleCallback.h"
#include "Client.h"
#include "../lib/CGameState.h"
#include "../lib/BattleState.h"
#include "../lib/NetPacks.h"
#include "../lib/Connection.h"


CBattleCallback::CBattleCallback(CGameState *GS, int Player, CClient *C )
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
int CBattleCallback::battleMakeAction(BattleAction* action)
{
	assert(action->actionType == BattleAction::HERO_SPELL);
	MakeCustomAction mca(*action);
	sendRequest(&mca);
	return 0;
}

void CBattleCallback::sendRequest(const CPack* request)
{

	//TODO should be part of CClient (client owns connection, not CB)
	//but it would have to be very tricky cause template/serialization issues
// 	if(waitTillRealize)
// 		cl->waitingRequest.set(typeList.getTypeID(request));

	cl->serv->sendPack(*request);

// 	if(waitTillRealize)
// 	{
// 		if(unlockGsWhenWaiting)
// 			getGsMutex().unlock_shared();
// 		cl->waitingRequest.waitWhileTrue();
// 		if(unlockGsWhenWaiting)
// 			getGsMutex().lock_shared();
// 	}
}