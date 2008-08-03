// Copyright (c) 2003 Daniel Wallin and Arvid Norberg

// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF
// ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
// TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
// PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT
// SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
// OR OTHER DEALINGS IN THE SOFTWARE.
#include "stdafx.h"
#include <luabind/lua_include.hpp>

#include <luabind/detail/stack_utils.hpp>
#include <luabind/luabind.hpp>
#include <utility>

using namespace luabind::detail;

namespace luabind { namespace detail
{
	struct method_name
	{
		method_name(char const* n): name(n) {}
		bool operator()(method_rep const& o) const
		{ return std::strcmp(o.name, name) == 0; }
		char const* name;
	};
}}


#ifndef LUABIND_NO_ERROR_CHECKING

	std::string luabind::detail::get_overload_signatures_candidates(
			lua_State* L
			, std::vector<const overload_rep_base*>::iterator start
			, std::vector<const overload_rep_base*>::iterator end
			, std::string name)
	{
		std::string s;
		for (; start != end; ++start)
		{
			s += name;
			(*start)->get_signature(L, s);
			s += "\n";
		}
		return s;
	}

#endif


luabind::detail::class_rep::class_rep(LUABIND_TYPE_INFO type
	, const char* name
	, lua_State* L
	,  void(*destructor)(void*)
	,  void(*const_holder_destructor)(void*)
	, LUABIND_TYPE_INFO holder_type
	, LUABIND_TYPE_INFO const_holder_type
	, void*(*extractor)(void*)
	, const void*(*const_extractor)(void*)
	, void(*const_converter)(void*,void*)
	, void(*construct_holder)(void*,void*)
	, void(*construct_const_holder)(void*,void*)
	, void(*default_construct_holder)(void*)
	, void(*default_construct_const_holder)(void*)
	, void(*adopt_fun)(void*)
	, int holder_size
	, int holder_alignment)

	: m_type(type)
	, m_holder_type(holder_type)
	, m_const_holder_type(const_holder_type)
	, m_extractor(extractor)
	, m_const_extractor(const_extractor)
	, m_const_converter(const_converter)
	, m_construct_holder(construct_holder)
	, m_construct_const_holder(construct_const_holder)
	, m_default_construct_holder(default_construct_holder)
	, m_default_construct_const_holder(default_construct_const_holder)
	, m_adopt_fun(adopt_fun)
	, m_holder_size(holder_size)
	, m_holder_alignment(holder_alignment)
	, m_name(name)
	, m_class_type(cpp_class)
	, m_destructor(destructor)
	, m_const_holder_destructor(const_holder_destructor)
	, m_operator_cache(0)
{
	assert(m_holder_alignment >= 1 && "internal error");

	lua_newtable(L);
	handle(L, -1).swap(m_table);
	lua_newtable(L);
	handle(L, -1).swap(m_default_table);
	lua_pop(L, 2);

	class_registry* r = class_registry::get_registry(L);
	assert((r->cpp_class() != LUA_NOREF) && "you must call luabind::open()");

	detail::getref(L, r->cpp_class());
	lua_setmetatable(L, -2);

	lua_pushvalue(L, -1); // duplicate our user data
	m_self_ref.set(L);

	m_instance_metatable = r->cpp_instance();
}

luabind::detail::class_rep::class_rep(lua_State* L, const char* name)
	: m_type(LUABIND_INVALID_TYPE_INFO)
	, m_holder_type(LUABIND_INVALID_TYPE_INFO)
	, m_const_holder_type(LUABIND_INVALID_TYPE_INFO)
	, m_extractor(0)
	, m_const_extractor(0)
	, m_const_converter(0)
	, m_construct_holder(0)
	, m_construct_const_holder(0)
	, m_default_construct_holder(0)
	, m_default_construct_const_holder(0)
	, m_adopt_fun(0)
	, m_holder_size(0)
	, m_holder_alignment(1)
	, m_name(name)
	, m_class_type(lua_class)
	, m_destructor(0)
	, m_const_holder_destructor(0)
	, m_operator_cache(0)
{
	lua_newtable(L);
	handle(L, -1).swap(m_table);
	lua_newtable(L);
	handle(L, -1).swap(m_default_table);
	lua_pop(L, 2);

	class_registry* r = class_registry::get_registry(L);
	assert((r->cpp_class() != LUA_NOREF) && "you must call luabind::open()");

	detail::getref(L, r->lua_class());
	lua_setmetatable(L, -2);
	lua_pushvalue(L, -1); // duplicate our user data
	m_self_ref.set(L);

	m_instance_metatable = r->lua_instance();
}

luabind::detail::class_rep::~class_rep()
{
}

// leaves object on lua stack
std::pair<void*,void*> 
luabind::detail::class_rep::allocate(lua_State* L) const
{
	const int overlap = sizeof(object_rep)&(m_holder_alignment-1);
	const int padding = overlap==0?0:m_holder_alignment-overlap;
	const int size = sizeof(object_rep) + padding + m_holder_size;

	char* mem = static_cast<char*>(lua_newuserdata(L, size));
	char* ptr = mem + sizeof(object_rep) + padding;

	return std::pair<void*,void*>(mem,ptr);
}
/*
#include <iostream>
namespace
{
	void dump_stack(lua_State* L)
	{
		for (int i = 1; i <= lua_gettop(L); ++i)
		{
			int t = lua_type(L, i);
			switch (t)
			{
			case LUA_TNUMBER:
				std::cout << "[" << i << "] number: " << lua_tonumber(L, i) << "\n";
				break;
			case LUA_TSTRING:
				std::cout << "[" << i << "] string: " << lua_tostring(L, i) << "\n";
				break;
			case LUA_TUSERDATA:
				std::cout << "[" << i << "] userdata: " << lua_touserdata(L, i) << "\n";
				break;
			case LUA_TTABLE:
				std::cout << "[" << i << "] table:\n";
				break;
			case LUA_TNIL:
				std::cout << "[" << i << "] nil:\n";
				break;
			}
		}
	}
}
*/

void luabind::detail::class_rep::adopt(bool const_obj, void* obj)
{
	if (m_adopt_fun == 0) return;

	if (m_extractor)
	{
		assert(m_const_extractor);
		if (const_obj)
			m_adopt_fun(const_cast<void*>(m_const_extractor(obj)));
		else
			m_adopt_fun(m_extractor(obj));
	}
	else
	{
		m_adopt_fun(obj);
	}
}

// lua stack: userdata, key
int luabind::detail::class_rep::gettable(lua_State* L)
{
	// if key is nil, return nil
	if (lua_isnil(L, 2))
	{
		lua_pushnil(L);
		return 1;
	}

	object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));

	// we have to ignore the first argument since this may point to
	// a method that is not present in this class (but in a subclass)
	const char* key = lua_tostring(L, 2);

#ifndef LUABIND_NO_ERROR_CHECKING

	if (std::strlen(key) != lua_strlen(L, 2))
	{
		{
			std::string msg("luabind does not support "
				"member names with extra nulls:\n");
			msg += std::string(lua_tostring(L, 2), lua_strlen(L, 2));
			lua_pushstring(L, msg.c_str());
		}
		lua_error(L);
	}

#endif

	// special case to see if this is a null-pointer
	if (key && !std::strcmp(key, "__ok"))
	{
		class_rep* crep = obj->crep();

		void* p = crep->extractor() ? crep->extractor()(obj->ptr())
			: obj->ptr();

		lua_pushboolean(L, p != 0);
		return 1;
	}

// First, look in the instance's table
	detail::lua_reference const& tbl = obj->get_lua_table();
	if (tbl.is_valid())
	{
		tbl.get(L);
		lua_pushvalue(L, 2);
		lua_gettable(L, -2);
		if (!lua_isnil(L, -1)) 
		{
			lua_remove(L, -2); // remove table
			return 1;
		}
		lua_pop(L, 2);
	}

// Then look in the class' table for this member
	obj->crep()->get_table(L);
	lua_pushvalue(L, 2);
	lua_gettable(L, -2);

	if (!lua_isnil(L, -1)) 
	{
		lua_remove(L, -2); // remove table
		return 1;
	}
	lua_pop(L, 2);

	std::map<const char*, callback, ltstr>::iterator j = m_getters.find(key);
	if (j != m_getters.end())
	{
		// the name is a data member
		return j->second.func(L, j->second.pointer_offset);
	}

	lua_pushnil(L);
	return 1;
}

// called from the metamethod for __newindex
// the object pointer is passed on the lua stack
// lua stack: userdata, key, value
bool luabind::detail::class_rep::settable(lua_State* L)
{
	// if the key is 'nil' fail
	if (lua_isnil(L, 2)) return false;

	// we have to ignore the first argument since this may point to
	// a method that is not present in this class (but in a subclass)

	const char* key = lua_tostring(L, 2);

	if (std::strlen(key) == lua_strlen(L, 2))
	{
		std::map<const char*, callback, ltstr>::iterator j = m_setters.find(key);
		if (j != m_setters.end())
		{
			// the name is a data member
#ifndef LUABIND_NO_ERROR_CHECKING
			if (j->second.match(L, 3) < 0)
			{
				std::string msg("the attribute '");
				msg += m_name;
				msg += ".";
				msg += key;
				msg += "' is of type: ";
				j->second.sig(L, msg);
				msg += "\nand does not match: (";
				msg += stack_content_by_name(L, 3);
				msg += ")";
				lua_pushstring(L, msg.c_str());
				return false;
			}
#endif
			j->second.func(L, j->second.pointer_offset);
			return true;
		}

		if (m_getters.find(key) != m_getters.end())
		{
			// this means that we have a getter but no
			// setter for an attribute. We will then fail
			// because that attribute is read-only
			std::string msg("the attribute '");
			msg += m_name;
			msg += ".";
			msg += key;
			msg += "' is read only";
			lua_pushstring(L, msg.c_str());
			return false;
		}
	}

	// set the attribute to the object's table
	object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));
	detail::lua_reference& tbl = obj->get_lua_table();
	if (!tbl.is_valid())
	{
		// this is the first time we are trying to add
		// a member to this instance, create the table.
		lua_newtable(L);
		lua_pushvalue(L, -1);
		tbl.set(L);
	}
	else
	{
		tbl.get(L);
	}
	lua_pushvalue(L, 2);
	lua_pushvalue(L, 3);
	lua_settable(L, 4);
	lua_pop(L, 3);
	return true;
}

int class_rep::gettable_dispatcher(lua_State* L)
{
	object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));
	return obj->crep()->gettable(L);
}

// this is called as __newindex metamethod on every instance of this class
int luabind::detail::class_rep::settable_dispatcher(lua_State* L)
{
	object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));

	bool success = obj->crep()->settable(L);

#ifndef LUABIND_NO_ERROR_CHECKING

	if (!success)
	{
		// class_rep::settable() will leave
		// error message on the stack in case
		// of failure
		lua_error(L);
	}

#endif

	return 0;
}


int luabind::detail::class_rep::operator_dispatcher(lua_State* L)
{
	for (int i = 0; i < 2; ++i)
	{
		if (is_class_object(L, 1 + i))
		{
            int nargs = lua_gettop(L);

            lua_pushvalue(L, lua_upvalueindex(1));
			lua_gettable(L, 1 + i);

			if (lua_isnil(L, -1))
			{
				lua_pop(L, 1);
				continue;
			}

			lua_insert(L, 1); // move the function to the bottom

            nargs = lua_toboolean(L, lua_upvalueindex(2)) ? 1 : nargs;

            if (lua_toboolean(L, lua_upvalueindex(2))) // remove trailing nil
                lua_remove(L, 3);

            lua_call(L, nargs, 1);
            return 1;
		}
	}

	lua_pop(L, lua_gettop(L));
	lua_pushstring(L, "No such operator defined");
	lua_error(L);

	return 0;
}

// this is called as metamethod __call on the class_rep.
int luabind::detail::class_rep::constructor_dispatcher(lua_State* L)
{
	class_rep* crep = static_cast<class_rep*>(lua_touserdata(L, 1));
	construct_rep* rep = &crep->m_constructor;

	bool ambiguous = false;
	int match_index = -1;
	int min_match = std::numeric_limits<int>::max();
	bool found;

#ifdef LUABIND_NO_ERROR_CHECKING

	if (rep->overloads.size() == 1)
	{
		match_index = 0;
	}
	else
	{

#endif

		int num_params = lua_gettop(L) - 1;
		found = find_best_match(L, &rep->overloads.front(), rep->overloads.size(), sizeof(construct_rep::overload_t), ambiguous, min_match, match_index, num_params);

#ifdef LUABIND_NO_ERROR_CHECKING
	}

#else

	if (!found)
	{
		{
			std::string msg("no constructor of '");
			msg += crep->name();
			msg += "' matched the arguments (";
			msg += stack_content_by_name(L, 2);
			msg += ")\n candidates are:\n";

			msg += get_overload_signatures(L, rep->overloads.begin(), rep->overloads.end(), crep->name());

			lua_pushstring(L, msg.c_str());
		}
		lua_error(L);
	}
	else if (ambiguous)
	{
		{
			std::string msg("call of overloaded constructor '");
			msg += crep->m_name;
			msg +=  "(";
			msg += stack_content_by_name(L, 2);
			msg += ")' is ambiguous\nnone of the overloads have a best conversion:\n";

			std::vector<const overload_rep_base*> candidates;
			find_exact_match(L, &rep->overloads.front(), rep->overloads.size(), sizeof(construct_rep::overload_t), min_match, num_params, candidates);
			msg += get_overload_signatures_candidates(L, candidates.begin(), candidates.end(), crep->name());

			lua_pushstring(L, msg.c_str());
		}
		lua_error(L);
	}

#endif

#ifndef LUABIND_NO_EXCEPTIONS

	try
	{

#endif
		void* obj_rep;
		void* held;

		boost::tie(obj_rep,held) = crep->allocate(L);

		weak_ref backref(L, -1);

		void* object_ptr = rep->overloads[match_index].construct(L, backref);

		if (crep->has_holder())
		{
			crep->m_construct_holder(held, object_ptr);
			object_ptr = held;
		}
		new(obj_rep) object_rep(object_ptr, crep, object_rep::owner, crep->destructor());

		detail::getref(L, crep->m_instance_metatable);
		lua_setmetatable(L, -2);
		return 1;

#ifndef LUABIND_NO_EXCEPTIONS

	}
    
    catch(const error&)
    {
    }
	catch(const std::exception& e)
	{
		lua_pushstring(L, e.what());
	}
	catch(const char* s)
	{
		lua_pushstring(L, s);
	}
	catch(...)
	{
		{
			std::string msg = crep->name();
			msg += "() threw an exception";
			lua_pushstring(L, msg.c_str());
		}
	}

	// we can only reach this line if an exception was thrown
	lua_error(L);
	return 0; // will never be reached

#endif

}

/*
	the functions dispatcher assumes the following:

	upvalues:
	1: method_rep* method, points to the method_rep that this dispatcher is to call
	2: boolean force_static, is true if this is to be a static call
       and false if it is a normal call (= virtual if possible).

	stack:
	1: object_rep* self, points to the object the call is being made on
*/

int luabind::detail::class_rep::function_dispatcher(lua_State* L)
{
#ifndef NDEBUG

/*	lua_Debug tmp_;
	assert(lua_getinfo(L, "u", &tmp_));
	assert(tmp_.nups == 2);*/
	assert(lua_type(L, lua_upvalueindex(1)) == LUA_TLIGHTUSERDATA);
	assert(lua_type(L, lua_upvalueindex(2)) == LUA_TBOOLEAN);
	assert(lua_type(L, lua_upvalueindex(3)) == LUA_TLIGHTUSERDATA);
	assert(lua_touserdata(L, lua_upvalueindex(3)) == reinterpret_cast<void*>(0x1337));



//	assert(lua_type(L, 1) == LUA_TUSERDATA);

#endif

	method_rep* rep = static_cast<method_rep*>(lua_touserdata(L, lua_upvalueindex(1)));
	int force_static_call = lua_toboolean(L, lua_upvalueindex(2));

	bool ambiguous = false;
	int match_index = -1;
	int min_match = std::numeric_limits<int>::max();
	bool found;

#ifdef LUABIND_NO_ERROR_CHECKING
	if (rep->overloads().size() == 1)
	{
		match_index = 0;
	}
	else
	{
#endif

		int num_params = lua_gettop(L) /*- 1*/;
		found = find_best_match(L, &rep->overloads().front(), rep->overloads().size()
			, sizeof(overload_rep), ambiguous, min_match, match_index, num_params);

#ifdef LUABIND_NO_ERROR_CHECKING

	}

#else

	if (!found)
	{
		{
			std::string msg = "no overload of  '";
			msg += rep->crep->name();
			msg += ":";
			msg += rep->name;
			msg += "' matched the arguments (";
			msg += stack_content_by_name(L, 1);
			msg += ")\ncandidates are:\n";

			std::string function_name;
			function_name += rep->crep->name();
			function_name += ":";
			function_name += rep->name;

			msg += get_overload_signatures(L, rep->overloads().begin()
				, rep->overloads().end(), function_name);

			lua_pushstring(L, msg.c_str());
		}
		lua_error(L);
	}
	else if (ambiguous)
	{
		{
			std::string msg = "call of overloaded  '";
			msg += rep->crep->name();
			msg += ":";
			msg += rep->name;
			msg += "(";
			msg += stack_content_by_name(L, 1);
			msg += ")' is ambiguous\nnone of the overloads have a best conversion:\n";

			std::vector<const overload_rep_base*> candidates;
			find_exact_match(L, &rep->overloads().front(), rep->overloads().size()
				, sizeof(overload_rep), min_match, num_params, candidates);

			std::string function_name;
			function_name += rep->crep->name();
			function_name += ":";
			function_name += rep->name;

			msg += get_overload_signatures_candidates(L, candidates.begin()
				, candidates.end(), function_name);

			lua_pushstring(L, msg.c_str());
		}
		lua_error(L);
	}

#endif

#ifndef LUABIND_NO_EXCEPTIONS

	try
	{

#endif

		const overload_rep& o = rep->overloads()[match_index];

        if (force_static_call && !o.has_static())
		{
			lua_pushstring(L, "pure virtual function called");
        }
		else
		{
	        return o.call(L, force_static_call != 0);
		}

#ifndef LUABIND_NO_EXCEPTIONS

	}
    catch(const error&)
    {
    }
    catch(const std::exception& e)
	{
		lua_pushstring(L, e.what());
	}
	catch (const char* s)
	{
		lua_pushstring(L, s);
	}
	catch(...)
	{
		std::string msg = rep->crep->name();
		msg += ":";
		msg += rep->name;
		msg += "() threw an exception";
		lua_pushstring(L, msg.c_str());
	}

#endif

	// we can only reach this line if an error occured
	lua_error(L);
	return 0; // will never be reached
}

#ifndef NDEBUG

#ifndef BOOST_NO_STRINGSTREAM
#include <sstream>
#else
#include <strstream>
#endif

namespace
{
	std::string to_string(luabind::object const& o)
	{
		using namespace luabind;
		if (type(o) == LUA_TSTRING) return object_cast<std::string>(o);
		lua_State* L = o.interpreter();
		LUABIND_CHECK_STACK(L);

#ifdef BOOST_NO_STRINGSTREAM
		std::strstream s;
#else
		std::stringstream s;
#endif

		if (type(o) == LUA_TNUMBER)
		{
			s << object_cast<float>(o);
			return s.str();
		}

		s << "<" << lua_typename(L, type(o)) << ">";
#ifdef BOOST_NO_STRINGSTREAM
		s << std::ends;
#endif
		return s.str();
	}


	std::string member_to_string(luabind::object const& e)
	{
#if !defined(LUABIND_NO_ERROR_CHECKING)
        using namespace luabind;
		lua_State* L = e.interpreter();
		LUABIND_CHECK_STACK(L);

		if (type(e) == LUA_TFUNCTION)
		{
			e.push(L);
			detail::stack_pop p(L, 1);

			{
				if (lua_getupvalue(L, -1, 3) == 0) return to_string(e);
				detail::stack_pop p2(L, 1);
				if (lua_touserdata(L, -1) != reinterpret_cast<void*>(0x1337)) return to_string(e);
			}

#ifdef BOOST_NO_STRINGSTREAM
			std::strstream s;
#else
			std::stringstream s;
#endif
			{
				lua_getupvalue(L, -1, 2);
				detail::stack_pop p2(L, 1);
				int b = lua_toboolean(L, -1);
				s << "<c++ function";
				if (b) s << " (default)";
				s << "> ";
			}

			{
				lua_getupvalue(L, -1, 1);
				detail::stack_pop p2(L, 1);
				method_rep* m = static_cast<method_rep*>(lua_touserdata(L, -1));
				s << m << "\n";
				for (std::vector<overload_rep>::const_iterator i = m->overloads().begin();
					i != m->overloads().end(); ++i)
				{
					std::string str;
					i->get_signature(L, str);
					s << "   " << str << "\n";
				}
			}
#ifdef BOOST_NO_STRINGSTREAM
			s << std::ends;
#endif
			return s.str();
		}

        return to_string(e);
#else
        return "";
#endif
	}
}

std::string luabind::detail::class_rep::class_info_string(lua_State* L) const
{
#ifdef BOOST_NO_STRINGSTREAM
	std::strstream ret;
#else
	std::stringstream ret;
#endif

	ret << "CLASS: " << m_name << "\n";

	ret << "dynamic dispatch functions:\n------------------\n";

	for (luabind::iterator i(m_table), end; i != end; ++i)
	{
		luabind::object e = *i;
		ret << "  " << to_string(i.key()) << ": " << member_to_string(e) << "\n";
	}

	ret << "default implementations:\n------------------\n";

	for (luabind::iterator i(m_default_table), end; i != end; ++i)
	{
		luabind::object e = *i;
		ret << "  " << to_string(i.key()) << ": " << member_to_string(e) << "\n";
	}
#ifdef BOOST_NO_STRINGSTREAM
	ret << std::ends;
#endif
	return ret.str();
}
#endif

void luabind::detail::class_rep::add_base_class(const luabind::detail::class_rep::base_info& binfo)
{
	// If you hit this assert you are deriving from a type that is not registered
	// in lua. That is, in the class_<> you are giving a baseclass that isn't registered.
	// Please note that if you don't need to have access to the base class or the
	// conversion from the derived class to the base class, you don't need
	// to tell luabind that it derives.
	assert(binfo.base && "You cannot derive from an unregistered type");

	class_rep* bcrep = binfo.base;

	// import all functions from the base
	typedef std::list<detail::method_rep> methods_t;

	for (methods_t::const_iterator i = bcrep->m_methods.begin();
		i != bcrep->m_methods.end(); ++i)
    {
		add_method(*i);
    }

	// import all getters from the base
	for (std::map<const char*, callback, ltstr>::const_iterator i = bcrep->m_getters.begin(); 
			i != bcrep->m_getters.end(); ++i)
	{
		callback& m = m_getters[i->first];
		m.pointer_offset = i->second.pointer_offset + binfo.pointer_offset;
		m.func = i->second.func;

#ifndef LUABIND_NO_ERROR_CHECKING
		m.match = i->second.match;
		m.sig = i->second.sig;
#endif
	}

	// import all setters from the base
	for (std::map<const char*, callback, ltstr>::const_iterator i = bcrep->m_setters.begin(); 
			i != bcrep->m_setters.end(); ++i)
	{
		callback& m = m_setters[i->first];
		m.pointer_offset = i->second.pointer_offset + binfo.pointer_offset;
		m.func = i->second.func;

#ifndef LUABIND_NO_ERROR_CHECKING
		m.match = i->second.match;
		m.sig = i->second.sig;
#endif
	}

	// import all static constants
	for (std::map<const char*, int, ltstr>::const_iterator i = bcrep->m_static_constants.begin(); 
			i != bcrep->m_static_constants.end(); ++i)
	{
		int& v = m_static_constants[i->first];
		v = i->second;
	}

	// import all operators
	for (int i = 0; i < number_of_operators; ++i)
	{
		for (std::vector<operator_callback>::const_iterator j = bcrep->m_operators[i].begin(); 
				j != bcrep->m_operators[i].end(); ++j)
			m_operators[i].push_back(*j);
	}

	// also, save the baseclass info to be used for typecasts
	m_bases.push_back(binfo);
}

int luabind::detail::class_rep::super_callback(lua_State* L)
{
	int args = lua_gettop(L);
		
	object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, lua_upvalueindex(2)));
	class_rep* crep = static_cast<class_rep*>(lua_touserdata(L, lua_upvalueindex(1)));
	class_rep* base = crep->bases()[0].base;

	if (base->get_class_type() == class_rep::lua_class)
	{
		if (base->bases().empty())
		{
			obj->set_flags(obj->flags() & ~object_rep::call_super);

			lua_pushstring(L, "super");
			lua_pushnil(L);
			lua_settable(L, LUA_GLOBALSINDEX);
		}
		else
		{
			lua_pushstring(L, "super");
			lua_pushlightuserdata(L, base);
			lua_pushvalue(L, lua_upvalueindex(2));
			lua_pushcclosure(L, super_callback, 2);
			lua_settable(L, LUA_GLOBALSINDEX);
		}

		base->get_table(L);
		lua_pushstring(L, "__init");
		lua_gettable(L, -2);
		lua_insert(L, 1);
		lua_pop(L, 1);

		lua_pushvalue(L, lua_upvalueindex(2));
		lua_insert(L, 2);

		lua_call(L, args + 1, 0);

		// TODO: instead of clearing the global variable "super"
		// store it temporarily in the registry. maybe we should
		// have some kind of warning if the super global is used?
		lua_pushstring(L, "super");
		lua_pushnil(L);
		lua_settable(L, LUA_GLOBALSINDEX);
	}
	else
	{
		obj->set_flags(obj->flags() & ~object_rep::call_super);

		// we need to push some garbage at index 1 to make the construction work
		lua_pushboolean(L, 1);
		lua_insert(L, 1);

		construct_rep* rep = &base->m_constructor;

		bool ambiguous = false;
		int match_index = -1;
		int min_match = std::numeric_limits<int>::max();
		bool found;
			
#ifdef LUABIND_NO_ERROR_CHECKING

		if (rep->overloads.size() == 1)
		{
			match_index = 0;
		}
		else
		{

#endif

			int num_params = lua_gettop(L) - 1;
			found = find_best_match(L, &rep->overloads.front(), rep->overloads.size(), sizeof(construct_rep::overload_t), ambiguous, min_match, match_index, num_params);

#ifdef LUABIND_NO_ERROR_CHECKING

		}

#else
				
		if (!found)
		{
			{
				std::string msg = "no constructor of '";
				msg += base->m_name;
				msg += "' matched the arguments (";
				msg += stack_content_by_name(L, 2);
				msg += ")";
				lua_pushstring(L, msg.c_str());
			}
			lua_error(L);
		}
		else if (ambiguous)
		{
			{
				std::string msg = "call of overloaded constructor '";
				msg += base->m_name;
				msg +=  "(";
				msg += stack_content_by_name(L, 2);
				msg += ")' is ambiguous";
				lua_pushstring(L, msg.c_str());
			}
			lua_error(L);
		}

			// TODO: should this be a warning or something?
/*
			// since the derived class is a lua class
			// it may have reimplemented virtual functions
			// therefore, we have to instantiate the Basewrapper
			// if there is no basewrapper, throw a run-time error
			if (!rep->overloads[match_index].has_wrapped_construct())
			{
				{
					std::string msg = "Cannot derive from C++ class '";
					msg += base->name();
					msg += "'. It does not have a wrapped type";
					lua_pushstring(L, msg.c_str());
				}
				lua_error(L);
			}
*/
#endif

#ifndef LUABIND_NO_EXCEPTIONS

		try
		{

#endif
			lua_pushvalue(L, lua_upvalueindex(2));
			weak_ref backref(L, -1);
			lua_pop(L, 1);

			void* storage_ptr = obj->ptr();		

			if (!rep->overloads[match_index].has_wrapped_construct())
			{
				// if the type doesn't have a wrapped type, use the ordinary constructor
				void* instance = rep->overloads[match_index].construct(L, backref);

				if (crep->has_holder())
				{
					crep->m_construct_holder(storage_ptr, instance);
				}
				else
				{
					obj->set_object(instance);
				}
			}
			else
			{
				// get reference to lua object
/*				lua_pushvalue(L, lua_upvalueindex(2));
				detail::lua_reference ref;
				ref.set(L);
				void* instance = rep->overloads[match_index].construct_wrapped(L, ref);*/

				void* instance = rep->overloads[match_index].construct_wrapped(L, backref);

				if (crep->has_holder())
				{
					crep->m_construct_holder(storage_ptr, instance);			
				}
				else
				{
					obj->set_object(instance);
				}
			}
			// TODO: is the wrapped type destructed correctly?
			// it should, since the destructor is either the wrapped type's
			// destructor or the base type's destructor, depending on wether
			// the type has a wrapped type or not.
			obj->set_destructor(base->destructor());
			return 0;

#ifndef LUABIND_NO_EXCEPTIONS

		}
        catch(const error&)
        {
        }
        catch(const std::exception& e)
		{
			lua_pushstring(L, e.what());
		}
		catch(const char* s)
		{
			lua_pushstring(L, s);
		}
		catch(...)
		{
			std::string msg = base->m_name;
			msg += "() threw an exception";
			lua_pushstring(L, msg.c_str());
		}
		// can only be reached if an exception was thrown
		lua_error(L);
#endif
	}

	return 0;

}



int luabind::detail::class_rep::lua_settable_dispatcher(lua_State* L)
{
	class_rep* crep = static_cast<class_rep*>(lua_touserdata(L, 1));

	// get first table
	crep->get_table(L);

	// copy key, value
	lua_pushvalue(L, -3);
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	// pop table
	lua_pop(L, 1);

	// get default table
	crep->get_default_table(L);
	lua_replace(L, 1);
	lua_rawset(L, -3);

	crep->m_operator_cache = 0; // invalidate cache
	
	return 0;
}

int luabind::detail::class_rep::construct_lua_class_callback(lua_State* L)
{
	class_rep* crep = static_cast<class_rep*>(lua_touserdata(L, 1));

	int args = lua_gettop(L);

	// lua stack: crep <arguments>

	lua_newtable(L);
	detail::lua_reference ref;
	ref.set(L);

	bool has_bases = !crep->bases().empty();
		
	if (has_bases)
	{
		lua_pushstring(L, "super");
		lua_pushvalue(L, 1); // crep
	}

	// lua stack: crep <arguments> "super" crep
	// or
	// lua stack: crep <arguments>

	// if we have a baseclass we set the flag to say that the super has not yet been called
	// we will use this flag later to check that it actually was called from __init()
	int flags = object_rep::lua_class | object_rep::owner | (has_bases ? object_rep::call_super : 0);

//	void* obj_ptr = lua_newuserdata(L, sizeof(object_rep));
	void* obj_ptr;
	void* held_storage;

	boost::tie(obj_ptr, held_storage) = crep->allocate(L);
	(new(obj_ptr) object_rep(crep, flags, ref))->set_object(held_storage);

	detail::getref(L, crep->metatable_ref());
	lua_setmetatable(L, -2);

	// lua stack: crep <arguments> "super" crep obj_ptr
	// or
	// lua stack: crep <arguments> obj_ptr

	if (has_bases)	lua_pushvalue(L, -1); // obj_ptr
	lua_replace(L, 1); // obj_ptr

	// lua stack: obj_ptr <arguments> "super" crep obj_ptr
	// or
	// lua stack: obj_ptr <arguments>

	if (has_bases)
	{
		lua_pushcclosure(L, super_callback, 2);
		// lua stack: crep <arguments> "super" function
		lua_settable(L, LUA_GLOBALSINDEX);
	}

	// lua stack: crep <arguments>

	lua_pushvalue(L, 1);
	lua_insert(L, 1);

	crep->get_table(L);
	lua_pushstring(L, "__init");
	lua_gettable(L, -2);

#ifndef LUABIND_NO_ERROR_CHECKING

	// TODO: should this be a run-time error?
	// maybe the default behavior should be to just call
	// the base calss' constructor. We should register
	// the super callback funktion as __init
	if (!lua_isfunction(L, -1))
	{
		{
			std::string msg = crep->name();
			msg += ":__init is not defined";
			lua_pushstring(L, msg.c_str());
		}
		lua_error(L);
	}

#endif

	lua_insert(L, 2); // function first on stack
	lua_pop(L, 1);
	// TODO: lua_call may invoke longjump! make sure we don't have any memory leaks!
	// we don't have any stack objects here
	lua_call(L, args, 0);

#ifndef LUABIND_NO_ERROR_CHECKING

	object_rep* obj = static_cast<object_rep*>(obj_ptr);
	if (obj->flags() & object_rep::call_super)
	{
		lua_pushstring(L, "derived class must call super on base");
		lua_error(L);
	}

#endif

	return 1;
}

// called from the metamethod for __index
// obj is the object pointer
int luabind::detail::class_rep::lua_class_gettable(lua_State* L)
{
	object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));
	class_rep* crep = obj->crep();

#ifndef LUABIND_NO_ERROR_CHECKING

	if (obj->flags() & object_rep::call_super)
	{
		lua_pushstring(L, "derived class must call super on base");
		lua_error(L);
	}

#endif

	// we have to ignore the first argument since this may point to
	// a method that is not present in this class (but in a subclass)

	// BUG: This might catch members called "__ok\0foobar"
	const char* key = lua_tostring(L, 2);

	if (key && !std::strcmp(key, "__ok"))
	{
		class_rep* crep = obj->crep();

		void* p = crep->extractor() ? crep->extractor()(obj->ptr())
			: obj->ptr();

		lua_pushboolean(L, p != 0);
		return 1;
	}
	
	// first look in the instance's table
	detail::lua_reference const& tbl = obj->get_lua_table();
	assert(tbl.is_valid());
	tbl.get(L);
	lua_pushvalue(L, 2);
	lua_gettable(L, -2);
	if (!lua_isnil(L, -1)) 
	{
		lua_remove(L, -2); // remove table
		return 1;
	}
	lua_pop(L, 2);

	// then look in the class' table
	crep->get_table(L);
	lua_pushvalue(L, 2);
	lua_gettable(L, -2);

	if (!lua_isnil(L, -1)) 
	{
		lua_remove(L, -2); // more table
		return 1;
	}

	lua_pop(L, 2);

	if (lua_isnil(L, 2))
	{
		lua_pushnil(L);
		return 1;
	}

	std::map<const char*, class_rep::callback, ltstr>::iterator j = crep->m_getters.find(key);
	if (j != crep->m_getters.end())
	{
		// the name is a data member
		return j->second.func(L, j->second.pointer_offset);
	}

	lua_pushnil(L);
	return 1;
}

// called from the metamethod for __newindex
// obj is the object pointer
int luabind::detail::class_rep::lua_class_settable(lua_State* L)
{
	object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, 1));
	class_rep* crep = obj->crep();

#ifndef LUABIND_NO_ERROR_CHECKING

	if (obj->flags() & object_rep::call_super)
	{
		// this block makes sure the std::string is destructed
		// before lua_error is called
		{
			std::string msg = "derived class '";
			msg += crep->name();
			msg += "'must call super on base";
			lua_pushstring(L, msg.c_str());
		}
		lua_error(L);
	}

#endif

	// we have to ignore the first argument since this may point to
	// a method that is not present in this class (but in a subclass)
	// BUG: This will not work with keys with extra nulls in them
	const char* key = lua_tostring(L, 2);


	std::map<const char*, class_rep::callback, ltstr>::iterator j = crep->m_setters.find(key);

	// if the strlen(key) is not the true length,
	// it means that the member-name contains
	// extra nulls. luabind does not support such
	// names as member names. So, use the lua
	// table as fall-back
	if (j == crep->m_setters.end()
		|| std::strlen(key) != lua_strlen(L, 2))
	{
		std::map<const char*, class_rep::callback, ltstr>::iterator k = crep->m_getters.find(key);

#ifndef LUABIND_NO_ERROR_CHECKING

		if (k != crep->m_getters.end())
		{
			{
				std::string msg = "cannot set property '";
				msg += crep->name();
				msg += ".";
				msg += key;
				msg += "', because it's read only";
				lua_pushstring(L, msg.c_str());
			}
			lua_error(L);
		}

#endif

		detail::lua_reference const& tbl = obj->get_lua_table();
		assert(tbl.is_valid());
		tbl.get(L);
		lua_replace(L, 1);
		lua_settable(L, 1);
	}
	else
	{
		// the name is a data member
		j->second.func(L, j->second.pointer_offset);
	}

	return 0;
}

/*
	stack:
	1: class_rep
	2: member name
*/
int luabind::detail::class_rep::static_class_gettable(lua_State* L)
{
	class_rep* crep = static_cast<class_rep*>(lua_touserdata(L, 1));

	// look in the static function table
	crep->get_default_table(L);
	lua_pushvalue(L, 2);
	lua_gettable(L, -2);
	if (!lua_isnil(L, -1)) return 1;
	else lua_pop(L, 2);

	const char* key = lua_tostring(L, 2);

	if (std::strlen(key) != lua_strlen(L, 2))
	{
		lua_pushnil(L);
		return 1;
	}

	std::map<const char*, int, ltstr>::const_iterator j = crep->m_static_constants.find(key);

	if (j != crep->m_static_constants.end())
	{
		lua_pushnumber(L, j->second);
		return 1;
	}

#ifndef LUABIND_NO_ERROR_CHECKING

	{
		std::string msg = "no static '";
		msg += key;
		msg += "' in class '";
		msg += crep->name();
		msg += "'";
		lua_pushstring(L, msg.c_str());
	}
	lua_error(L);

#endif

	lua_pushnil(L);

	return 1;
}

bool luabind::detail::is_class_rep(lua_State* L, int index)
{
	if (lua_getmetatable(L, index) == 0) return false;

	lua_pushstring(L, "__luabind_classrep");
	lua_gettable(L, -2);
	if (lua_toboolean(L, -1))
	{
		lua_pop(L, 2);
		return true;
	}

	lua_pop(L, 2);
	return false;
}

void luabind::detail::finalize(lua_State* L, class_rep* crep)
{
	if (crep->get_class_type() != class_rep::lua_class) return;

//	lua_pushvalue(L, -1); // copy the object ref
	crep->get_table(L);
	lua_pushstring(L, "__finalize");
	lua_gettable(L, -2);
	lua_remove(L, -2);

	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
	}
	else
	{
		lua_pushvalue(L, -2);
		lua_call(L, 1, 0);
	}

	for (std::vector<class_rep::base_info>::const_iterator 
			i = crep->bases().begin(); i != crep->bases().end(); ++i)
	{
		if (i->base) finalize(L, i->base);
	}
}

void* luabind::detail::class_rep::convert_to(
	LUABIND_TYPE_INFO target_type
	, const object_rep* obj
	, void* target_memory) const
{
	// TODO: since this is a member function, we don't have to use the accesor functions for
	// the types and the extractor

	assert(obj == 0 || obj->crep() == this);

	int steps = 0;
	int offset = 0;
	if (!(LUABIND_TYPE_INFO_EQUAL(holder_type(), target_type))
		&& !(LUABIND_TYPE_INFO_EQUAL(const_holder_type(), target_type)))
	{
		steps = implicit_cast(this, target_type, offset);
	}

	// should never be called with a type that can't be cast
	assert((steps >= 0) && "internal error, please report");

	if (LUABIND_TYPE_INFO_EQUAL(target_type, holder_type()))
	{
		if (obj == 0)
		{
			// we are trying to convert nil to a holder type
			m_default_construct_holder(target_memory);
			return target_memory;
		}
		// if the type we are trying to convert to is the holder_type
		// it means that his crep has a holder_type (since it would have
		// been invalid otherwise, and T cannot be invalid). It also means
		// that we need no conversion, since the holder_type is what the
		// object points to.
		return obj->ptr();
	}

	if (LUABIND_TYPE_INFO_EQUAL(target_type, const_holder_type()))
	{
		if (obj == 0)
		{
			// we are trying to convert nil to a const holder type
			m_default_construct_const_holder(target_memory);
			return target_memory;
		}

		if (obj->flags() & object_rep::constant)
		{
			// we are holding a constant
			return obj->ptr();
		}
		else
		{
			// we are holding a non-constant, we need to convert it
			// to a const_holder.
			m_const_converter(obj->ptr(), target_memory);
			return target_memory;
		}
	}

	void* raw_pointer;

	if (has_holder())
	{
		assert(obj);
		// this means that we have a holder type where the
		// raw-pointer needs to be extracted
		raw_pointer = extractor()(obj->ptr());
	}
	else
	{
		if (obj == 0) raw_pointer = 0;
		else raw_pointer = obj->ptr();
	}

	return static_cast<char*>(raw_pointer) + offset;
}

void luabind::detail::class_rep::cache_operators(lua_State* L)
{
	m_operator_cache = 0x1;

	for (int i = 0; i < number_of_operators; ++i)
	{
		get_table(L);
		lua_pushstring(L, get_operator_name(i));
		lua_rawget(L, -2);

		if (lua_isfunction(L, -1)) m_operator_cache |= 1 << (i + 1);

		lua_pop(L, 2);
	}
}

bool luabind::detail::class_rep::has_operator_in_lua(lua_State* L, int id)
{
	if ((m_operator_cache & 0x1) == 0)
		cache_operators(L);

	const int mask = 1 << (id + 1);

	return (m_operator_cache & mask) != 0;
}


// this will merge all overloads of fun into the list of
// overloads in this class
void luabind::detail::class_rep::add_method(luabind::detail::method_rep const& fun)
{
	typedef std::list<detail::method_rep> methods_t;

	methods_t::iterator m = std::find_if(
		m_methods.begin()
		, m_methods.end()
		, method_name(fun.name));
	if (m == m_methods.end())
	{
		m_methods.push_back(method_rep());
		m = m_methods.end();
		std::advance(m, -1);
		m->name = fun.name;
	}
	m->crep = this;

	typedef std::vector<detail::overload_rep> overloads_t;

    for (overloads_t::const_iterator j = fun.overloads().begin();
		j != fun.overloads().end(); ++j)
    {
        detail::overload_rep o = *j;
        m->add_overload(o);
    }
}

// this function will add all the overloads in method rep to
// this class' lua tables. If there already are overloads with this
// name, thses will simply be appended to the overload list
void luabind::detail::class_rep::register_methods(lua_State* L)
{
	LUABIND_CHECK_STACK(L);
	// insert the function in the normal member table
	// and in the default member table
	m_default_table.push(L);
	m_table.push(L);

	// pops the tables
	detail::stack_pop pop_tables(L, 2);

	for (std::list<method_rep>::const_iterator m = m_methods.begin();
		m != m_methods.end(); ++m)
	{
		// create the function closure in m_table
		lua_pushstring(L, m->name);
		lua_pushlightuserdata(L, const_cast<void*>((const void*)&(*m)));
		lua_pushboolean(L, 0);
		lua_pushlightuserdata(L, reinterpret_cast<void*>(0x1337));
		lua_pushcclosure(L, function_dispatcher, 3);
		lua_settable(L, -3);

		// create the function closure in m_default_table
		lua_pushstring(L, m->name);
		lua_pushlightuserdata(L, const_cast<void*>((const void*)&(*m)));
		lua_pushboolean(L, 1);
		lua_pushlightuserdata(L, reinterpret_cast<void*>(0x1337));
		lua_pushcclosure(L, function_dispatcher, 3);
		lua_settable(L, -4);
	}
}

const class_rep::property_map& luabind::detail::class_rep::properties() const
{
	return m_getters;
}

