/*
 * IScriptVariablesHost.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

class ScriptVariablesStorage;

/// Interface for objects that own a ScriptVariablesStorage.
/// The Lua bindings target this interface so a single binding-group works for every host type.
class DLL_LINKAGE IScriptVariablesHost
{
public:
	virtual ~IScriptVariablesHost() = default;

	virtual ScriptVariablesStorage & getScriptVariables() = 0;
	virtual const ScriptVariablesStorage & getScriptVariables() const = 0;
};
