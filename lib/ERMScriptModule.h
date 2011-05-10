#pragma once

#include "../global.h"
#include "IGameEventsReceiver.h"

class ERMInterpreter;

class DLL_EXPORT CScriptingModule : public IGameEventsReceiver, public IBattleEventsReceiver
{
public:

	virtual void init(){}; //called upon the start of game (after map randomization, before first turn)
	virtual ~CScriptingModule();
};

class DLL_EXPORT CERMScriptModule : public CScriptingModule
{
public:
	ERMInterpreter *interpreter;

	CERMScriptModule(void);
	~CERMScriptModule(void);

	virtual void heroVisit(const CGHeroInstance *visitor, const CGObjectInstance *visitedObj, bool start) OVERRIDE;
	virtual void init() OVERRIDE;
	virtual void battleStart(const CCreatureSet *army1, const CCreatureSet *army2, int3 tile, const CGHeroInstance *hero1, const CGHeroInstance *hero2, bool side) OVERRIDE;
};

