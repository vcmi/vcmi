#pragma once
#include "../global.h"
#include <set>
class CVCMIServer;
class CGameState;
class CConnection;
struct StartInfo;
class CCPPObjectScript;
class CScriptCallback;
template <typename T> struct CPack;

class CGameHandler
{

	CGameState *gs;
	std::set<CCPPObjectScript *> cppscripts; //C++ scripts
	//std::map<int, std::map<std::string, CObjectScript*> > objscr; //non-C++ scripts 

	CVCMIServer *s;
	std::map<int,CConnection*> connections;
	std::set<CConnection*> conns;

	void handleCPPObjS(std::map<int,CCPPObjectScript*> * mapa, CCPPObjectScript * script);

public:
	CGameHandler(void);
	~CGameHandler(void);
	void init(StartInfo *si, int Seed);
	void handleConnection(std::set<int> players, CConnection &c);
	template <typename T>void sendToAllClients(CPack<T> * info)
	{
		for(std::set<CConnection*>::iterator i=conns.begin(); i!=conns.end();i++)
		{
			(*i)->rmx->lock();
			**i << info->getType() << *info->This();
			(*i)->rmx->unlock();
		}
	}
	template <typename T>void sendAndApply(CPack<T> * info)
	{
		gs->apply(info);
		sendToAllClients(info);
	}
	void run();
	void newTurn();

	friend class CVCMIServer;
	friend class CScriptCallback;
};
