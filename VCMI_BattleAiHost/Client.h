#pragma once
#include "../global.h"
class CGameState;
class CConnection;
struct CPack;
class CBattleGameInterface;
struct BattleAction;
class CStack;

class CClient/* : public IGameCallback*/
{
public:
	bool terminate;
	BattleAction *curbaction;
	CGameState *gs;
	CConnection *serv;
	CBattleGameInterface *ai;

	CClient();

	void run();
	void handlePack( CPack * pack ); //applies the given pack and deletes it
	void requestMoveFromAI(const CStack *s);
	void requestMoveFromAIWorker(const CStack *s);
};