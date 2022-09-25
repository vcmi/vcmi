/*
 * api/netpacks/BattleLogMessage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "PackForClient.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
namespace netpacks
{

class BattleLogMessageProxy : public SharedWrapper<BattleLogMessage, BattleLogMessageProxy>
{
public:
	using Wrapper = SharedWrapper<BattleLogMessage, BattleLogMessageProxy>;

	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;

	static int addText(lua_State * L);
};

}
}
}

VCMI_LIB_NAMESPACE_END
