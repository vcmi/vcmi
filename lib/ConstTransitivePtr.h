#pragma once

template <typename T>
class ConstTransitivePtr
{
	T *ptr;
public:
	ConstTransitivePtr(T *Ptr = NULL)
		: ptr(Ptr) 
	{}

	operator const T*() const
	{
		return ptr;
	}
	operator T*() 
	{
		return ptr;
	}
	T *operator->() 
	{
		return ptr;
	}
	const T *operator->() const 
	{
		return ptr;
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ptr;
	}
};