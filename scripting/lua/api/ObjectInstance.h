/*
 * ObjectInstance.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/HeroClass.h>

#include "../LuaWrapper.h"

#include "../../../lib/mapObjects/CObjectHandler.h"

namespace scripting
{
namespace api
{

class ObjectInstanceProxy : public OpaqueWrapper<const CGObjectInstance, ObjectInstanceProxy>
{
public:
	using Wrapper = OpaqueWrapper<const CGObjectInstance, ObjectInstanceProxy>;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
};


}
}


VCMI_LIB_NAMESPACE_END
