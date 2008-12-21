#ifndef CLIENT_H
#define CLIENT_H

#ifdef _MSC_VER
#pragma once
#endif

#include "../global.h"
#include <boost/thread.hpp>
struct StartInfo;
class CGameState;
class CGameInterface;
class CConnection;
class CCallback;
class CClient;
void processCommand(const std::string &message, CClient *&client);
namespace boost
{
	class mutex;
	class condition_variable;
}

template <typename T>
struct CSharedCond
{
	boost::mutex *mx;
	boost::condition_variable *cv;
	T *res;
	CSharedCond(T*R)
	{
		res = R;
		mx = new boost::mutex;
		cv = new boost::condition_variable;
	}
	~CSharedCond()
	{
		delete res;
		delete mx;
		delete cv;
	}
};

class CClient
{
	CCallback *cb;
	CGameState *gs;
	std::map<ui8,CGameInterface *> playerint;
	CConnection *serv;

	void waitForMoveAndSend(int color);
public:
	CClient(void);
	CClient(CConnection *con, StartInfo *si);
	~CClient(void);

	void close();
	void save(const std::string & fname);
	void process(int what);
	void run();

	friend class CCallback; //handling players actions
	friend class CScriptCallback; //for objects scripts
	friend void processCommand(const std::string &message, CClient *&client); //handling console
};
#endif //CLIENT_H