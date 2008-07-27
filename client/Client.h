#pragma once
#include "../global.h"
struct StartInfo;
class CGameState;
class CGameInterface;
class CConnection;
class CCallback;
class CClient
{
	CGameState *gs;
	std::map<int,CGameInterface *> playerint;
	CConnection *serv;
public:
	CClient(void);
	CClient(CConnection *con, StartInfo *si);
	~CClient(void);

	void process(int what);
	void run();

	friend class CCallback;
};
