#ifndef __TIMEHANDLER_H__
#define __TIMEHANDLER_H__

#include <ctime>

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
public:
	clock_t start, last, mem;
	timeHandler():start(clock()){last=clock();mem=0;};
	long getDif(){long ret=clock()-last;last=clock();return ret;};
	void update(){last=clock();};
	void remember(){mem=clock();};
	long memDif(){return clock()-mem;};
};


#endif // __TIMEHANDLER_H__
