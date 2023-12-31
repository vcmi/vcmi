/*
 * api/Creature.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../LuaWrapper.h"

#include <vcmi/Creature.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{

class CreatureProxy : public OpaqueWrapper<const Creature, CreatureProxy>
{
public:
	using Wrapper = OpaqueWrapper<const Creature, CreatureProxy>;

	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
};


}
}

VCMI_LIB_NAMESPACE_END
