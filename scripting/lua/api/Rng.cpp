/*
 * Rng.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Rng.h"
#include "Registry.h"
#include "../LuaStack.h"
#include "../LuaCallWrapper.h"

namespace scripting
{
namespace api
{

VCMI_REGISTER_SCRIPT_API(RngProxy, "Rng");

const std::vector<RngProxy::CustomRegType> RngProxy::REGISTER_CUSTOM =
{
	{"getDefault", &RngProxy::getDefault, true},
	{"nextInt", LuaMethodWrapper<CRandomGenerator, int(CRandomGenerator::*)(int), &CRandomGenerator::nextInt>::invoke, false}
};

int RngProxy::getDefault(lua_State * L)
{
	LuaStack S(L);

	S.clear();
	S.push(&CRandomGenerator::getDefault());

	return S.retPushed();
}

}
}