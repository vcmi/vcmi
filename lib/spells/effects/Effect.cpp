/*
 * Effect.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Effect.h"
#include "Registry.h"

#include "../../serializer/JsonSerializeFormat.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

Effect::Effect()
	: indirect(false),
	optional(false)
{

}

bool Effect::applicable(Problem & problem, const Mechanics * m) const
{
	return true;
}

bool Effect::applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const
{
	return true;
}

void Effect::serializeJson(JsonSerializeFormat & handler)
{
	handler.serializeBool("indirect", indirect, false);
	handler.serializeBool("optional", optional, false);
	serializeJsonEffect(handler);
}

std::shared_ptr<Effect> Effect::create(const Registry * registry, const std::string & type)
{
	auto factory = registry->find(type);

	if(factory)
	{
		std::shared_ptr<Effect> ret;
		ret.reset(factory->create());
		return ret;
	}
	else
	{
		logGlobal->error("Unknown effect type '%s'", type);
		return {};
	}
}

}
}

VCMI_LIB_NAMESPACE_END
