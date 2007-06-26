#ifndef TIMEHANDLER_H
#define TIMEHANDLER_H

#include <ctime>
class timeHandler
{
public:
	clock_t start, last, mem;
	timeHandler():start(clock()){last=0;mem=0;};
	long getDif(){long ret=clock()-last;last=clock();return ret;};
	void update(){last=clock();};
	void remember(){mem=clock();};
	long memDif(){return mem-clock();};
};

#endif //TIMEHANDLER_H