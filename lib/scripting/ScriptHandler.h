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

#include "ScriptService.h"

#include "../IHandlerBase.h"

/// Registry of every script the game knows, of every kind. Scripts differ only in which engine
/// interface they implement, declared as `implements`, so they share one entity, one identifier
/// namespace and one set of features - patches, parameter schema, translatable strings.
class DLL_LINKAGE ScriptHandler final : public IHandlerBase, public ScriptService
{
public:
	const ScriptTypeDescription & getById(ScriptID scriptID) const override;
	std::shared_ptr<spells::effects::Effect> createSpellEffect(ScriptID scriptID) const override;
	const IDamageCalculatorScript * getDamageCalculator() const override;

	void prepareParameters(ScriptID scriptID, JsonNode & parameters, const TextIdentifier & owner) const override;

	void registerFactory(std::shared_ptr<IScriptFactory> factory) override;

	std::vector<JsonNode> loadLegacyData() override;

	/// loads single object into game. Scope is namespace of this object, same as name of source mod
	void loadObject(const std::string & scope, const std::string & name, const JsonNode & data) override;
	void loadObject(const std::string & scope, const std::string & name, const JsonNode & data, size_t index) override;

private:
	std::shared_ptr<IScriptFactory> factory;
	std::shared_ptr<IDamageCalculatorScript> damageCalculator;
	std::vector<ScriptTypeDescription> scripts;
};
