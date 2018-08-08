/*
 * StdInc.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../Global.h"
#include "../lib/serializer/CTypeList.h"

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
    try {
        auto* sourceTypeInfo = typeList.getTypeInfo(ptr);
        auto* targetTypeInfo = &typeid(typename std::remove_const<typename std::remove_pointer<T>::type>::type);
        typeList.castRaw((void *)ptr, sourceTypeInfo, targetTypeInfo);
    } catch (...) {
        return nullptr;
    }
    return static_cast<const T*>(ptr);
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
    try {
        auto* sourceTypeInfo = typeList.getTypeInfo(ptr);
        auto* targetTypeInfo = &typeid(typename std::remove_const<typename std::remove_pointer<T>::type>::type);
        typeList.castRaw((void *)ptr, sourceTypeInfo, targetTypeInfo);
    } catch (...) {
        return nullptr;
    }
    return static_cast<T*>(ptr);
	#endif
}
