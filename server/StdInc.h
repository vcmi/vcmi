#pragma once

#include "../Global.h"

#include <boost/crc.hpp>
#include <boost/date_time/posix_time/posix_time_types.hpp> //no i/o just types
#include <boost/random/linear_congruential.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/variate_generator.hpp>
#include <boost/system/system_error.hpp>

template<class T, class F>
inline const T * dynamic_ptr_cast(const F * ptr)
{
	#ifndef __APPLE__
  return dynamic_cast<const T*>(ptr);
	#else
	if (!strcmp(typeid(*ptr).name(), typeid(T).name()))
	{
		return static_cast<const T*>(ptr);
	}
	return nullptr;
	#endif
}

template<class T, class F>
inline T * dynamic_ptr_cast(F * ptr)
{
	#ifndef __APPLE__
  return dynamic_cast<T*>(ptr);
	#else
	if (!strcmp(typeid(*ptr).name(), typeid(T).name()))
	{
		return static_cast<T*>(ptr);
	}
	return nullptr;
	#endif
}
