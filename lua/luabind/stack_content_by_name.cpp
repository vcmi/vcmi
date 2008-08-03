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

#include <luabind/luabind.hpp>

using namespace luabind::detail;

std::string luabind::detail::stack_content_by_name(lua_State* L, int start_index)
{
	std::string ret;
	int top = lua_gettop(L);
	for (int i = start_index; i <= top; ++i)
	{
		object_rep* obj = is_class_object(L, i);
		class_rep* crep = is_class_rep(L, i)?(class_rep*)lua_touserdata(L, i):0;
		if (obj == 0 && crep == 0)
		{
			int type = lua_type(L, i);
			ret += lua_typename(L, type);
		}
		else if (obj)
		{
			if (obj->flags() & object_rep::constant) ret += "const ";
			ret += obj->crep()->name();
		}
		else if (crep)
		{
			ret += "<";
			ret += crep->name();
			ret += ">";
		}
		if (i < top) ret += ", ";
	}
	return ret;
}

