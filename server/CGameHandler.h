#pragma once
#include "../global.h"
#include <set>
class CVCMIServer;
class CGameState;
class CConnection;
struct StartInfo;
class CGameHandler
{
	CGameState *gs;
	CVCMIServer *s;
	std::map<int,CConnection*> connections;
	std::set<CConnection*> conns;
public:
	CGameHandler(void);
	~CGameHandler(void);
	void init(StartInfo *si, int Seed);
	void handleConnection(std::set<int> players, CConnection &c);
	void run();
	void newTurn();

	friend class CVCMIServer;
};
