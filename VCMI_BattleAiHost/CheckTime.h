#pragma once

#include "../global.h"


#ifdef _WIN32
	#include <ctime>
	typedef time_t TTime;
	#define GET_TIME(var) (var = clock())
#else
	#include <sys/time.h>                // for gettimeofday()
	typedef timeval TTime;
	#define GET_TIME(var) (gettimeofday(&var, NULL))
#endif


struct CheckTime
{
	TTime start;

	std::string msg;

	CheckTime(const std::string & Msg = "") : msg(Msg)
	{
		GET_TIME(start);
	}

	int timeDiff(const TTime &t1, const TTime &t2)
	{
		int ret = 0;
#ifdef _WIN32
		ret = (float)(t2 - t1) / (CLOCKS_PER_SEC / 1000);
#else
		ret += (t2.tv_sec - t1.tv_sec) * 1000.0;   // sec to ms
		ret += (t2.tv_usec - t1.tv_usec) / 1000.0; // us to ms
#endif

		//TODO abs?
		return ret;
	}

	int timeSinceStart()
	{
		TTime now;
		GET_TIME(now);
		return timeDiff(start, now);
	}

	~CheckTime()
	{
		if(msg.size())
		{
			float liczyloSie = timeSinceStart();
			tlog0 << msg << ": " << liczyloSie << "ms" << std::endl;
		}
	}
};

//all ms
const int PROCESS_INFO_TIME = 5; 
const int MAKE_DECIDION_TIME = 150; 
const int MEASURE_MARGIN = 3;
const int HANGUP_TIME = 250;
const int CONSTRUCT_TIME = 50;
const int STARTUP_TIME = 100;

void postInfoCall(int timeUsed);
void postDecisionCall(int timeUsed, const std::string &text = "AI was thinking over an action");

struct Bomb
{
	std::string txt;
	int armed;

	void run(int time)
	{
		boost::this_thread::sleep(boost::posix_time::milliseconds(time));
		if(armed)
		{
			tlog1 << "BOOOM! The bomb exploded! AI was thinking for too long!\n";
			if(txt.size())
				tlog1 << "Bomb description: " << txt << std::endl;;
			exit(1);
		}

		delete this;
	}

	Bomb(int timer, const std::string &TXT = "") : txt(TXT)
	{
		boost::thread t(&Bomb::run, this, timer);
		t.detach();
	}

	void disarm()
	{
		armed = 0;
	}
};