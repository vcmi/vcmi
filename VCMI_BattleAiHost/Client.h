#pragma once
#include "../global.h"
#include "../lib/CBattleCallback.h"
class CGameState;
class CConnection;
struct CPack;
class CBattleGameInterface;
struct BattleAction;
class CStack;

class CClient/* : public IGameCallback*/ : public IConnectionHandler
{
public:
	bool terminate;
	BattleAction *curbaction;
	CGameState *gs;
	CBattleGameInterface *ai;
	ui8 color;

	CClient();

	void run();
	void handlePack( CPack * pack ); //applies the given pack and deletes it
	void requestMoveFromAI(const CStack *s);
	void requestMoveFromAIWorker(const CStack *s);
};