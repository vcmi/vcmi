/*
 * ObjectInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "ObjectInstance.h"

#include "Registry.h"

#include "../LuaStack.h"
#include "../LuaCallWrapper.h"

namespace scripting
{
namespace api
{
VCMI_REGISTER_CORE_SCRIPT_API(ObjectInstanceProxy, "ObjectInstance");

const std::vector<ObjectInstanceProxy::CustomRegType> ObjectInstanceProxy::REGISTER_CUSTOM =
{
	{"getOwner", LuaMethodWrapper<CGObjectInstance, decltype(&IObjectInterface::getOwner), &IObjectInterface::getOwner>::invoke, false},
	{"getObjGroupIndex", LuaMethodWrapper<CGObjectInstance, decltype(&IObjectInterface::getObjGroupIndex), &IObjectInterface::getObjGroupIndex>::invoke, false},
	{"getObjTypeIndex", LuaMethodWrapper<CGObjectInstance, decltype(&IObjectInterface::getObjTypeIndex), &IObjectInterface::getObjTypeIndex>::invoke, false},
	{"getVisitablePosition", LuaMethodWrapper<CGObjectInstance, decltype(&IObjectInterface::visitablePos), &IObjectInterface::visitablePos>::invoke, false},
	{"getPosition", LuaMethodWrapper<CGObjectInstance, decltype(IObjectInterface::getPosition), &IObjectInterface::getPosition>::invoke, false},
};

}
}


VCMI_LIB_NAMESPACE_END
