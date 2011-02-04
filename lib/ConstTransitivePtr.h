#pragma once

class CGameHandler;

template <typename T>
class ConstTransitivePtr
{
	T *ptr;
	ConstTransitivePtr(const T *Ptr)
		: ptr(const_cast<T*>(Ptr)) 
	{}
public:
	ConstTransitivePtr(T *Ptr = NULL)
		: ptr(Ptr) 
	{}

	const T& operator*() const
	{
		return *ptr;
	}
	T& operator*()
	{
		return *ptr;
	}
	operator const T*() const
	{
		return ptr;
	}
	T* get()
	{
		return ptr;
	}
	const T* get() const
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
	const T*operator=(T *t)
	{
		return ptr = t;
	}

	void dellNull()
	{
		delete ptr;
		ptr = NULL;
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & ptr;
	}

	friend CGameHandler;
};
