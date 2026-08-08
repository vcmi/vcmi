/*
 * CombatScriptService.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../constants/EntityIdentifiers.h"

class ICombatEventScript;

class DLL_LINKAGE ICombatScriptFactory
{
public:
	/// (modScope, sourcePath) pair identifying one patch script layer.
	using PatchEntry = std::pair<std::string, std::string>;

	virtual ~ICombatScriptFactory() = default;

	/// scriptId is the unique identifier of the script (e.g. "core:spikes"), used as cache key.
	/// scope/name point at the base script; patches list extra layers stacked over the base in order.
	virtual void initialize(const std::string & scriptId, const std::string & scope, const std::string & name, const std::vector<PatchEntry> & patches) = 0;

	virtual std::shared_ptr<ICombatEventScript> get(const std::string & scriptId) const = 0;
};

class DLL_LINKAGE CombatScriptService : boost::noncopyable
{
public:
	virtual ~CombatScriptService() = default;

	/// Returns null if no script is set. Throws on an identifier that is set but out of range,
	/// which can only mean the value itself is corrupt.
	virtual std::shared_ptr<ICombatEventScript> get(CombatScriptID scriptID) const = 0;

	/// Text ID of the description shown for a bonus that runs this script. Empty if none is set.
	virtual std::string getDescriptionTextID(CombatScriptID scriptID) const = 0;

	/// Scoped identifier of the script, e.g. "core:lifeDrain". Empty if no script is set.
	virtual std::string getJsonKey(CombatScriptID scriptID) const = 0;

	/// Order in which scripts reacting to the same event run, from lowest to highest. Zero when
	/// the script declares none, which is what a script that does not care about ordering wants.
	virtual int getPriority(CombatScriptID scriptID) const = 0;

	virtual void registerFactory(const std::string & typeName, std::shared_ptr<ICombatScriptFactory> factory) = 0;
};
