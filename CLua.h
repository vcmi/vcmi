#include "global.h"
#include "lstate.h"
class CLua;
class CObjectScript
{
public:
	int owner;
	int getOwner(){return owner;} //255 - neutral /  254 - not flaggable
	CObjectScript();
	virtual ~CObjectScript();
};
class CScript
{
public:
	CScript();
	virtual ~CScript();
};

class CLua :public CScript
{
	lua_State * is; /// tez niebezpieczne!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! (ale chwilowo okielznane)
	bool opened;
public:
	CLua(std::string initpath);
	void registerCLuaCallback();
	CLua();
	virtual ~CLua();
};

class CLuaObjectScript : public CLua, public CObjectScript
{
public:
	CLuaObjectScript();
	virtual ~CLuaObjectScript();

};