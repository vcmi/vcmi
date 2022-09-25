/*
 * api/Player.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Player.h"

#include "Registry.h"

#include "../LuaStack.h"
#include "../LuaCallWrapper.h"

namespace scripting
{
namespace api
{


VCMI_REGISTER_CORE_SCRIPT_API(PlayerProxy, "Player");

const std::vector<PlayerProxy::CustomRegType> PlayerProxy::REGISTER_CUSTOM =
{
//	virtual PlayerColor getColor() const = 0;
//	virtual TeamID getTeam() const = 0;
//	virtual bool isHuman() const = 0;
//	virtual const IBonusBearer * accessBonuses() const = 0;
//	virtual int getResourceAmount(int type) const = 0;

};

}
}

VCMI_LIB_NAMESPACE_END
