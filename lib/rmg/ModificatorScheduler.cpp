/*
 * ModificatorScheduler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "ModificatorScheduler.h"
#include "modificators/Modificator.h"

#include <atomic>
#include <exception>
#include <mutex>
#include <tbb/task_group.h>

namespace rmg
{

void runModificators(TModificators jobs, bool singleThread, const std::function<void(Modificator &)> & onFinished)
{
	if(singleThread)
	{
		while(!jobs.empty())
		{
			for(auto it = jobs.begin(); it != jobs.end();)
			{
				if((*it)->isReady())
				{
					auto jobCopy = *it;
					jobCopy->run();
					onFinished(*jobCopy);
					jobs.erase(it);
					break;
				}
				else
				{
					++it;
				}
			}
		}
	}
	else
	{
		tbb::task_group pool;
		std::mutex errorMutex;
		std::exception_ptr firstError;
		std::atomic<bool> failed{false};

		while(!jobs.empty() && !failed)
		{
			for(auto it = jobs.begin(); it != jobs.end();)
			{
				if((*it)->isReady())
				{
					auto jobCopy = *it;
					pool.run([jobCopy, &onFinished, &errorMutex, &firstError, &failed]() -> void
						{
							try
							{
								jobCopy->run();
								onFinished(*jobCopy);
							}
							catch(...)
							{
								failed = true;
								std::lock_guard<std::mutex> lock(errorMutex);
								if(!firstError)
									firstError = std::current_exception();
							}
						}
					);
					it = jobs.erase(it);
				}
				else
				{
					++it;
				}
			}
		}

		pool.wait();
		if(firstError)
			std::rethrow_exception(firstError);
	}
}

}
