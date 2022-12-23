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

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{

namespace detail
{

template<int ...>
struct Seq {};

template<int N, int ...S>
struct Gens : Gens<N-1, N-1, S...> {};

template<int ...S>
struct Gens<0, S...>
{
	typedef Seq<S...> type;
};

template <typename R, typename ... Args>
class LuaArgumentsTuple
{
public:
	using TupleData = std::tuple<Args ...>;
	using Functor = R(*)(Args ...);

	TupleData args;
	Functor f;

	LuaArgumentsTuple(Functor _f)
		:f(_f),
		args()
	{
	}


	STRONG_INLINE int invoke(lua_State * L)
	{
		return callFunc(L, typename Gens<sizeof...(Args)>::type());
	}
private:
	template<int ...N>
	int callFunc(lua_State * L, Seq<N...>)
	{
		LuaStack S(L);

		bool ok[sizeof...(Args)] = {(S.tryGet(N+1, std::get<N>(args)))...};

		if(std::count(std::begin(ok), std::end(ok), false) > 0)
			return S.retVoid();


		R ret = f(std::get<N>(args) ...);
		S.clear();
		S.push(ret);
		return 1;
	}
};


class LuaFunctionInvoker
{
public:
	template<typename R, typename ... Args>
	static STRONG_INLINE int invoke(lua_State * L, R(*f)(Args ...))
	{
		LuaArgumentsTuple<R, Args ...> args(f);
		return args.invoke(L);
	}
};

}

template <typename F, F f>
class LuaFunctionWrapper
{
public:
	static int invoke(lua_State * L)
	{
		return detail::LuaFunctionInvoker::invoke(L, f);
	}
};


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

template <typename U, typename T, typename R, R(T:: * method)()>
class LuaMethodWrapper <U, R(T:: *)(), method>
{
public:
	static int invoke(lua_State * L)
	{
		LuaStack S(L);
		U * obj = nullptr;

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

template <typename U, typename T, void(T:: * method)()>
class LuaMethodWrapper <U, void(T:: *)(), method>
{
public:
	static int invoke(lua_State * L)
	{
		LuaStack S(L);
		U * obj = nullptr;

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

template <typename U, typename T, typename R, typename P1, R(T:: * method)(P1)>
class LuaMethodWrapper <U, R(T:: *)(P1), method>
{
public:
	static int invoke(lua_State * L)
	{
		LuaStack S(L);
		U * obj = nullptr;

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

template <typename U, typename T, typename P1, void(T:: * method)(P1)>
class LuaMethodWrapper <U, void(T:: *)(P1), method>
{
public:
	static int invoke(lua_State * L)
	{
		LuaStack S(L);
		U * obj = nullptr;

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

}

VCMI_LIB_NAMESPACE_END
