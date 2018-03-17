/*
 * LuaCallWrapper.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "api/Registry.h"
#include "LuaStack.h"
#include "LuaFunctor.h"

namespace scripting
{

namespace detail
{

}

//TODO: this should be the only one wrapper type
//
template <typename U, typename M, M m>
class LuaMethodWrapper
{

};

template <typename U, typename T, typename R, R(T:: * method)()const>
class LuaMethodWrapper <U, R(T:: *)()const, method>
{
public:
	static int invoke(lua_State * L)
	{
		LuaStack S(L);
		const U * obj = nullptr;

		if(!S.tryGet(1,obj))
			return S.retVoid();

		static auto functor = std::mem_fn(method);

		S.clear();
		S.push(functor(obj));
		return S.retPushed();
	}
};

template <typename U, typename T, void(T:: * method)()const>
class LuaMethodWrapper <U, void(T:: *)()const, method>
{
public:
	static int invoke(lua_State * L)
	{
		LuaStack S(L);
		const U * obj = nullptr;

		if(!S.tryGet(1,obj))
			return S.retVoid();

		static auto functor = std::mem_fn(method);
		S.clear();
		functor(obj);
		return 0;
	}
};

template <typename U, typename T, typename R, typename P1, R(T:: * method)(P1)const>
class LuaMethodWrapper <U, R(T:: *)(P1)const, method>
{
public:
	static int invoke(lua_State * L)
	{
		LuaStack S(L);
		const U * obj = nullptr;

		if(!S.tryGet(1,obj))
			return S.retVoid();

		P1 p1;
		if(!S.tryGet(2, p1))
			return S.retVoid();

		static auto functor = std::mem_fn(method);
		S.clear();
		S.push(functor(obj, p1));
		return S.retPushed();
	}
};

template <typename U, typename T, typename P1, void(T:: * method)(P1)const>
class LuaMethodWrapper <U, void(T:: *)(P1)const, method>
{
public:
	static int invoke(lua_State * L)
	{
		LuaStack S(L);
		const U * obj = nullptr;

		if(!S.tryGet(1,obj))
			return S.retVoid();

		P1 p1;
		if(!S.tryGet(2, p1))
			return S.retVoid();

		static auto functor = std::mem_fn(method);
		S.clear();
		functor(obj, p1);
		return 0;
	}
};


template <typename U, typename T, typename R, typename P1, typename P2, R(T:: * method)(P1, P2)const>
class LuaMethodWrapper <U, R(T:: *)(P1, P2)const, method>
{
public:
	static int invoke(lua_State * L)
	{
		LuaStack S(L);
		const U * obj = nullptr;

		if(!S.tryGet(1, obj))
			return S.retVoid();

		P1 p1;
		if(!S.tryGet(2, p1))
			return S.retVoid();

		P2 p2;
		if(!S.tryGet(3, p2))
			return S.retVoid();

		static auto functor = std::mem_fn(method);
		S.clear();
		S.push(functor(obj, p1, p2));
		return S.retPushed();
	}
};

template <typename U, typename T, typename P1, typename P2, void(T:: * method)(P1, P2)const>
class LuaMethodWrapper <U, void(T:: *)(P1, P2)const, method>
{
public:
	static int invoke(lua_State * L)
	{
		LuaStack S(L);
		const U * obj = nullptr;

		if(!S.tryGet(1, obj))
			return S.retVoid();

		P1 p1;
		if(!S.tryGet(2, p1))
			return S.retVoid();

		P2 p2;
		if(!S.tryGet(3, p2))
			return S.retVoid();

		static auto functor = std::mem_fn(method);
		S.clear();
		functor(obj, p1, p2);
		return 0;
	}
};


//deprecated, should use LuaMethodWrapper instead, once implemented
template <typename T>
class LuaCallWrapper
{
public:
	using Wrapped = typename std::remove_const<T>::type;

	static std::function<int(lua_State *, T *)> createFunctor(void (Wrapped::* method)())
	{
		auto functor = std::mem_fn(method);
		auto ret = [=](lua_State * L, T * object)->int
		{
			LuaStack S(L);
			functor(object);
			return S.retVoid();
		};

		return ret;
	}

	static std::function<int(lua_State *, T *)> createFunctor(void (Wrapped::* method)() const)
	{
		auto functor = std::mem_fn(method);
		auto ret = [=](lua_State * L, T * object)->int
		{
			LuaStack S(L);
			functor(object);
			return S.retVoid();
		};

		return ret;
	}

	template <typename R>
	static std::function<int(lua_State *, T *)> createFunctor(R (Wrapped::* method)())
	{
		auto functor = std::mem_fn(method);
		auto ret = [=](lua_State * L, T * object)->int
		{
			LuaStack S(L);
			S.clear();
			S.push(functor(object));
			return S.retPushed();
		};

		return ret;
	}

	template <typename R>
	static std::function<int(lua_State *, T *)> createFunctor(R (Wrapped::* method)() const)
	{
		auto functor = std::mem_fn(method);
		auto ret = [=](lua_State * L, T * object)->int
		{
			LuaStack S(L);
			S.clear();
			S.push(functor(object));
			return S.retPushed();
		};

		return ret;
	}

	template <typename P1>
	static std::function<int(lua_State *, T *)> createFunctor(void (Wrapped::* method)(P1))
	{
		auto functor = std::mem_fn(method);
		auto ret = [=](lua_State * L, T * object)->int
		{
			LuaStack S(L);
			P1 p1;
			if(S.tryGet(1, p1))
			{
				functor(object, p1);
			}

			return S.retVoid();
		};

		return ret;
	}

	template <typename P1>
	static std::function<int(lua_State *, T *)> createFunctor(void (Wrapped::* method)(P1) const)
	{
		auto functor = std::mem_fn(method);
		auto ret = [=](lua_State * L, T * object)->int
		{
			LuaStack S(L);
			P1 p1;
			if(S.tryGet(1, p1))
			{
				functor(object, p1);
			}

			return S.retVoid();
		};

		return ret;
	}

	template <typename R, typename P1>
	static std::function<int(lua_State *, T *)> createFunctor(R (Wrapped::* method)(P1))
	{
		auto functor = std::mem_fn(method);
		auto ret = [=](lua_State * L, T * object)->int
		{
			LuaStack S(L);
			P1 p1;
			if(S.tryGet(1, p1))
			{
				S.push(functor(object, p1));
				return 1;
			}

			return S.retVoid();
		};

		return ret;
	}

	template <typename R, typename P1>
	static std::function<int(lua_State *, T *)> createFunctor(R (Wrapped::* method)(P1) const)
	{
		auto functor = std::mem_fn(method);
		auto ret = [=](lua_State * L, T * object)->int
		{
			LuaStack S(L);
			P1 p1;
			if(S.tryGet(1, p1))
			{
				S.push(functor(object, p1));
				return 1;
			}

			return S.retVoid();
		};

		return ret;
	}

};



}
