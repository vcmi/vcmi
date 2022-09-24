/*
 * StackInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "StackInstance.h"

#include "Registry.h"

#include "../LuaStack.h"
#include "../LuaCallWrapper.h"
#include <vcmi/Creature.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
VCMI_REGISTER_CORE_SCRIPT_API(StackInstanceProxy, "StackInstance");

const std::vector<StackInstanceProxy::CustomRegType> StackInstanceProxy::REGISTER_CUSTOM =
{
	{"getType", LuaMethodWrapper<CStackInstance, decltype(&CStackBasicDescriptor::getType), &CStackBasicDescriptor::getType>::invoke, false},
	{"getCount", LuaMethodWrapper<CStackInstance, decltype(&CStackBasicDescriptor::getCount), &CStackBasicDescriptor::getCount>::invoke, false},
};


}
}

VCMI_LIB_NAMESPACE_END
