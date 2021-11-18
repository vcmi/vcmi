/*
 * api/Faction.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/Faction.h>

#include "../LuaWrapper.h"

namespace scripting
{
namespace api
{

class FactionProxy : public OpaqueWrapper<const Faction, FactionProxy>
{
public:
	using Wrapper = OpaqueWrapper<const Faction, FactionProxy>;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
};

}
}
