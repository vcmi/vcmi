/*
 * LuaExpressionEvaluator.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <lua.hpp>
#include <boost/noncopyable.hpp>

VCMI_LIB_NAMESPACE_BEGIN

class LuaExpressionEvaluator : boost::noncopyable
{
	static std::mutex mutex;
	lua_State * luaState = nullptr;
	const std::string expression;
	int compiledReference = -1;

	void compile();
	void registerLibrary();

public:
	LuaExpressionEvaluator(const std::string & expression);
	~LuaExpressionEvaluator();

	double evaluate(const std::map<std::string, double> & param);
};

VCMI_LIB_NAMESPACE_END
