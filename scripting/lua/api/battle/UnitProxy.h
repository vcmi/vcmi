/*
 * UnitProxy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/Service.h>
#include "../../../../lib/battle/Unit.h"

#include "../../LuaWrapper.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
namespace battle
{
using ::battle::IUnitInfo;
using ::battle::Unit;

class UnitProxy : public OpaqueWrapper<const Unit, UnitProxy>
{
public:
	using Wrapper = OpaqueWrapper<const Unit, UnitProxy>;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
};

}
}
}

VCMI_LIB_NAMESPACE_END
