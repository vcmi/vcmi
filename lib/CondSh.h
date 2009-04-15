#ifndef __CONDSH_H__
#define __CONDSH_H__
#include <boost/thread.hpp>

/*
 * CondSh.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

template <typename T> struct CondSh
{
	T data;
	boost::condition_variable cond;
	boost::mutex mx;
	CondSh(){};
	CondSh(T t){data = t;};
	void set(T t){mx.lock();data=t;mx.unlock();}; //set data
	void setn(T t){mx.lock();data=t;mx.unlock();cond.notify_all();}; //set data and notify
	T get(){boost::unique_lock<boost::mutex> lock(mx); return data;};
};
#endif // __CONDSH_H__
