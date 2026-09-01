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
class IDamageCalculatorScript;
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

	/// One creator per script kind. Returning null is a load error, which the handler reports
	/// rather than passing on.
	virtual std::shared_ptr<ICombatEventScript> createCombatEventScript(const std::string & scriptId) const = 0;
	virtual std::shared_ptr<IDamageCalculatorScript> createDamageCalculatorScript(const std::string & scriptId) const = 0;
	virtual std::shared_ptr<spells::effects::Effect> createSpellEffect(const std::string & scriptId) const = 0;
};

class DLL_LINKAGE ScriptService : boost::noncopyable
{
public:
	virtual ~ScriptService() = default;

	/// Everything the game knows about one script, including the shared instance of a combat event
	/// script. Throws on an id naming no script, which can only come from content that was never
	/// validated - there is nothing sensible to hand back for it.
	virtual const ScriptTypeDescription & getById(ScriptID scriptID) const = 0;

	/// Fresh spell effect instance - the caller initializes it with its own parameters.
	virtual std::shared_ptr<spells::effects::Effect> createSpellEffect(ScriptID scriptID) const = 0;

	/// Validates the parameters of a single instance of this script against the schema it
	/// declares, and turns every field it lists in stringRegistrations into a registered text ID.
	/// `owner` identifies whoever holds this instance and prefixes the generated text IDs;
	/// when it is empty only validation runs, since there is no stable key to register under.
	virtual void prepareParameters(ScriptID scriptID, JsonNode & parameters, const TextIdentifier & owner) const = 0;

	/// The damage calculator of the game, or null when no script declares one - which leaves no rule
	/// for what an attack is worth, so whoever asks for one throws.
	virtual const IDamageCalculatorScript * getDamageCalculator() const = 0;

	virtual void registerFactory(std::shared_ptr<IScriptFactory> factory) = 0;
};
