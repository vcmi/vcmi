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

namespace scripting
{
namespace api
{
VCMI_REGISTER_CORE_SCRIPT_API(StackInstanceProxy, "StackInstance");

const std::vector<StackInstanceProxy::RegType> StackInstanceProxy::REGISTER =
{

};

const std::vector<StackInstanceProxy::CustomRegType> StackInstanceProxy::REGISTER_CUSTOM =
{
	{"getType", LuaMethodWrapper<CStackInstance, const Creature *(CStackBasicDescriptor:: *)()const, &CStackBasicDescriptor::getType>::invoke, false},
	{"getCount", LuaMethodWrapper<CStackInstance, TQuantity(CStackBasicDescriptor:: *)()const, &CStackBasicDescriptor::getCount>::invoke, false},
};


}
}

