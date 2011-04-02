#ifndef __TIMEHANDLER_H__
#define __TIMEHANDLER_H__

#ifdef __FreeBSD__
	#include <sys/types.h>
	#include <sys/time.h>
	#include <sys/resource.h>
	#define TO_MS_DIVISOR (1000)
#else
	#include <ctime>
	#define TO_MS_DIVISOR (CLOCKS_PER_SEC/1000)
#endif

/*
 * timeHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class timeHandler
{
	clock_t start, last, mem;
public:
	timeHandler()
		: start(clock())
	{
		last=clock();
		mem=0;
	}

	long getDif() //get diff in milliseconds
	{
		long ret=clock()-last;
		last=clock();
		return ret/TO_MS_DIVISOR;
	}
	void update()
	{
		last=clock();
	}
	void remember()
	{
		mem=clock();
	}
	long memDif()
	{
		return clock()-mem;
	}
	long clock() 
	{
	#ifdef __FreeBSD__
		struct rusage usage;
		getrusage(RUSAGE_SELF, &usage);
		return static_cast<long>(usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) * 1000000 + usage.ru_utime.tv_usec + usage.ru_stime.tv_usec;
	#else
		return std::clock();
	#endif
	}
};


#endif // __TIMEHANDLER_H__
