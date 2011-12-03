#pragma once
#include "../global.h"
#include "IGameCallback.h"
#include "CondSh.h"

class CConnection;
struct CPack;

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
///	 if true, request functions will return after they are realized by server
	bool waitTillRealize; 

///	 if true after sending each request, gs mutex will be unlocked so the changes can be applied; NOTICE caller must have gs mx locked prior to any call to actiob callback!
	bool unlockGsWhenWaiting;

	//battle
///	 for casting spells by hero - DO NOT use it for moving active stack
	virtual int battleMakeAction(BattleAction* action)=0; 

///	 performs tactic phase actions
	virtual bool battleMakeTacticAction(BattleAction * action) =0; 

};

class DLL_EXPORT CBattleCallback : public IBattleCallback, public CBattleInfoCallback
{
protected:
	void sendRequest(const CPack *request);
	IConnectionHandler *connHandler;
	//virtual bool hasAccess(int playerId) const;

public:
	CBattleCallback(CGameState *GS, int Player, IConnectionHandler *C);
///	 for casting spells by hero - DO NOT use it for moving active stack
	int battleMakeAction(BattleAction* action) OVERRIDE; 

///	 performs tactic phase actions
	bool battleMakeTacticAction(BattleAction * action) OVERRIDE; 


	friend class CCallback;
	friend class CClient;
};
