#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

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

struct SharedMem 
{
	boost::interprocess::shared_memory_object smo;
	boost::interprocess::mapped_region *mr;
	ServerReady *sr;
	
	SharedMem()
		:smo(boost::interprocess::open_or_create,"vcmi_memory",boost::interprocess::read_write) 
	{
		smo.truncate(sizeof(ServerReady));
		mr = new boost::interprocess::mapped_region(smo,boost::interprocess::read_write);
		sr = new(mr->get_address())ServerReady();
	};
	~SharedMem()
	{
		delete mr;
		boost::interprocess::shared_memory_object::remove("vcmi_memory");
	}
};