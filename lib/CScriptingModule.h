#pragma once
#include "../global.h"
#include "IGameEventsReceiver.h"
#include "IGameCallback.h"

class CScriptingModule : public IGameEventsReceiver, public IBattleEventsReceiver
{
public:
	virtual void executeUserCommand(const std::string &cmd){};
	virtual void init(){}; //called upon the start of game (after map randomization, before first turn)
	virtual void giveActionCB(IGameEventRealizer *cb){}; 
	virtual void giveInfoCB(CPrivilagedInfoCallback *cb){}; 

	CScriptingModule(){}
	virtual ~CScriptingModule(){}
};