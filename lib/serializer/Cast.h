/*
 * Cast.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <typeinfo>
#include <string>
#include "CTypeList.h"

VCMI_LIB_NAMESPACE_BEGIN

template<class T, class F>
inline const T * dynamic_ptr_cast(const F * ptr)
{
#ifndef VCMI_APPLE
	return dynamic_cast<const T *>(ptr);
#else
	if(!strcmp(typeid(*ptr).name(), typeid(T).name()))
	{
		return static_cast<const T *>(ptr);
	}
	try
	{
		auto * sourceTypeInfo = typeList.getTypeInfo(ptr);
		auto * targetTypeInfo = &typeid(typename std::remove_const<typename std::remove_pointer<T>::type>::type);
		typeList.castRaw((void *)ptr, sourceTypeInfo, targetTypeInfo);
	}
	catch(...)
	{
		return nullptr;
	}
	return static_cast<const T *>(ptr);
#endif
}

template<class T, class F>
inline T * dynamic_ptr_cast(F * ptr)
{
#ifndef VCMI_APPLE
	return dynamic_cast<T *>(ptr);
#else
	if(!strcmp(typeid(*ptr).name(), typeid(T).name()))
	{
		return static_cast<T *>(ptr);
	}
	try
	{
		auto * sourceTypeInfo = typeList.getTypeInfo(ptr);
		auto * targetTypeInfo = &typeid(typename std::remove_const<typename std::remove_pointer<T>::type>::type);
		typeList.castRaw((void *)ptr, sourceTypeInfo, targetTypeInfo);
	}
	catch(...)
	{
		return nullptr;
	}
	return static_cast<T *>(ptr);
#endif
}

VCMI_LIB_NAMESPACE_END
