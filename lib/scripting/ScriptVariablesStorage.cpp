/*
 * ScriptVariablesStorage.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "ScriptVariablesStorage.h"

const JsonNode & ScriptVariablesStorage::get(const std::string & scope, const std::string & name) const
{
	static const JsonNode nullNode;

	auto scopeIt = variables.find(scope);
	if(scopeIt == variables.end())
		return nullNode;

	auto valueIt = scopeIt->second.find(name);
	if(valueIt == scopeIt->second.end())
		return nullNode;

	return valueIt->second;
}

void ScriptVariablesStorage::set(const std::string & scope, const std::string & name, JsonNode value)
{
	variables[scope][name] = std::move(value);
}

bool ScriptVariablesStorage::has(const std::string & scope, const std::string & name) const
{
	auto scopeIt = variables.find(scope);
	return scopeIt != variables.end() && scopeIt->second.count(name) != 0;
}

void ScriptVariablesStorage::erase(const std::string & scope, const std::string & name)
{
	auto scopeIt = variables.find(scope);
	if(scopeIt != variables.end())
		scopeIt->second.erase(name);
}
