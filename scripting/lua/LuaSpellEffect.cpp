/*
 * LuaSpellEffect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LuaSpellEffect.h"

#include <vcmi/scripting/Service.h>

#include "../../lib/spells/effects/Registry.h"
#include "../../lib/spells/ISpellMechanics.h"

#include "../../lib/battle/Unit.h"
#include "../../lib/battle/CBattleInfoCallback.h"
#include "../../lib/json/JsonUtils.h"
#include "../../lib/serializer/JsonSerializeFormat.h"

static const std::string APPLICABLE_GENERAL = "applicable";
static const std::string APPLICABLE_TARGET = "applicableTarget";
static const std::string APPLY = "apply";

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

LuaSpellEffectFactory::LuaSpellEffectFactory(const Script * script_)
	: script(script_)
{

}

LuaSpellEffectFactory::~LuaSpellEffectFactory() = default;

Effect * LuaSpellEffectFactory::create() const
{
	return new LuaSpellEffect(script);
}

LuaSpellEffect::LuaSpellEffect(const Script * script_)
	: script(script_)
{

}

LuaSpellEffect::~LuaSpellEffect() = default;

void LuaSpellEffect::adjustTargetTypes(std::vector<TargetType> & types) const
{

}

void LuaSpellEffect::adjustAffectedHexes(BattleHexArray & hexes, const Mechanics * m, const Target & spellTarget) const
{

}

bool LuaSpellEffect::applicable(Problem & problem, const Mechanics * m) const
{
	std::shared_ptr<scripting::Context> context = resolveScript(m);
	if(!context)
		throw std::runtime_error("Failed to execute Lua script effect! Context not available!");

	setContextVariables(m, context);

	JsonNode response = context->callGlobal(APPLICABLE_GENERAL, JsonNode());

	if(response.getType() != JsonNode::JsonType::DATA_BOOL)
	{
		logMod->error("Invalid API response from script %s.", script->getJsonKey());
		logMod->debug(response.toCompactString());
		return false;
	}
	return response.Bool();
}

bool LuaSpellEffect::applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const
{
	std::shared_ptr<scripting::Context> context = resolveScript(m);
	if(!context)
		throw std::runtime_error("Failed to execute Lua script effect! Context not available!");

	setContextVariables(m, context);

	JsonNode requestP;

	if(target.empty())
		return false;

	for(const auto & dest : target)
	{
		JsonNode targetData;
		targetData.Vector().emplace_back(dest.hexValue.toInt());

		if(dest.unitValue)
			targetData.Vector().emplace_back(dest.unitValue->unitId());
		else
			targetData.Vector().emplace_back(-1);

		requestP.Vector().push_back(targetData);
	}

	JsonNode request;
	request.Vector().push_back(requestP);

	JsonNode response = context->callGlobal(APPLICABLE_TARGET, request);

	if(response.getType() != JsonNode::JsonType::DATA_BOOL)
	{
		logMod->error("Invalid API response from script %s.", script->getJsonKey());
		logMod->debug(response.toCompactString());
		return false;
	}
	return response.Bool();
}

void LuaSpellEffect::apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const
{
	if(target.empty())
		return;

	std::shared_ptr<scripting::Context> context = resolveScript(m);
	if(!context)
	{
		server->complain("Unable to create scripting context");
		throw std::runtime_error("Failed to execute Lua script effect! Context not available!");
	}

	setContextVariables(m, context);

	JsonNode request;
	request.Vector().push_back(spellTargetToJson(target));

	context->callGlobal(server, APPLY, request);
}

EffectTarget LuaSpellEffect::filterTarget(const Mechanics * m, const EffectTarget & target) const
{
	return EffectTarget(target);
}

EffectTarget LuaSpellEffect::transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const
{
	return EffectTarget(spellTarget);
}

void LuaSpellEffect::serializeJsonEffect(JsonSerializeFormat & handler)
{
	parameters = handler.getCurrent();
	//TODO: load everything and provide to script
}

std::shared_ptr<Context> LuaSpellEffect::resolveScript(const Mechanics * m) const
{
	return m->battle()->getContextPool()->getContext(script);
}

void LuaSpellEffect::setContextVariables(const Mechanics * m, const std::shared_ptr<Context>& context)  const
{
	context->setGlobal("parameters", parameters);
}

JsonNode LuaSpellEffect::spellTargetToJson(const Target & target) const
{
	JsonNode requestP;

	for(const auto & dest : target)
	{
		JsonNode targetData;
		targetData.Vector().emplace_back(dest.hexValue.toInt());

		if(dest.unitValue)
			targetData.Vector().emplace_back(dest.unitValue->unitId());
		else
			targetData.Vector().emplace_back(-1);

		requestP.Vector().push_back(targetData);
	}

	return requestP;
}

Target LuaSpellEffect::spellTargetFromJson(const Mechanics * m, const JsonNode & config) const
{
	Target result;

	for (const auto & entry : config.Vector())
	{
		Destination dest;

		if (!entry[1].isNull() && entry[1].Integer() != -1 )
			dest = Destination(m->battle()->battleGetUnitByID(entry[1].Integer()), BattleHex(entry[0].Integer()));
		else
			dest = Destination(BattleHex(entry[0].Integer()));

		result.push_back(dest);
	}

	return result;
}

}
}

VCMI_LIB_NAMESPACE_END
