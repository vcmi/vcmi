/*
 * ScriptService.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "ScriptTypeDescription.h"

#include "../constants/EntityIdentifiers.h"

class ICombatEventScript;
class JsonNode;
class TextIdentifier;

namespace spells::effects
{
class Effect;
}

/// Loads script sources and turns them into the objects the engine calls.
/// One factory for the whole game, shared by every script kind.
class DLL_LINKAGE IScriptFactory
{
public:
	virtual ~IScriptFactory() = default;

	/// Loads the base source and any patch layers, keeping them under description.scriptId.
	virtual void initialize(const ScriptTypeDescription & description) = 0;

	/// One creator per script kind. A factory that can not express a kind inherits the null
	/// default, which the handler reports as a load error rather than passing on.
	virtual std::shared_ptr<ICombatEventScript> createCombatEventScript(const std::string & scriptId) const { return nullptr; }
	virtual std::shared_ptr<spells::effects::Effect> createSpellEffect(const std::string & scriptId) const { return nullptr; }
};

class DLL_LINKAGE ScriptService : boost::noncopyable
{
public:
	virtual ~ScriptService() = default;

	/// Shared, stateless handler for a combat event script. Null if no script is set, or if the
	/// script the id names is of another kind.
	virtual std::shared_ptr<ICombatEventScript> getCombatEventScript(ScriptID scriptID) const = 0;

	/// Fresh spell effect instance - the caller initializes it with its own parameters.
	/// Null if no script is set, or if the script the id names is of another kind.
	virtual std::shared_ptr<spells::effects::Effect> createSpellEffect(ScriptID scriptID) const = 0;

	/// Scoped identifier of the script, e.g. "core:lifeDrain". Empty if no script is set.
	virtual std::string getJsonKey(ScriptID scriptID) const = 0;

	/// Text ID of the description shown to the player. Empty if the script declares none.
	virtual std::string getDescriptionTextID(ScriptID scriptID) const = 0;

	/// Order in which scripts reacting to the same event run, from lowest to highest. Zero when
	/// the script declares none, which is what a script that does not care about ordering wants.
	virtual int getPriority(ScriptID scriptID) const = 0;

	/// Which engine interface this script implements. INVALID if no script is set.
	virtual ScriptKind getKind(ScriptID scriptID) const = 0;

	/// Validates the parameters of a single instance of this script against the schema it
	/// declares, and turns every field it lists in stringRegistrations into a registered text ID.
	/// `owner` identifies whoever holds this instance and prefixes the generated text IDs;
	/// when it is empty only validation runs, since there is no stable key to register under.
	virtual void prepareParameters(ScriptID scriptID, JsonNode & parameters, const TextIdentifier & owner) const = 0;

	virtual void registerFactory(std::shared_ptr<IScriptFactory> factory) = 0;
};
