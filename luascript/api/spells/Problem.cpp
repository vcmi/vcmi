/*
 * Problem.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Problem.h"

#include "../../../lib/json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
using ::spells::Problem;

VCMI_REGISTER_CORE_SCRIPT_API(SpellProblemProxy, "battle.SpellProblem");

const std::vector<SpellProblemProxy::CustomRegType> SpellProblemProxy::REGISTER_CUSTOM =
{
	{"add", &SpellProblemProxy::add, false},
};

int SpellProblemProxy::add(lua_State * L)
{
	LuaStack S(L);

	Problem * object = nullptr;
	JsonNode metaStringConfig;

	if (!S.tryGetAll(1, object, metaStringConfig))
		return S.retVoid();

	MetaString metaString;
	for (const auto & entry : metaStringConfig["append"].Vector())
		metaString.appendTextID(entry.String());

	for (const auto & entry : metaStringConfig["replace"].Vector())
		metaString.replaceTextID(entry.String());

	object->add(std::move(metaString));
	return S.retVoid();
}
}
}

VCMI_LIB_NAMESPACE_END
