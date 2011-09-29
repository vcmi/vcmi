#pragma once

#include "../global.h"
#include <ctime>
#ifndef _WIN32
#include <sys/time.h>                // for gettimeofday()
#endif


struct CheckTime
{
#ifdef _WIN32
	time_t t0;
#else
	timeval t1, t2;
#endif
	std::string msg;

	CheckTime(const std::string & Msg) : msg(Msg)
#ifdef _WIN32
		, t0(clock())
#endif
	{

#ifndef _WIN32
		gettimeofday(&t1, NULL);
#endif
	}
	~CheckTime()
	{
		float liczyloSie = 0;

#ifdef _WIN32
		liczyloSie = (float)(clock() - t0) / (CLOCKS_PER_SEC / 1000);
#else
		// stop timer
		gettimeofday(&t2, NULL);
		liczyloSie = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
		liczyloSie += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
#endif
		tlog0 << msg << ": " << liczyloSie << "ms" << std::endl;;
	}
};
