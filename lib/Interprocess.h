/*
 * Interprocess.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

VCMI_LIB_NAMESPACE_BEGIN

struct ServerReady
{
	bool ready;
	uint16_t port; //ui16?
	boost::interprocess::interprocess_mutex mutex;
	boost::interprocess::interprocess_condition cond;

	ServerReady()
	{
		ready = false;
		port = 0;
	}

	void waitTillReady()
	{
		boost::interprocess::scoped_lock<boost::interprocess::interprocess_mutex> slock(mutex);
		while(!ready)
		{
			cond.wait(slock);
		}
	}

	void setToReadyAndNotify(const uint16_t Port)
	{
		{
			boost::unique_lock<boost::interprocess::interprocess_mutex> lock(mutex);
			ready = true;
			port = Port;
		}
		cond.notify_all();
	}
};

struct SharedMemory
{
	std::string name;
	boost::interprocess::shared_memory_object smo;
	boost::interprocess::mapped_region * mr;
	ServerReady * sr;

	SharedMemory(const std::string & Name, bool initialize = false)
		: name(Name)
	{
		if(initialize)
		{
			//if the application has previously crashed, the memory may not have been removed. to avoid problems - try to destroy it
			boost::interprocess::shared_memory_object::remove(name.c_str());
		}
		smo = boost::interprocess::shared_memory_object(boost::interprocess::open_or_create, name.c_str(), boost::interprocess::read_write);
		smo.truncate(sizeof(ServerReady));
		mr = new boost::interprocess::mapped_region(smo,boost::interprocess::read_write);
		if(initialize)
			sr = new(mr->get_address())ServerReady();
		else
			sr = reinterpret_cast<ServerReady*>(mr->get_address());
	};

	~SharedMemory()
	{
		delete mr;
		boost::interprocess::shared_memory_object::remove(name.c_str());
	}
};

VCMI_LIB_NAMESPACE_END
