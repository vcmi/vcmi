/*
 * StackInstance.h, part of VCMI engine
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

#include "../../../lib/CCreatureSet.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{

class StackInstanceProxy : public OpaqueWrapper<const CStackInstance, StackInstanceProxy>
{
public:
	using Wrapper = OpaqueWrapper<const CStackInstance, StackInstanceProxy>;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
};


}
}

VCMI_LIB_NAMESPACE_END
