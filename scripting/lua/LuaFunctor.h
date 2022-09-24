/*
 * LuaFunctor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{

template <typename T>
class LuaFunctor
{
public:
	virtual int operator() (lua_State *, T *) = 0;
};

}

VCMI_LIB_NAMESPACE_END
