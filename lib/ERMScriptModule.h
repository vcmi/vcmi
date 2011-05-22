#pragma once

#include "../global.h"
#include "IGameEventsReceiver.h"
#include "IGameCallback.h"

class IGameEventRealizer;

class ERMInterpreter;

class DLL_EXPORT CScriptingModule : public IGameEventsReceiver, public IBattleEventsReceiver
{
public:

	virtual void init(){}; //called upon the start of game (after map randomization, before first turn)
	virtual ~CScriptingModule();
};

extern DLL_EXPORT IGameEventRealizer *acb;
extern DLL_EXPORT CPrivilagedInfoCallback *icb;


DLL_EXPORT CScriptingModule *getERMModule();