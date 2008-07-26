#pragma once
#include "../global.h"
#include <set>
class CVCMIServer;
class CGameState;
class CConnection;
struct StartInfo;
template <typename T> struct CPack;

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
	template <typename T>void sendToAllClients(CPack<T> * info);
	void run();
	void newTurn();

	friend class CVCMIServer;
};
