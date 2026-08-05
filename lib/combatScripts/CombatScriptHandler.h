/*
 * CombatScriptHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CombatScriptService.h"
#include "../IHandlerBase.h"

struct CombatScriptType
{
	std::string identifier;
	std::string modScope;
	std::string scriptId; ///< modScope + ':' + identifier
	std::string type;
	std::string scriptName;
	std::vector<ICombatScriptFactory::PatchEntry> patches; ///< vector(modScope, sourcePath)
};

class CombatScriptHandler final : public IHandlerBase, public CombatScriptService
{
public:
	std::shared_ptr<ICombatEventScript> get(CombatScriptID scriptID) const override;

	void registerFactory(const std::string & typeName, std::shared_ptr<ICombatScriptFactory> factory) override;

	std::vector<JsonNode> loadLegacyData() override;

	/// loads single object into game. Scope is namespace of this object, same as name of source mod
	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

private:
	std::unordered_map<std::string, std::shared_ptr<ICombatScriptFactory>> scriptTypeFactories;
	std::vector<CombatScriptType> scriptTypes;
};
