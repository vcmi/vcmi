#define VCMI_DLL
#include "CBattleCallback.h"
#include "NetPacks.h"
#include "Connection.h"
#include "CGameState.h"
#include <boost/thread/shared_mutex.hpp>
#include "BattleState.h"

CBattleCallback::CBattleCallback(CGameState *GS, int Player, IConnectionHandler *C )
{
	gs = GS;
	player = Player;
	connHandler = C;
}

bool CBattleCallback::battleMakeTacticAction( BattleAction * action )
{
	assert(gs->curB->tacticDistance);
	if(action->actionType == BattleAction::WALK)
	{
		if(!gs->curB->isInTacticRange(action->destinationTile))
		{
			tlog0 << "Requesting movement to tile that is not in tactics range? Illegal!\n";
			if(!action->destinationTile.isValid())
				tlog0 << "Moreover the hex is invalid!!!\n";
			return false;
		}
	}
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
	if(waitTillRealize)
	 	connHandler->waitingRequest.set(typeList.getTypeID(request));

	connHandler->serv->sendPack(*request);

	if(waitTillRealize)
	{
	 	if(unlockGsWhenWaiting)
	 		gs->mx->unlock_shared();
	 	connHandler->waitingRequest.waitWhileTrue();
	 	if(unlockGsWhenWaiting)
	 		gs->mx->lock_shared();
	}
}

IConnectionHandler::IConnectionHandler()
	: waitingRequest(0)
{

}

IConnectionHandler::~IConnectionHandler()
{

}