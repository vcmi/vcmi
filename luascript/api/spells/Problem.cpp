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
#include "../../../lib/spells/ISpellMechanics.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
using ::spells::Problem;
using ::spells::Mechanics;

const std::vector<SpellProblemProxy::CustomRegType> SpellProblemProxy::REGISTER_CUSTOM =
{
	{"addCustom", LuaCallWrapper<&SpellProblemProxy::addCustom>::invoke, false},
	{"addGeneric", LuaCallWrapper<&SpellProblemProxy::addGeneric>::invoke, false},
	{"addStandard", LuaCallWrapper<&SpellProblemProxy::addStandard>::invoke, false},
};

int SpellProblemProxy::addCustom(lua_State * L)
{
	LuaStack S(L);

	Problem * object = nullptr;
	JsonNode metaStringConfig;

	S.getNonNullOrThrow(1, object);
	S.getOrThrow(2, metaStringConfig);

	MetaString metaString;
	for (const auto & entry : metaStringConfig["append"].Vector())
		metaString.appendTextID(entry.String());

	for (const auto & entry : metaStringConfig["replace"].Vector())
		metaString.replaceTextID(entry.String());

	object->add(std::move(metaString));
	return S.retVoid();
}

int SpellProblemProxy::addGeneric(lua_State * L)
{
	LuaStack S(L);

	Problem * problem = nullptr;
	const Mechanics * mechanics = nullptr;

	S.getNonNullOrThrow(1, problem);
	S.getNonNullOrThrow(2, mechanics);

	mechanics->adaptGenericProblem(*problem);
	return S.retVoid();
}

int SpellProblemProxy::addStandard(lua_State * L)
{
	LuaStack S(L);

	Problem * problem = nullptr;
	const Mechanics * mechanics = nullptr;
	ESpellCastProblem spellProblem = ESpellCastProblem::OK;

	S.getNonNullOrThrow(1, problem);
	S.getNonNullOrThrow(2, mechanics);
	S.getOrThrow(3, spellProblem);

	mechanics->adaptProblem(spellProblem, *problem);
	return S.retVoid();
}

}
}

VCMI_LIB_NAMESPACE_END
