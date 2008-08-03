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
#include <luabind/detail/object_rep.hpp>
#include <luabind/detail/class_rep.hpp>

namespace luabind { namespace detail
{

	// dest is a function that is called to delete the c++ object this struct holds
	object_rep::object_rep(void* obj, class_rep* crep, int flags, void(*dest)(void*))
		: m_object(obj)
		, m_classrep(crep)
		, m_flags(flags)
		, m_destructor(dest)
		, m_dependency_cnt(1)
	{
		// if the object is owned by lua, a valid destructor must be given
		assert((((m_flags & owner) && dest) || !(m_flags & owner)) && "internal error, please report");
	}

	object_rep::object_rep(class_rep* crep, int flags, detail::lua_reference const& table_ref)
		: m_object(0)
		, m_classrep(crep)
		, m_flags(flags)
		, m_lua_table_ref(table_ref)
		, m_destructor(0)
		, m_dependency_cnt(1)
	{
	}

	object_rep::~object_rep() 
	{
		if (m_flags & owner && m_destructor) m_destructor(m_object);
	}

	void object_rep::remove_ownership()
	{
		assert((m_flags & owner) && "cannot remove ownership of object that's not owned");
		assert(m_classrep->get_class_type() == class_rep::cpp_class
			|| m_classrep->bases().size() == 1 && "can only adopt c++ types or lua classes that derives from a c++ class");

        // daniel040727 Bogus assert above? C++ types can be adopted just fine
        // without a hierarchy?

		m_flags &= ~owner;
		// if this is a type with a wrapper we also have to
		// transform the wrappers weak_ref into a strong
		// reference, to make sure the lua part
		// stays alive as long as the c++ part stays
		// alive.
/*		if (m_classrep->get_class_type() == class_rep::cpp_class)
			m_classrep->adopt(m_flags & constant, m_object);
		else*/

        // daniel040727 I changed the above. It just seems wrong. adopt()
        // should only be called on the wrapper type.
        if (m_classrep->get_class_type() == class_rep::lua_class)
			m_classrep->bases().front().base->adopt(m_flags & constant, m_object);
	}

	void object_rep::set_destructor(void(*ptr)(void*))
	{
		m_destructor = ptr;
	}

	void object_rep::add_dependency(lua_State* L, int index)
	{
		if (!m_dependency_ref.is_valid())
		{
			lua_newtable(L);
			m_dependency_ref.set(L);
		}

		m_dependency_ref.get(L);
		lua_pushvalue(L, index);
		lua_rawseti(L, -2, m_dependency_cnt);
		lua_pop(L, 1);
		++m_dependency_cnt;
	}

	int object_rep::garbage_collector(lua_State* L)
	{
		object_rep* obj = static_cast<object_rep*>(lua_touserdata(L, -1));

		finalize(L, obj->crep());

		obj->~object_rep();
		return 0;
	}

}}

