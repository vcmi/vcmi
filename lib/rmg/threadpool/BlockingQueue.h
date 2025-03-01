/*
 * BlockingQueue.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "StdInc.h"

VCMI_LIB_NAMESPACE_BEGIN

//Credit to https://github.com/Liam0205/toy-threadpool/tree/master/yuuki

template <typename T>
class DLL_LINKAGE BlockingQueue : protected std::queue<T>
{
	using WriteLock = std::unique_lock<std::shared_mutex>;
	using Readlock = std::shared_lock<std::shared_mutex>;

public:
	BlockingQueue() = default;
	~BlockingQueue()
	{
		clear();
  	}
	BlockingQueue(const BlockingQueue&) = delete;
	BlockingQueue(BlockingQueue&&) = delete;
	BlockingQueue& operator=(const BlockingQueue&) = delete;
	BlockingQueue& operator=(BlockingQueue&&) = delete;

public:
	bool empty() const
	{
		Readlock lock(mx);
		return std::queue<T>::empty();
	}

	size_t size() const
	{
		Readlock lock(mx);
		return std::queue<T>::size();
	}

public:
	void clear()
	{
		WriteLock lock(mx);
		while (!std::queue<T>::empty())
		{
			std::queue<T>::pop();
		}
	}

	void push(const T& obj)
	{
		WriteLock lock(mx);
		std::queue<T>::push(obj);
	}

	template <typename... Args>
	void emplace(Args&&... args)
	{
		WriteLock lock(mx);
		std::queue<T>::emplace(std::forward<Args>(args)...);
	}

	bool pop(T& holder)
	{
		WriteLock lock(mx);
		if (std::queue<T>::empty())
		{
			return false;
		}
		else
		{
			holder = std::move(std::queue<T>::front());
			std::queue<T>::pop();
			return true;
		}
	}

private:
	mutable std::shared_mutex mx;
};

VCMI_LIB_NAMESPACE_END