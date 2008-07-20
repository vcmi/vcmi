#include "stdafx.h"

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h" 
}
//#include <luabind/luabind.hpp>
//#include <luabind/function.hpp>
//#include <luabind/class.hpp>
#include "CLuaHandler.h"
#include "boost/filesystem.hpp"
#include <boost/algorithm/string.hpp>
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

	//luabind::open(lua);
	//luabind::module(lua)
	//[
	//	luabind::class_<int3>("int3")
	//		//.def(luabind::constructor<>())
	//		//.def(luabind::constructor<const int&,const int&,const int&>())
	//		.def_readwrite("x", &int3::x)
	//		.def_readwrite("y", &int3::y)
	//		.def_readwrite("z", &int3::z)
	//];
	//luabind::module(lua)
	//[
	//	luabind::def("powitanie",&piszpowitanie2)
	//];


	if ((iErr = luaL_loadfile (lua, "scripts/lua/objects/0023_marletto_tower.lua")) == 0)
	{
	   // Call main...
	   if ((iErr = lua_pcall (lua, 0, LUA_MULTRET, 0)) == 0)
	   {

			//int ret = luabind::call_function<int>(lua, "helloWorld2");
			//lua_pushstring (lua, "helloWorld2");
			//lua_gettable (lua, LUA_GLOBALSINDEX);
			//lua_pcall (lua, 0, 0, 0);

			// Push the function name onto the stack
			lua_pushstring (lua, "rightText");
			lua_gettable (lua, LUA_GLOBALSINDEX);
			lua_pcall (lua, 0, 0, 0);
		}
	}
	lua_close (lua);
}


std::vector<std::string> * CLuaHandler::searchForScripts(std::string fol)
{
	std::vector<std::string> * ret = new std::vector<std::string> ();
	boost::filesystem::path folder(fol);
	if (!boost::filesystem::exists(folder))
#ifndef __GNUC__
		throw new std::exception("No such folder!");
#else
		throw std::exception();
#endif
	boost::filesystem::directory_iterator end_itr;
	for
	  (
	  boost::filesystem::directory_iterator it(folder);
	  it!=end_itr;
	  it++
	  )
	{
		if(boost::algorithm::ends_with((it->path().leaf()),".lua"))
		{
			ret->push_back(fol+"/"+(it->path().leaf()));
		}
	}
	return ret;
}
std::vector<std::string> * CLuaHandler::functionList(std::string file)
{
	std::vector<std::string> * ret = new std::vector<std::string> ();
	char linia[500];
	std::ifstream is(file.c_str());
	while (!is.eof())
	{
		is.getline(linia,500);
		std::string ss(linia);
		boost::algorithm::trim_left(ss);
		if (boost::algorithm::starts_with(ss,"local"))
			boost::algorithm::erase_first(ss,"local ");
		if (boost::algorithm::starts_with(ss,"function"))
		{
			boost::algorithm::erase_first(ss,"function ");
			int ps = ss.find_first_of(' ');
			int op = ss.find_first_of('(');
			if (ps<0)
				ps = ss.length()-1;
			if (op<0)
				op = ss.length()-1;
			ps = std::min(ps,op);
			ret->push_back(ss.substr(0,ps));
		}
	}
	is.close();
	return ret;
}
