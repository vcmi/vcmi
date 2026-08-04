/*
 * ScriptVariablesStorage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../json/JsonNode.h"

/// Definition of a script variable: its name, initial value and campaign carry-over flags.
/// Metadata only - live values are held in ScriptVariablesStorage.
struct DLL_LINKAGE ScriptVariableDefinition
{
	std::string name;
	JsonNode initialValue;
	bool persistInCampaign = false; ///< value is carried into the campaign state when the scenario is won
	bool importFromPreviousScenario = false; ///< value is seeded from the campaign state at scenario start

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & name;
		h & initialValue;
		h & persistInCampaign;
		h & importFromPreviousScenario;
	}
};

/// Persistent key/value store for script-authored variables. Values are JsonNode so any Lua
/// value round-trips and serializes natively. Keys are namespaced by mod scope so scripts from
/// different mods cannot clash on a variable name.
class DLL_LINKAGE ScriptVariablesStorage
{
	JsonNode variables; // scope -> name -> value

public:
	/// Returns the stored value, or a null JsonNode when the variable is unset.
	const JsonNode & get(const std::string & scope, const std::string & name) const;
	void set(const std::string & scope, const std::string & name, JsonNode value);
	bool has(const std::string & scope, const std::string & name) const;

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & variables;
	}
};
