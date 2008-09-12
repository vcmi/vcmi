#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>

struct ServerReady
{
	bool ready;
	boost::interprocess::interprocess_mutex  mutex;
	boost::interprocess::interprocess_condition  cond;

	ServerReady()
	{
		ready = false;
	}

	void setToTrueAndNotify()
	{
		mutex.lock();
		ready = true;
		mutex.unlock();
		cond.notify_all();
	}
};
