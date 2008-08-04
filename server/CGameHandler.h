#pragma once
#include "../global.h"
#include <set>
#include "../CGameState.h"
#include "../lib/Connection.h"
#ifndef _MSC_VER
#include <boost/thread.hpp>
#endif
class CVCMIServer;
class CGameState;
//class CConnection;
struct StartInfo;
class CCPPObjectScript;
class CScriptCallback;
template <typename T> struct CPack;
class CGHeroInstance;
class CGameHandler
{

	CGameState *gs;
	std::set<CCPPObjectScript *> cppscripts; //C++ scripts
	//std::map<int, std::map<std::string, CObjectScript*> > objscr; //non-C++ scripts 

	CVCMIServer *s;
	std::map<int,CConnection*> connections; //player color -> connection to clinet with interface of that player
	std::map<int,int> states; //player color -> player state
	std::set<CConnection*> conns;

	void handleCPPObjS(std::map<int,CCPPObjectScript*> * mapa, CCPPObjectScript * script);
	void changePrimSkill(int ID, int which, int val, bool abs=false);
	void startBattle(CCreatureSet army1, CCreatureSet army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2); //use hero=NULL for no hero


public:
	CGameHandler(void);
	~CGameHandler(void);
	void init(StartInfo *si, int Seed);
	void handleConnection(std::set<int> players, CConnection &c);
	template <typename T>void sendToAllClients(CPack<T> * info)
	{
		for(std::set<CConnection*>::iterator i=conns.begin(); i!=conns.end();i++)
		{
			(*i)->wmx->lock();
			**i << info->getType() << *info->This();
			(*i)->wmx->unlock();
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
