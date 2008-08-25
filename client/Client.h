#pragma once
#include "../global.h"
#include <boost/thread.hpp>
struct StartInfo;
class CGameState;
class CGameInterface;
class CConnection;
class CCallback;

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
	void process(int what);
	void run();

	friend class CCallback;
	friend class CScriptCallback;
};
