#pragma once
#include "../global.h"
#include "IGameCallback.h"
#include "CondSh.h"

class CConnection;

class DLL_EXPORT IConnectionHandler
{
public:
	IConnectionHandler();
	~IConnectionHandler();

	CConnection *serv;
	CondSh<int> waitingRequest;
};

class DLL_EXPORT IBattleCallback
{
public:
	bool waitTillRealize; //if true, request functions will return after they are realized by server
	bool unlockGsWhenWaiting;//if true after sending each request, gs mutex will be unlocked so the changes can be applied; NOTICE caller must have gs mx locked prior to any call to actiob callback!
	//battle
	virtual int battleMakeAction(BattleAction* action)=0;//for casting spells by hero - DO NOT use it for moving active stack
	virtual bool battleMakeTacticAction(BattleAction * action) =0; // performs tactic phase actions
};

class DLL_EXPORT CBattleCallback : public IBattleCallback, public CBattleInfoCallback
{
protected:
	void sendRequest(const CPack *request);
	IConnectionHandler *connHandler;
	//virtual bool hasAccess(int playerId) const;

public:
	CBattleCallback(CGameState *GS, int Player, IConnectionHandler *C);
	int battleMakeAction(BattleAction* action) OVERRIDE;//for casting spells by hero - DO NOT use it for moving active stack
	bool battleMakeTacticAction(BattleAction * action) OVERRIDE; // performs tactic phase actions

	friend class CCallback;
	friend class CClient;
};
