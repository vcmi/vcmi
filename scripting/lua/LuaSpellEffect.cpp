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

void LuaSpellEffect::adjustAffectedHexes(std::set<BattleHex> & hexes, const Mechanics * m, const Target & spellTarget) const
{

}

bool LuaSpellEffect::applicable(Problem & problem, const Mechanics * m) const
{
	std::shared_ptr<scripting::Context> context = resolveScript(m);
	if(!context)
		return false;

	setContextVariables(m, context);

	JsonNode response = context->callGlobal(APPLICABLE_GENERAL, JsonNode());

	if(response.getType() != JsonNode::JsonType::DATA_BOOL)
	{
		logMod->error("Invalid API response from script %s.", script->getName());
		logMod->debug(response.toJson(true));
		return false;
	}
	return response.Bool();
}

bool LuaSpellEffect::applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const
{
	std::shared_ptr<scripting::Context> context = resolveScript(m);
	if(!context)
		return false;

	setContextVariables(m, context);

	JsonNode requestP;

	if(target.empty())
		return false;

	for(const auto & dest : target)
	{
		JsonNode targetData;
		targetData.Vector().push_back(JsonUtils::intNode(dest.hexValue.hex));

		if(dest.unitValue)
			targetData.Vector().push_back(JsonUtils::intNode(dest.unitValue->unitId()));
		else
			targetData.Vector().push_back(JsonUtils::intNode(-1));

		requestP.Vector().push_back(targetData);
	}

	JsonNode request;
	request.Vector().push_back(requestP);

	JsonNode response = context->callGlobal(APPLICABLE_TARGET, request);

	if(response.getType() != JsonNode::JsonType::DATA_BOOL)
	{
		logMod->error("Invalid API response from script %s.", script->getName());
		logMod->debug(response.toJson(true));
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
		return;
	}

	setContextVariables(m, context);

	JsonNode requestP;

	for(const auto & dest : target)
	{
		JsonNode targetData;
		targetData.Vector().push_back(JsonUtils::intNode(dest.hexValue.hex));

		if(dest.unitValue)
			targetData.Vector().push_back(JsonUtils::intNode(dest.unitValue->unitId()));
		else
			targetData.Vector().push_back(JsonUtils::intNode(-1));

		requestP.Vector().push_back(targetData);
	}

	JsonNode request;
	request.Vector().push_back(requestP);

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
	//TODO: load everything and provide to script
}

std::shared_ptr<Context> LuaSpellEffect::resolveScript(const Mechanics * m) const
{
	return m->battle()->getContextPool()->getContext(script);
}

void LuaSpellEffect::setContextVariables(const Mechanics * m, const std::shared_ptr<Context>& context) 
{
	context->setGlobal("effectLevel", m->getEffectLevel());
	context->setGlobal("effectRangeLevel", m->getRangeLevel());
	context->setGlobal("effectPower", m->getEffectPower());
	context->setGlobal("effectDuration", m->getEffectDuration());
	context->setGlobal("effectValue", static_cast<int>(m->getEffectValue()));
}
}
}

VCMI_LIB_NAMESPACE_END
