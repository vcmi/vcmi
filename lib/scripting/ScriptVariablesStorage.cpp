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
	return variables[scope][name];
}

void ScriptVariablesStorage::set(const std::string & scope, const std::string & name, JsonNode value)
{
	variables[scope][name] = std::move(value);
}

bool ScriptVariablesStorage::has(const std::string & scope, const std::string & name) const
{
	return !get(scope, name).isNull();
}
