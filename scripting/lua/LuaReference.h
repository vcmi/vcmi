/*
 * LuaReference.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

namespace scripting
{

class LuaReference : public boost::noncopyable
{
public:
	//pop from the top of stack
	LuaReference(lua_State * L);

	LuaReference(LuaReference && other);
	~LuaReference();

	void push();
private:
	bool doCleanup;
	int key;
	lua_State * l;
};

}

