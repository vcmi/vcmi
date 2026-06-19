/*
 * BattleHex.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "BattleHex.h"

#include "../Enums.h"
#include "../../../lib/GameLibrary.h"
#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"
#include "../Registry.h"

#include "BattleHexArray.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

void BattleHexProxy::registerMethods(MethodRegistrar & R)
{
	R.method<&BattleHex::isValid>            ("isValid",     {}, "Returns true if this hex value identifies existing battlefield position.");
	R.method<&BattleHex::isAvailable>        ("isAvailable", {}, "Returns true if the hex is on the battlefield and is not located in the first or last column, which are reserved for back tile of war machines and are not accessible for regular movement. NOTE: does NOT checks whether hex is blocked by obstacles or units");
	R.function<&BattleHexProxy::getClosestTile>("getClosestTile",
		{
			{"side",  "Tie-breaker side — closer to this side wins when several candidates are equidistant."},
			{"hexes", "Candidate hexes to search through."}
		}, {},
		"Searches provided array for hex that is closest to current one. If multiple equidistant hexes are found, this function will prefer hex closest to specified battle side.");
	R.method<&BattleHex::copyToNorthWest>    ("copyToNorthWest", {}, "Returns the neighbouring hex one step north-west.");
	R.method<&BattleHex::copyToNorthEast>    ("copyToNorthEast", {}, "Returns the neighbouring hex one step north-east.");
	R.method<&BattleHex::copyToEast>         ("copyToEast",      {}, "Returns the neighbouring hex one step east.");
	R.method<&BattleHex::copyToSouthEast>    ("copyToSouthEast", {}, "Returns the neighbouring hex one step south-east.");
	R.method<&BattleHex::copyToSouthWest>    ("copyToSouthWest", {}, "Returns the neighbouring hex one step south-west.");
	R.method<&BattleHex::copyToWest>         ("copyToWest",      {}, "Returns the neighbouring hex one step west.");
}

BattleHex BattleHexProxy::getClosestTile(const BattleHex & self, BattleSide side, const BattleHexArray & hexes)
{
	return BattleHex::getClosestTile(side, self, hexes);
}

}

VCMI_LIB_NAMESPACE_END
