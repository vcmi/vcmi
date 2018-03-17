/*
 * ScriptHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/Service.h>
#include "IHandlerBase.h"
#include "JsonNode.h"

class JsonNode;
class JsonSerializeFormat;
class Services;

namespace scripting
{
class Module;
class ScriptImpl;
class ScriptHandler;

using ModulePtr = std::shared_ptr<Module>;
using ScriptPtr = std::shared_ptr<ScriptImpl>;
using ScriptMap = std::map<std::string, ScriptPtr>;

class DLL_LINKAGE ScriptImpl : public Script
{
public:
	enum class Implements
	{
		ANYTHING,
		BATTLE_EFFECT
		//todo: adventure effect, map object(unified with building), server query, client query(with gui), may be smth else
	};

	std::string identifier;
	std::string sourcePath;
	std::string sourceText;

	std::string code;

	ModulePtr host;

	ScriptImpl(const ScriptHandler * owner_);
	virtual ~ScriptImpl();

	void compile();

	void serializeJson(JsonSerializeFormat & handler);
	void serializeJsonState(JsonSerializeFormat & handler);

	std::shared_ptr<Context> createContext(const Environment * env) const override;
	const std::string & getName() const override;
	const std::string & getSource() const override;

	void performRegistration(::Services * services) const;
private:
	const ScriptHandler * owner;
	Implements implements;

	void resolveHost();
};

class DLL_LINKAGE PoolImpl : public Pool
{
public:
	PoolImpl(const Environment * ENV);
	std::shared_ptr<Context> getContext(const Script * script) override;

	void serializeState(const bool saving, JsonNode & data) override;
private:
	std::map<const Script *, std::shared_ptr<Context>> cache;

	JsonNode state;

	const Environment * env;
};

class DLL_LINKAGE ScriptHandler : public ::IHandlerBase, public Service
{
public:
	ScriptHandler();
	virtual ~ScriptHandler();

	const Script * resolveScript(const std::string & name) const;

	std::vector<bool> getDefaultAllowed() const override;
	std::vector<JsonNode> loadLegacyData(size_t dataSize) override;

	ScriptPtr loadFromJson(const JsonNode & json, const std::string & identifier) const;

	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	void performRegistration(Services * services) const override;

	template <typename Handler> void serialize(Handler & h, const int version)
	{
		JsonNode state;
		if(h.saving)
			saveState(state);

		h & state;

		if(!h.saving)
			loadState(state);
	}

	ModulePtr erm;
	ModulePtr lua;

protected:

private:
	friend class ScriptImpl;

	ScriptMap objects;

	void loadState(const JsonNode & state);
	void saveState(JsonNode & state);
};

}
