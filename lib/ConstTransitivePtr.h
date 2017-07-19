/*
 * ConstTransitivePtr.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

class CGameHandler;

template<typename T>
class ConstTransitivePtr
{
	T * ptr;
	ConstTransitivePtr(const T * Ptr)
		: ptr(const_cast<T *>(Ptr))
	{}

public:
	ConstTransitivePtr(T * Ptr = nullptr)
		: ptr(Ptr)
	{}
	ConstTransitivePtr(std::nullptr_t)
		: ptr(nullptr)
	{}


	const T & operator*() const
	{
		return *ptr;
	}
	T & operator*()
	{
		return *ptr;
	}
	operator const T *() const
	{
		return ptr;
	}
	T * get()
	{
		return ptr;
	}
	const T * get() const
	{
		return ptr;
	}
	operator T *()
	{
		return ptr;
	}
	T * operator->()
	{
		return ptr;
	}
	const T * operator->() const
	{
		return ptr;
	}
	const T * operator=(T * t)
	{
		return ptr = t;
	}

	void dellNull()
	{
		delete ptr;
		ptr = nullptr;
	}

	template<typename Handler> void serialize(Handler & h, const int version)
	{
		h & ptr;
	}

	friend class CGameHandler;
};
