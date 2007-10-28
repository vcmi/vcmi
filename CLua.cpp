#include "stdafx.h"
#include "CLua.h"
#include "CLuaHandler.h"
#include "lualib.h"
#include "lauxlib.h"

CObjectScript::CObjectScript()
{
	//std::cout << "Tworze obiekt objectscript "<<this<<std::endl;
}

CObjectScript::~CObjectScript()
{
	//std::cout << "Usuwam obiekt objectscript "<<this<<std::endl;
}

CScript::CScript()
{
	//std::cout << "Tworze obiekt CScript "<<this<<std::endl;
}
CScript::~CScript()
{
	//std::cout << "Usuwam obiekt CScript "<<this<<std::endl;
}

#define LST (is)
CLua::CLua(std::string initpath)
{
	opened=false;
	LST = lua_open();
	opened = true;
	LUA_OPEN_LIB(LST, luaopen_base);
	LUA_OPEN_LIB(LST, luaopen_io);
	if ((luaL_loadfile (LST, "test.lua")) == 0)
	{
		//lua_pcall (LST, 0, LUA_MULTRET, 0);
	}
	else
	{
		std::string temp = "Cannot open script";
		temp += initpath;
		throw std::exception(temp.c_str());
	}
}
CLua::CLua()
{
	//std::cout << "Tworze obiekt clua "<<this<<std::endl;
	opened=false;
}
void CLua::registerCLuaCallback()
{
}

CLua::~CLua()
{
	//std::cout << "Usuwam obiekt clua "<<this<<std::endl;
	if (opened)
	{
		std::cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"<<std::endl;
		lua_close(LST);
	}
}
#undef LST

CLuaObjectScript::CLuaObjectScript()
{
	//std::cout << "Tworze obiekt CLuaObjectScript "<<this<<std::endl;
}
CLuaObjectScript::~CLuaObjectScript()
{
	//std::cout << "Usuwam obiekt CLuaObjectScript "<<this<<std::endl;
}