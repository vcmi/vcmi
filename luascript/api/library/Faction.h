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

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

class FactionProxy : public RawPointerWrapper<const Faction, FactionProxy>
{
public:
	static constexpr std::string_view luaName = "Faction";
	static constexpr std::string_view luaDescription =
		"A town/army faction definition (Castle, Rampart, Inferno, …). Identifies the kind, "
		"not a placed town; use Game lookups for the latter.";

	static void registerMethods(MethodRegistrar & R);
};

}

VCMI_LIB_NAMESPACE_END
