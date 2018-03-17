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

const std::vector<ObjectInstanceProxy::RegType> ObjectInstanceProxy::REGISTER = {};

const std::vector<ObjectInstanceProxy::CustomRegType> ObjectInstanceProxy::REGISTER_CUSTOM =
{
	{"getOwner", LuaMethodWrapper<CGObjectInstance, PlayerColor(IObjectInterface:: *)()const, &IObjectInterface::getOwner>::invoke, false},
	{"getObjGroupIndex", LuaMethodWrapper<CGObjectInstance, int32_t(IObjectInterface:: *)()const, &IObjectInterface::getObjGroupIndex>::invoke, false},
	{"getObjTypeIndex", LuaMethodWrapper<CGObjectInstance, int32_t(IObjectInterface:: *)()const, &IObjectInterface::getObjTypeIndex>::invoke, false},
	{"getVisitablePosition", LuaMethodWrapper<CGObjectInstance, int3(IObjectInterface:: *)()const, &IObjectInterface::visitablePos>::invoke, false},
	{"getPosition", LuaMethodWrapper<CGObjectInstance, int3(IObjectInterface:: *)()const, &IObjectInterface::getPosition>::invoke, false},
};

}
}

