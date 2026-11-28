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

namespace
{

using OnFinished = std::function<void(const Modificator &)>;

/// Keeps the first exception thrown by any job. Safe to use from several threads.
class FirstError
{
public:
	void record(std::exception_ptr error)
	{
		std::scoped_lock lock(mx);
		if(!firstError)
			firstError = error;
		failed = true;
	}

	bool isSet() const
	{
		return failed;
	}

	void rethrowIfSet() const
	{
		std::scoped_lock lock(mx);
		if(firstError)
			std::rethrow_exception(firstError);
	}

private:
	mutable std::mutex mx;
	std::exception_ptr firstError;
	std::atomic<bool> failed = false;
};

/// Removes the first ready job from the queue and returns it, or returns nullptr when no job is ready.
std::shared_ptr<Modificator> takeFirstReady(TModificators & jobs)
{
	auto it = std::find_if(jobs.begin(), jobs.end(), [](const auto & job) { return job->isReady(); });
	if(it == jobs.end())
		return nullptr;

	auto job = *it;
	jobs.erase(it);
	return job;
}

/// Runs jobs in a fixed order: always the first ready job in the queue.
void runSingleThreaded(TModificators & jobs, const OnFinished & onFinished)
{
	while(!jobs.empty())
	{
		auto job = takeFirstReady(jobs);
		if(!job)
			continue;

		job->run();
		onFinished(*job);
	}
}

void runJob(Modificator & job, FirstError & error, const OnFinished & onFinished)
{
	try
	{
		job.run();
		onFinished(job);
	}
	catch(const std::exception &)
	{
		error.record(std::current_exception());
	}
}

/// Starts every job that is ready now and removes it from the queue.
void scheduleReadyJobs(TModificators & jobs, tbb::task_group & pool, FirstError & error, const OnFinished & onFinished)
{
	for(auto it = jobs.begin(); it != jobs.end();)
	{
		if(!(*it)->isReady())
		{
			++it;
			continue;
		}

		auto job = *it;
		pool.run([job, &error, &onFinished]() { runJob(*job, error, onFinished); });
		it = jobs.erase(it);
	}
}

void runMultiThreaded(TModificators & jobs, const OnFinished & onFinished)
{
	tbb::task_group pool;
	FirstError error;

	while(!jobs.empty() && !error.isSet())
		scheduleReadyJobs(jobs, pool, error, onFinished);

	// Jobs already started finish before the failure leaves this function
	pool.wait();
	error.rethrowIfSet();
}

}

void runModificators(TModificators jobs, bool singleThread, const OnFinished & onFinished)
{
	if(singleThread)
		runSingleThreaded(jobs, onFinished);
	else
		runMultiThreaded(jobs, onFinished);
}

}
