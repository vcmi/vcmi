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
class BlockingQueue : boost::noncopyable
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

class TaskGroup
{
	friend class ThreadPool;

	std::vector<boost::future<void>> tasks;
	std::atomic<int> tasksLeft = 0;
};

class ThreadPool
{
	friend class TaskGroup;
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
	static ThreadPool & global();

	explicit ThreadPool();
	~ThreadPool();

	void addTask(TaskGroup & group, std::function<void()>&& f);
	void waitFor(TaskGroup & group);
};

inline ThreadPool & ThreadPool::global()
{
	static ThreadPool globalInstance;
	return globalInstance;
}

inline ThreadPool::ThreadPool()
{
	size_t actualThreadCount = boost::thread::hardware_concurrency();

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

inline void ThreadPool::addTask(TaskGroup & group, std::function<void()>&& f)
{
	using TaskT = boost::packaged_task<void>;

	if (stopping)
		throw std::runtime_error("Delegating task to a threadpool that has been terminated.");

	std::shared_ptr<TaskT> task = std::make_shared<TaskT>(f);
	group.tasks.emplace_back(task->get_future());

	++group.tasksLeft;
	tasks.emplace([&group, task]() -> void
	{
		(*task)();
		--group.tasksLeft;
	});
	cv.notify_one();
}

inline void ThreadPool::waitFor(TaskGroup & group)
{
	while (group.tasksLeft.load() != 0)
	{
		Functor task;
		bool pop = tasks.pop(task);

		if (pop)
		{
			// task acquired - execute it on our thread
			task();
		}
		else
		{
			// failed to acquire task.
			// This might happen if another thread is currently executing task from our group - in this case sleep for a bit and try again later
			boost::this_thread::sleep_for(boost::chrono::milliseconds(1));
		}
	}
}

VCMI_LIB_NAMESPACE_END
