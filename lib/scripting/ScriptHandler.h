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
	struct LoadedScript
	{
		ScriptTypeDescription description;
		/// combat event scripts are stateless and shared between every unit running them,
		/// so the single instance is created on load and handed out as is
		std::shared_ptr<ICombatEventScript> combatEventScript;
	};

public:
	std::shared_ptr<ICombatEventScript> getCombatEventScript(ScriptID scriptID) const override;
	std::shared_ptr<spells::effects::Effect> createSpellEffect(ScriptID scriptID) const override;

	std::string getJsonKey(ScriptID scriptID) const override;
	std::string getDescriptionTextID(ScriptID scriptID) const override;
	int getPriority(ScriptID scriptID) const override;
	ScriptKind getKind(ScriptID scriptID) const override;

	void prepareParameters(ScriptID scriptID, JsonNode & parameters, const TextIdentifier & owner) const override;

	void registerFactory(const std::string & backend, std::shared_ptr<IScriptFactory> factory) override;

	std::vector<JsonNode> loadLegacyData() override;

	/// loads single object into game. Scope is namespace of this object, same as name of source mod
	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

private:
	/// Returns null on an identifier that is not set, throws on one that is set but out of range,
	/// which can only mean the value itself is corrupt.
	const LoadedScript * find(ScriptID scriptID) const;

	std::unordered_map<std::string, std::shared_ptr<IScriptFactory>> backends;
	std::vector<LoadedScript> scripts;
};
