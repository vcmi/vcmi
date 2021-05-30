/*
 * Rng.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../../lib/CRandomGenerator.h"

#include "../LuaWrapper.h"

namespace scripting
{
namespace api
{

class RngProxy : public OpaqueWrapper<CRandomGenerator, RngProxy>
{
public:
	using Wrapper = OpaqueWrapper<CRandomGenerator, RngProxy>;

	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;

	static int getDefault(lua_State * L);
};

}
}
