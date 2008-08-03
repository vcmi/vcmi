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

bool luabind::detail::find_best_match(
    lua_State* L
  , overload_rep_base const* start
  , int num_overloads
  , size_t orep_size
  , bool& ambiguous
  , int& min_match
  , int& match_index
  , int num_params)
{
    int min_but_one_match = std::numeric_limits<int>::max();
    bool found = false;

    for (int index = 0; index < num_overloads; ++index)
    {
        int match_value = start->match(L, num_params);

        reinterpret_cast<const char*&>(start) += orep_size;

        if (match_value < 0) continue;
        if (match_value < min_match)
        {
            found = true;
            match_index = index;
            min_but_one_match = min_match;
            min_match = match_value;
        }
        else if (match_value < min_but_one_match)
        {
            min_but_one_match = match_value;
        }
    }

    ambiguous = min_match == min_but_one_match
        && min_match < std::numeric_limits<int>::max();
    return found;
}

void luabind::detail::find_exact_match(
    lua_State* L
  , overload_rep_base const* start
  , int num_overloads
  , size_t orep_size
  , int cmp_match
  , int num_params
  , std::vector<const overload_rep_base*>& dest)
{
    for (int i = 0; i < num_overloads; ++i)
    {
        int match_value = start->match(L, num_params);
        if (match_value == cmp_match) dest.push_back(start);
        reinterpret_cast<const char*&>(start) += orep_size;
    }
}

