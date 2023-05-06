/*
 * ThreadPool.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "BlockingQueue.h"
#include "JobProvider.h"
#include <boost/thread/future.hpp>
#include <boost/thread/condition_variable.hpp>

VCMI_LIB_NAMESPACE_BEGIN

//Credit to https://github.com/Liam0205/toy-threadpool/tree/master/yuuki

class DLL_LINKAGE ThreadPool
{
private:
	using Lock = boost::unique_lock<boost::shared_mutex>;
	mutable boost::shared_mutex mx;
	mutable boost::condition_variable_any cv;
	mutable boost::once_flag once;

	bool isInitialized = false;
	bool stopping = false;
	bool canceling = false;
public:
	ThreadPool();
	~ThreadPool();

	void init(size_t numThreads);
	void spawn();
	void terminate();
	void cancel();

public:
	bool initialized() const;
	bool running() const;
	int size() const;
private:
	bool isRunning() const;

public:
	auto async(std::function<void()>&& f) const -> boost::future<void>;

private:
	std::vector<boost::thread> workers;
	mutable BlockingQueue<TRMGfunction> tasks;
};

ThreadPool::ThreadPool() :
	once(BOOST_ONCE_INIT)
{};

ThreadPool::~ThreadPool()
{
	terminate();
}

inline void ThreadPool::init(size_t numThreads)
{
	boost::call_once(once, [this, numThreads]()
	{
		Lock lock(mx);
		stopping = false;
		canceling = false;
		workers.reserve(numThreads);
		for (size_t i = 0; i < numThreads; ++i)
		{
			workers.emplace_back(std::bind(&ThreadPool::spawn, this));
		}
		isInitialized = true;
	});
}

bool ThreadPool::isRunning() const
{
	return isInitialized && !stopping && !canceling;
}

inline bool ThreadPool::initialized() const
{
	Lock lock(mx);
	return isInitialized;
}

inline bool ThreadPool::running() const
{
	Lock lock(mx);
	return isRunning();
}

inline int ThreadPool::size() const
{
	Lock lock(mx);
	return workers.size();
}

inline void ThreadPool::spawn()
{
	while(true)
	{
		bool pop = false;
		TRMGfunction task;
		{
			Lock lock(mx);
			cv.wait(lock, [this, &pop, &task]
			{
				pop = tasks.pop(task);
				return canceling || stopping || pop;
			});
		}
		if (canceling || (stopping && !pop))
		{
			return;
		}
		task();
	}
}

inline void ThreadPool::terminate()
{
	{
		Lock lock(mx);
		if (isRunning())
		{
			stopping = true;
		}
		else
		{
			return;
		}
	}
	cv.notify_all();
	for (auto& worker : workers)
	{
		worker.join();
	}
}

inline void ThreadPool::cancel()
{
	{
		Lock lock(mx);
		if (running())
		{
			canceling = true;
		}
		else
		{
			return;
		}
	}
	tasks.clear();
	cv.notify_all();
	for (auto& worker : workers)
	{
		worker.join();
	}
}

auto ThreadPool::async(std::function<void()>&& f) const -> boost::future<void>
{
    using TaskT = boost::packaged_task<void>;

    {
        Lock lock(mx);
        if (stopping || canceling)
        {
            throw std::runtime_error("Delegating task to a threadpool that has been terminated or canceled.");
        }
    }

    std::shared_ptr<TaskT> task = std::make_shared<TaskT>(f);
    boost::future<void> fut = task->get_future();
    tasks.emplace([task]() -> void
    {
        (*task)();
    });
    cv.notify_one();
    return fut;
}

VCMI_LIB_NAMESPACE_END