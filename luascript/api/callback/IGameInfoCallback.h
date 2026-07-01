/*
 * IGameInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

#include "../../../lib/callback/IGameInfoCallback.h"

#include <vcmi/scripting/Service.h>

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;

namespace scripting::api
{

class IGameInfoCallbackProxy : public RawPointerWrapper<const GameCb, IGameInfoCallbackProxy>
{
public:
	static constexpr std::string_view luaName = "Game";
	static constexpr std::string_view luaDescription =
		"Adventure-map query interface. Provides world-level "
		"lookups: current date, players, towns, heroes, and map objects accessible to the "
		"calling script's owner.";

	static void registerMethods(MethodRegistrar & R);
};

}

VCMI_LIB_NAMESPACE_END
