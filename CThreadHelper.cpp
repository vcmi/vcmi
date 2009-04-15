#include "CThreadHelper.h"
#include <boost/thread.hpp>
#include <boost/bind.hpp>

/*
 * CThreadHelper.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

CThreadHelper::CThreadHelper(std::vector<boost::function<void()> > *Tasks, int Threads)
{
	currentTask = 0; amount = Tasks->size();
	tasks = Tasks;
	threads = Threads;
}
void CThreadHelper::run()
{
	boost::thread_group grupa;
	for(int i=0;i<threads;i++)
		grupa.create_thread(boost::bind(&CThreadHelper::processTasks,this));
	grupa.join_all();
}
void CThreadHelper::processTasks()
{
	int pom;
	while(true)
	{
		rtinm.lock();
		if((pom=currentTask) >= amount)
		{
			rtinm.unlock();
			break;
		}
		else
		{
			++currentTask;
			rtinm.unlock();
			(*tasks)[pom]();
		}
	}
}