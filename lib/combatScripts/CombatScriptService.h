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

	/// Returns null if the identifier could not be resolved, e.g. because the mod providing it is gone.
	virtual std::shared_ptr<ICombatEventScript> get(CombatScriptID scriptID) const = 0;

	virtual void registerFactory(const std::string & typeName, std::shared_ptr<ICombatScriptFactory> factory) = 0;
};
