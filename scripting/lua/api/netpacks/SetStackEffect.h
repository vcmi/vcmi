/*
 * api/netpacks/SetStackEffect.h part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once


#include "PackForClient.h"

namespace scripting
{
namespace api
{
namespace netpacks
{

class SetStackEffectProxy : public SharedWrapper<SetStackEffect, SetStackEffectProxy>
{
public:
	using Wrapper = SharedWrapper<SetStackEffect, SetStackEffectProxy>;

	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;

	static int addBonus(lua_State * L);
};

}
}
}
