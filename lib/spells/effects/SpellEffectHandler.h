/*
 * SpellEffectHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Effect.h"
#include "SpellEffectService.h"
#include "../../IHandlerBase.h"
#include "../../json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells::effects
{

class BuiltinEffectFactory final : public ISpellEffectFactory
{
public:
	void initialize(const std::string & scope, const std::string & name) override;
	std::shared_ptr<Effect> create(const std::string & scope, const std::string & name) const override;
};

struct SpellEffectType
{
	std::string identifier;
	std::string modScope;
	std::string type;
	std::string scriptName;
	JsonNode validationSchema;
};

class SpellEffectHandler final : public IHandlerBase, public SpellEffectService
{
public:
	SpellEffectHandler();

	std::shared_ptr<Effect> create(SpellEffectID effectID) const override;

	void registerFactory(const std::string & typeName, std::shared_ptr<ISpellEffectFactory> factory) override;

	void validateEffect(SpellEffectID effectID, const JsonNode & data, const std::string & name) const override;

	std::vector<JsonNode> loadLegacyData() override;

	/// loads single object into game. Scope is namespace of this object, same as name of source mod
	void loadObject(std::string scope, std::string name, const JsonNode & data) override;
	void loadObject(std::string scope, std::string name, const JsonNode & data, size_t index) override;

	void afterLoadFinalization() override;

private:
	std::unordered_map<std::string, std::shared_ptr<ISpellEffectFactory>> effectTypeFactories;
	std::vector<SpellEffectType> effectTypes;

};

}

VCMI_LIB_NAMESPACE_END
