/*
 * LuaScriptingContext.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "LuaWrapper.h"
#include "LuaReference.h"

#include "../../lib/ScriptHandler.h"
#include "../../lib/CScriptingModule.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{

class LuaContext : public ContextBase
{
public:
	static const std::string STATE_FIELD;

	LuaContext(const Script * source, const Environment * env_);
	virtual ~LuaContext();

	void run(const JsonNode & initialState) override;
	void run(ServerCallback * server, const JsonNode & initialState) override;

	//log error and return nil from LuaCFunction
	int errorRetVoid(const std::string & message);

	JsonNode callGlobal(const std::string & name, const JsonNode & parameters) override;
	JsonNode callGlobal(ServerCallback * cb, const std::string & name, const JsonNode & parameters) override;

	void getGlobal(const std::string & name, int & value) override;
	void getGlobal(const std::string & name, std::string & value) override;
	void getGlobal(const std::string & name, double & value) override;
	void getGlobal(const std::string & name, JsonNode & value) override;

	void setGlobal(const std::string & name, int value) override;
	void setGlobal(const std::string & name, const std::string & value) override;
	void setGlobal(const std::string & name, double value) override;
	void setGlobal(const std::string & name, const JsonNode & value) override;

	JsonNode saveState() override;

	void pop(JsonNode & value);

	void popAll();

	void push(const std::string & value);
	void push(lua_CFunction f, void * opaque);

	std::string toStringRaw(int index);

private:
	lua_State * L;

	const Script * script;

	const Environment * env;

	std::shared_ptr<LuaReference> modules;
	std::shared_ptr<LuaReference> scriptClosure;

	void cleanupGlobals();

	void registerCore();

	//require global function
	static int require(lua_State * L);

	//print global function
	static int print(lua_State * L);

	static int logError(lua_State * L);

	//require function implementation
	int loadModule();

	int printImpl();

	int logErrorImpl();
};

}

VCMI_LIB_NAMESPACE_END
