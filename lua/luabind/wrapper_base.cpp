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
#include <luabind/config.hpp>
#include <luabind/lua_include.hpp>
#include <luabind/detail/object_rep.hpp>
#include <luabind/detail/class_rep.hpp>
#include <luabind/detail/stack_utils.hpp>

namespace luabind { namespace detail
{
	void do_call_member_selection(lua_State* L, char const* name)
	{
		object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, -1));
		lua_pop(L, 1); // pop self

		obj->crep()->get_table(L); // push the crep table
		lua_pushstring(L, name);
		lua_gettable(L, -2);
		lua_remove(L, -2); // remove the crep table

		{
			if (!lua_iscfunction(L, -1)) return;
			if (lua_getupvalue(L, -1, 3) == 0) return;
			detail::stack_pop p(L, 1);
			if (lua_touserdata(L, -1) != reinterpret_cast<void*>(0x1337)) return;
		}

		// this (usually) means the function has not been
		// overridden by lua, call the default implementation
		lua_pop(L, 1);
		obj->crep()->get_default_table(L); // push the crep table
		lua_pushstring(L, name);
		lua_gettable(L, -2);
		assert(!lua_isnil(L, -1));
		lua_remove(L, -2); // remove the crep table
	}
}}
