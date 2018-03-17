/*
 * GameCb.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/Service.h>
#include "../../../lib/CGameInfoCallback.h"

#include "../LuaWrapper.h"

namespace scripting
{
namespace api
{

class GameCbProxy : public OpaqueWrapper<const GameCb, GameCbProxy>
{
public:
	using Wrapper = OpaqueWrapper<const GameCb, GameCbProxy>;

	static const std::vector<typename Wrapper::RegType> REGISTER;

};

}
}
