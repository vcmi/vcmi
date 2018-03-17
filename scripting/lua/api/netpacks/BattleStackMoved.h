/*
 * api/netpacks/BattleStackMoved.h, part of VCMI engine
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

class BattleStackMovedProxy : public SharedWrapper<BattleStackMoved, BattleStackMovedProxy>
{
public:
	using Wrapper = SharedWrapper<BattleStackMoved, BattleStackMovedProxy>;

	static const std::vector<Wrapper::RegType> REGISTER;

	static int addTileToMove(lua_State * L, std::shared_ptr<BattleStackMoved> object);

	static int setUnitId(lua_State * L, std::shared_ptr<BattleStackMoved> object);
	static int setDistance(lua_State * L, std::shared_ptr<BattleStackMoved> object);
	static int setTeleporting(lua_State * L, std::shared_ptr<BattleStackMoved> object);
};

}
}
}
