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

#include <boost/thread/future.hpp>
#include <boost/thread/condition_variable.hpp>

VCMI_LIB_NAMESPACE_BEGIN

//Credit to https://github.com/Liam0205/toy-threadpool/tree/master/yuuki

template <typename T>
class DLL_LINKAGE BlockingQueue : boost::noncopyable
{
public:
	template <typename... Args>
	void emplace(Args&&... args)
	{
		boost::mutex::scoped_lock lock(mx);
		queue.emplace(std::forward<Args>(args)...);
	}

	bool pop(T& holder)
	{
		boost::mutex::scoped_lock lock(mx);
		if (queue.empty())
			return false;

		holder = std::move(queue.front());
		queue.pop();
		return true;
	}

private:
	std::queue<T> queue;
	mutable boost::mutex mx;
};

class DLL_LINKAGE ThreadPool
{
private:
	using Functor = std::function<void()>;
	mutable boost::mutex mx;
	mutable boost::condition_variable_any cv;

	std::atomic<bool> stopping = false;

	std::vector<boost::thread> workers;
	mutable BlockingQueue<Functor> tasks;

	void spawn();
	void terminate();

public:
	explicit ThreadPool(size_t maxThreadsCount);
	~ThreadPool();

	boost::future<void> async(std::function<void()>&& f) const;
};

inline ThreadPool::ThreadPool(size_t desiredThreadsCount)
{
	size_t actualThreadCount = std::min<int>(boost::thread::hardware_concurrency(), desiredThreadsCount);

	workers.reserve(actualThreadCount);
	for (size_t i = 0; i < actualThreadCount; ++i)
		workers.emplace_back(std::bind(&ThreadPool::spawn, this));
}

inline ThreadPool::~ThreadPool()
{
	terminate();
}

inline void ThreadPool::spawn()
{
	while(true)
	{
		bool pop = false;
		Functor task;
		{
			boost::mutex::scoped_lock lock(mx);
			cv.wait(lock, [this, &pop, &task]
			{
				pop = tasks.pop(task);
				return stopping || pop;
			});
		}
		if (stopping && !pop)
		{
			return;
		}
		task();
	}
}

inline void ThreadPool::terminate()
{
	if (stopping.exchange(true))
		return;

	cv.notify_all();

	for (auto& worker : workers)
		worker.join();
}

inline boost::future<void> ThreadPool::async(std::function<void()>&& f) const
{
	using TaskT = boost::packaged_task<void>;

	if (stopping)
		throw std::runtime_error("Delegating task to a threadpool that has been terminated.");

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
