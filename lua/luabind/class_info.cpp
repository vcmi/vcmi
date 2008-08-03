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
#include <luabind/class_info.hpp>

namespace luabind
{
	class_info get_class_info(const object& o)
	{
		lua_State* L = o.interpreter();
	
		class_info result;
	
		o.push(L);
		detail::object_rep* obj = static_cast<detail::object_rep*>(lua_touserdata(L, -1));
		lua_pop(L, 1);

		result.name = obj->crep()->name();
		obj->crep()->get_table(L);

		object methods(from_stack(L, -1));
		
		methods.swap(result.methods);
		lua_pop(L, 1);
		
		result.attributes = newtable(L);

		typedef detail::class_rep::property_map map_type;
		
		std::size_t index = 1;
		
		for (map_type::const_iterator i = obj->crep()->properties().begin();
				i != obj->crep()->properties().end(); ++i)
		{
			result.attributes[index] = i->first;
		}

		return result;
	}

	void bind_class_info(lua_State* L)
	{
		module(L)
		[
			class_<class_info>("class_info_data")
				.def_readonly("name", &class_info::name)
				.def_readonly("methods", &class_info::methods)
				.def_readonly("attributes", &class_info::attributes),
		
			def("class_info", &get_class_info)
		];
	}
}

