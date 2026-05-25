/*
 * api/Player.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/Player.h>

#include "../LuaWrapper.h"

namespace scripting
{
namespace api
{

class PlayerProxy : public OpaqueWrapper<const Player, PlayerProxy>
{
public:
	using Wrapper = OpaqueWrapper<const Player, PlayerProxy>;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;

};

}
}

VCMI_LIB_NAMESPACE_END
