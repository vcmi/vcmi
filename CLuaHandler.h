#ifndef CLUAHANDLER_H
#define CLUAHANDLER_H
#include "global.h"
#if (LUA_VERSION_NUM < 500)
#  define LUA_OPEN_LIB(L, lib) lib(L)
#else
#  define LUA_OPEN_LIB(L, lib) \
     lua_pushcfunction((L), lib); \
     lua_pcall((L), 0, 0, 0);
#endif
class CLuaHandler
{
public:
	CLuaHandler();

	static std::vector<std::string> * searchForScripts(std::string fol);
	static std::vector<std::string> * functionList(std::string file);

	~CLuaHandler();

	void test();
};
#endif //CLUAHANDLER_H