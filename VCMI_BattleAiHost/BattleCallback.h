#pragma once

#include "../global.h"
#include "../CCallback.h"

/*class CBattleCallback : public IBattleCallback, public CBattleInfoCallback
{
private:
	CBattleCallback(CGameState *GS, int Player, CClient *C);


protected:
	void sendRequest(const CPack *request);
	CClient *cl;
	//virtual bool hasAccess(int playerId) const;

public:
	int battleMakeAction(BattleAction* action) OVERRIDE;//for casting spells by hero - DO NOT use it for moving active stack
	bool battleMakeTacticAction(BattleAction * action) OVERRIDE; // performs tactic phase actions

	friend class CCallback;
	friend class CClient;
};*/