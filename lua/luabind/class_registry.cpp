// Copyright (c) 2004 Daniel Wallin

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
#include <luabind/detail/class_registry.hpp>
#include <luabind/detail/class_rep.hpp>
#include <luabind/detail/operator_id.hpp>

namespace luabind { namespace detail {

    namespace {

        void add_operator_to_metatable(lua_State* L, int op_index)
        {
            lua_pushstring(L, get_operator_name(op_index));
            lua_pushstring(L, get_operator_name(op_index));
            lua_pushboolean(L, op_index == op_unm);
            lua_pushcclosure(L, &class_rep::operator_dispatcher, 2);
            lua_settable(L, -3);
        }

        int create_cpp_class_metatable(lua_State* L)
        {
            lua_newtable(L);

            // mark the table with our (hopefully) unique tag
            // that says that the user data that has this
            // metatable is a class_rep
            lua_pushstring(L, "__luabind_classrep");
            lua_pushboolean(L, 1);
            lua_rawset(L, -3);

            lua_pushstring(L, "__gc");
            lua_pushcclosure(
                L
              , &garbage_collector_s<
                    detail::class_rep
                >::apply
                , 0);

            lua_rawset(L, -3);

            lua_pushstring(L, "__call");
            lua_pushcclosure(L, &class_rep::constructor_dispatcher, 0);
            lua_rawset(L, -3);

            lua_pushstring(L, "__index");
            lua_pushcclosure(L, &class_rep::static_class_gettable, 0);
            lua_rawset(L, -3);

            lua_pushstring(L, "__newindex");
            lua_pushcclosure(L, &class_rep::lua_settable_dispatcher, 0);
            lua_rawset(L, -3);

            return detail::ref(L);
        }

        int create_cpp_instance_metatable(lua_State* L)
        {
            lua_newtable(L);

            // just indicate that this really is a class and not just 
            // any user data
            lua_pushstring(L, "__luabind_class");
            lua_pushboolean(L, 1);
            lua_rawset(L, -3);

            // __index and __newindex will simply be references to the 
            // class_rep which in turn has it's own metamethods for __index
            // and __newindex
            lua_pushstring(L, "__index");
            lua_pushcclosure(L, &class_rep::gettable_dispatcher, 0);
            lua_rawset(L, -3);

            lua_pushstring(L, "__newindex");
            lua_pushcclosure(L, &class_rep::settable_dispatcher, 0);
            lua_rawset(L, -3);

            lua_pushstring(L, "__gc");

            lua_pushcclosure(L, detail::object_rep::garbage_collector, 0);
            lua_rawset(L, -3);

            lua_pushstring(L, "__gettable");
            lua_pushcclosure(L, &class_rep::static_class_gettable, 0);
            lua_rawset(L, -3);

            for (int i = 0; i < number_of_operators; ++i) 
                add_operator_to_metatable(L, i);

            // store a reference to the instance-metatable in our class_rep
            assert((lua_type(L, -1) == LUA_TTABLE) 
                && "internal error, please report");

            return detail::ref(L);
        }

        int create_lua_class_metatable(lua_State* L)
        {
            lua_newtable(L);

            lua_pushstring(L, "__luabind_classrep");
            lua_pushboolean(L, 1);
            lua_rawset(L, -3);

            lua_pushstring(L, "__gc");
            lua_pushcclosure(
                L
              , &detail::garbage_collector_s<
                    detail::class_rep
                >::apply
                , 0);

            lua_rawset(L, -3);

            lua_pushstring(L, "__newindex");
            lua_pushcclosure(L, &class_rep::lua_settable_dispatcher, 0);
            lua_rawset(L, -3);

            lua_pushstring(L, "__call");
            lua_pushcclosure(L, &class_rep::construct_lua_class_callback, 0);
            lua_rawset(L, -3);

            lua_pushstring(L, "__index");
            lua_pushcclosure(L, &class_rep::static_class_gettable, 0);
            lua_rawset(L, -3);

            return detail::ref(L);
        }

        int create_lua_instance_metatable(lua_State* L)
        {
            lua_newtable(L);

            // just indicate that this really is a class and not just 
            // any user data
            lua_pushstring(L, "__luabind_class");
            lua_pushboolean(L, 1);
            lua_rawset(L, -3);

            lua_pushstring(L, "__index");
            lua_pushcclosure(L, &class_rep::lua_class_gettable, 0);
            lua_rawset(L, -3);

            lua_pushstring(L, "__newindex");
            lua_pushcclosure(L, &class_rep::lua_class_settable, 0);
            lua_rawset(L, -3);

            lua_pushstring(L, "__gc");
            lua_pushcclosure(L, detail::object_rep::garbage_collector, 0);
            lua_rawset(L, -3);

            for (int i = 0; i < number_of_operators; ++i) 
                add_operator_to_metatable(L, i);

            // store a reference to the instance-metatable in our class_rep
            return detail::ref(L);
        }

        int create_lua_function_metatable(lua_State* L)
        {
            lua_newtable(L);

            lua_pushstring(L, "__gc");
            lua_pushcclosure(
                L 
              , detail::garbage_collector_s<
                    detail::free_functions::function_rep
                >::apply
              , 0);

            lua_rawset(L, -3);

            return detail::ref(L);
        }

    } // namespace unnamed

    class class_rep;

    class_registry::class_registry(lua_State* L)
        : m_cpp_instance_metatable(create_cpp_instance_metatable(L))
        , m_cpp_class_metatable(create_cpp_class_metatable(L))
        , m_lua_instance_metatable(create_lua_instance_metatable(L))
        , m_lua_class_metatable(create_lua_class_metatable(L))
        , m_lua_function_metatable(create_lua_function_metatable(L))
    {
    }

    class_registry* class_registry::get_registry(lua_State* L)
    {

#ifdef LUABIND_NOT_THREADSAFE

        // if we don't have to be thread safe, we can keep a
        // chache of the class_registry pointer without the
        // need of a mutex
        static lua_State* cache_key = 0;
        static class_registry* registry_cache = 0;
        if (cache_key == L) return registry_cache;

#endif

        lua_pushstring(L, "__luabind_classes");
        lua_gettable(L, LUA_REGISTRYINDEX);
        class_registry* p = static_cast<class_registry*>(lua_touserdata(L, -1));
        lua_pop(L, 1);

#ifdef LUABIND_NOT_THREADSAFE

        cache_key = L;
        registry_cache = p;

#endif

        return p;
    }

    void class_registry::add_class(LUABIND_TYPE_INFO info, class_rep* crep)
    {
        // class is already registered
        assert((m_classes.find(info) == m_classes.end()) 
            && "you are trying to register a class twice");
        m_classes[info] = crep;
    }

    class_rep* class_registry::find_class(LUABIND_TYPE_INFO info) const
    {
        std::map<LUABIND_TYPE_INFO, class_rep*, cmp>::const_iterator i(
            m_classes.find(info));

        if (i == m_classes.end()) return 0; // the type is not registered
        return i->second;
    }

}} // namespace luabind::detail

