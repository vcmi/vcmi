#include "stdafx.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include <luabind/luabind.hpp>
#include <luabind/function.hpp>
#include <luabind/class.hpp>
#include "CLuaHandler.h"

void piszpowitanie2(std::string i) //simple global function for testing
{
	std::cout<<"powitanie2zc++. Liczba dnia to " << i;
}


CLuaHandler::CLuaHandler()
{
}
CLuaHandler::~CLuaHandler()
{
}
void CLuaHandler::test()
{
	int iErr = 0;
	lua_State *lua = lua_open ();  // Open Lua
	LUA_OPEN_LIB(lua, luaopen_base);
	LUA_OPEN_LIB(lua, luaopen_io);

	if ((iErr = luaL_loadfile (lua, "test.lua")) == 0)
	{
		

	   // Call main...
	   if ((iErr = lua_pcall (lua, 0, LUA_MULTRET, 0)) == 0)
	   {    
			luabind::open(lua);
			luabind::module(lua)
			[
				luabind::def("powitanie",&piszpowitanie2)
			];

			//int ret = luabind::call_function<int>(lua, "helloWorld2");

			lua_pushstring (lua, "helloWorld2");
			lua_gettable (lua, LUA_GLOBALSINDEX);  
			lua_pcall (lua, 0, 0, 0);

			// Push the function name onto the stack
			lua_pushstring (lua, "helloWorld");
			lua_gettable (lua, LUA_GLOBALSINDEX);  
			lua_pcall (lua, 0, 0, 0);
		}
	}
	lua_close (lua);
}