#ifndef __CONDSH_H__
#define __CONDSH_H__
#include <boost/thread.hpp>
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
};#endif // __CONDSH_H__
