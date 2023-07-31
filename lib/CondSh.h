/*
 * CondSh.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

/// Used for multithreading, wraps boost functions
template <typename T> struct CondSh
{
	T data;
	std::condition_variable cond;
	std::mutex mx;

	CondSh() : data(T()) {}

	CondSh(T t) : data(t) {}

	// set data
	void set(T t)
	{
		std::unique_lock<std::mutex> lock(mx);
		data = t;
	}

	// set data and notify
	void setn(T t)
	{
		set(t);
		cond.notify_all();
	};

	// get stored value
	T get()
	{
		std::unique_lock<std::mutex> lock(mx);
		return data;
	}

	// waits until data is set to false
	void waitWhileTrue()
	{
		std::unique_lock<std::mutex> un(mx);
		while(data)
			cond.wait(un);
	}

	// waits while data is set to arg
	void waitWhile(const T & t)
	{
		std::unique_lock<std::mutex> un(mx);
		while(data == t)
			cond.wait(un);
	}

	// waits until data is set to arg
	void waitUntil(const T & t)
	{
		std::unique_lock<std::mutex> un(mx);
		while(data != t)
			cond.wait(un);
	}
};

VCMI_LIB_NAMESPACE_END
