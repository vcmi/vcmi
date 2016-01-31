#pragma once

#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/interprocess/shared_memory_object.hpp>

/*
 * Interprocess.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

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
		{
			boost::unique_lock<boost::interprocess::interprocess_mutex> lock(mutex); 
			ready = true;
		}
		cond.notify_all();
	}
};

struct SharedMem 
{
	boost::interprocess::shared_memory_object smo;
	boost::interprocess::mapped_region *mr;
	ServerReady *sr;
	
	SharedMem() //c-tor
		:smo(boost::interprocess::open_or_create,"vcmi_memory",boost::interprocess::read_write) 
	{
		smo.truncate(sizeof(ServerReady));
		mr = new boost::interprocess::mapped_region(smo,boost::interprocess::read_write);
		sr = new(mr->get_address())ServerReady();
	};
	~SharedMem() //d-tor
	{
		delete mr;
		boost::interprocess::shared_memory_object::remove("vcmi_memory");
	}
};
