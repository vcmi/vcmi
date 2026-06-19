/*
 * IBattleInfoCallback.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "IBattleInfoCallback.h"
#include <vcmi/Entity.h>

#include "../Enums.h"
#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"

#include "../battle/BattleHex.h"
#include "../battle/BattleHexArray.h"
#include "../battle/Obstacle.h"
#include "../battle/Unit.h"

#include "../../../lib/GameConstants.h"
#include "../../../lib/battle/Unit.h"
#include "../../../lib/battle/AccessibilityInfo.h"
#include "../../../lib/battle/CBattleInfoCallback.h"
#include "../../../lib/battle/CBattleInfoEssentials.h"
#include "../../../lib/battle/CObstacleInstance.h"
#include "../../../lib/BattleFieldHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

void IBattleInfoCallbackProxy::registerMethods(MethodRegistrar & R)
{
	R.method<&BattleCb::battleTacticDist>("getTacticDistance", {},
		"Returns the available tactic phase distance, or 0 if the tactic phase has ended.");
	R.cfunction<&IBattleInfoCallbackProxy::getAvailableHex>("getAvailableHex",
		{
			{"creature", "Creature",    "Creature template whose footprint is being placed."},
			{"side",     "BattleSide",  "Side whose deployment area to search."},
			{"hex",      "BattleHex?",  "Preferred origin hex; nil falls back to a side-appropriate default."}
		},
		{"BattleHex", "Empty hex closest to the desired location that fits the creature, or INVALID if none."},
		"Returns an empty hex next to desired location that the creature can be placed on.");
	R.cfunction<&IBattleInfoCallbackProxy::getUnitsIf>("getUnitsIf",
		{{"predicate", "fun(u: Unit): boolean", "Selector — called for each unit on the battlefield; unit is kept when it returns true."}},
		{"Unit[]", "Units for which the predicate returned true."},
		"Returns all units for which the predicate returns true.");
	R.function<&IBattleInfoCallbackProxy::isAccessibleForUnit>("isAccessibleForUnit",
		{
			{"unit", "Unit whose movement model is consulted."},
			{"hex",  "Hex to test for reachability."}
		}, {},
		"True if the given hex is reachable by the given unit either on current turn or on any future turns.");
	R.function<&IBattleInfoCallbackProxy::hasPenaltyOnLine>("hasPenaltyOnLine",
		{
			{"from",      "Origin hex of the ranged attack."},
			{"dest",      "Target hex of the ranged attack."},
			{"checkWall", "Pass true to count crossing a wall as a penalty source."},
			{"checkMoat", "Pass true to count crossing a moat as a penalty source."}
		}, {},
		"True if a ranged attack along this line crosses a wall or moat (per the flags).");
	R.function<&IBattleInfoCallbackProxy::getUnitByPos>("getUnitByPos",
		{
			{"hex",       "Hex to inspect for a unit."},
			{"onlyAlive", "Pass true to skip dead-but-resurrectable stacks."}
		}, {},
		"Returns the unit covering the given hex, or nil.");
	R.function<&IBattleInfoCallbackProxy::getAllObstacles>("getAllObstacles", {},
		"Returns all obstacles on the battlefield.");
	R.function<&IBattleInfoCallbackProxy::getObstaclesOnPos>("getObstaclesOnPos",
		{
			{"hex",          "Hex whose obstacles are queried."},
			{"onlyBlocking", "Pass true to limit results to obstacles that block movement."}
		}, {},
		"Returns the obstacles on the given hex.");
	R.function<&IBattleInfoCallbackProxy::hasFortifications>("hasFortifications", {},
		"True if the battle is a siege with fortifications present.");
	R.function<&IBattleInfoCallbackProxy::hasMoat>("hasMoat", {},
		"True if the battlefield has a moat.");
	R.function<&IBattleInfoCallbackProxy::hasNativeStack>("hasNativeStack",
		{{"side", "Battle side to inspect (attacker or defender)."}}, {},
		"True if the given side has at least one native-terrain stack.");
	R.function<&IBattleInfoCallbackProxy::getAllPossibleHexes>("getAllPossibleHexes", {},
		"Returns every valid battlefield hex.");
	R.function<&IBattleInfoCallbackProxy::getWallState>("getWallState",
		{{"part", "Wall section to query."}}, {},
		"Returns the current state of the given wall section, or nil if absent.");
	R.function<&IBattleInfoCallbackProxy::isWallPartAttackable>("isWallPartAttackable",
		{{"part", "Wall section to test."}}, {},
		"True if the given wall section can be targeted by an attack.");
	R.function<&IBattleInfoCallbackProxy::wallPartToBattleHex>("wallPartToBattleHex",
		{{"part", "Wall section to look up."}}, {},
		"Returns the battle hex corresponding to the given wall section.");
	R.function<&IBattleInfoCallbackProxy::hexToWallPart>("hexToWallPart",
		{{"hex", "Hex to look up."}}, {},
		"Returns the wall section corresponding to the given battle hex.");
	R.function<&IBattleInfoCallbackProxy::getTowerShooterHex>("getTowerShooterHex",
		{{"part", "Wall section whose tower-shooter hex is queried."}}, {},
		"Returns the hex used by the tower shooter for the given wall section.");
}

bool IBattleInfoCallbackProxy::isAccessibleForUnit(const IBattleInfoCallback & object, const battle::Unit & unit, BattleHex hex)
{
	const auto * cb = dynamic_cast<const CBattleInfoCallback *>(&object);
	if(!cb)
		return false;
	return cb->getAccessibility(&unit).accessible(hex, &unit);
}

bool IBattleInfoCallbackProxy::hasPenaltyOnLine(const IBattleInfoCallback & object, BattleHex from, BattleHex dest, bool checkWall, bool checkMoat)
{
	const auto * cb = dynamic_cast<const CBattleInfoCallback *>(&object);
	if(!cb)
		return false;
	return cb->battleHasPenaltyOnLine(from, dest, checkWall, checkMoat);
}

int IBattleInfoCallbackProxy::getAvailableHex(lua_State * L)
{
	LuaStack S(L);

	const IBattleInfoCallback * object;
	S.getNonNull(1, object);

	const Creature * creature;
	BattleSide side;
	BattleHex hexVal;

	S.get(2, creature);
	S.get(3, side);

	if(lua_gettop(L) >= 4 && !lua_isnil(L, 4))
		S.get(4, hexVal);

	S.clear();
	BattleHex result = object->getAvailableHex(creature, side, hexVal);
	S.push(result);
	return 1;
}

const battle::Unit * IBattleInfoCallbackProxy::getUnitByPos(const IBattleInfoCallback & object, BattleHex hex, bool onlyAlive)
{
	return object.battleGetUnitByPos(hex, onlyAlive);
}

std::vector<std::shared_ptr<const CObstacleInstance>> IBattleInfoCallbackProxy::getAllObstacles(const IBattleInfoCallback & object)
{
	return object.battleGetAllObstacles(BattleSide::ALL_KNOWING);
}

std::vector<std::shared_ptr<const CObstacleInstance>> IBattleInfoCallbackProxy::getObstaclesOnPos(const IBattleInfoCallback & object, BattleHex hex, bool onlyBlocking)
{
	return object.battleGetAllObstaclesOnPos(hex, onlyBlocking);
}

bool IBattleInfoCallbackProxy::hasFortifications(const IBattleInfoCallback & object)
{
	return object.hasFortifications();
}

bool IBattleInfoCallbackProxy::hasMoat(const IBattleInfoCallback & object)
{
	return object.hasMoat();
}

bool IBattleInfoCallbackProxy::hasNativeStack(const IBattleInfoCallback & object, BattleSide side)
{
	return object.battleHasNativeStack(side);
}

BattleHexArray IBattleInfoCallbackProxy::getAllPossibleHexes(const IBattleInfoCallback &)
{
	BattleHexArray result;
	for(int i = 0; i < GameConstants::BFIELD_SIZE; i++)
		result.insert(BattleHex(static_cast<si16>(i)));
	return result;
}

std::optional<EWallState> IBattleInfoCallbackProxy::getWallState(const IBattleInfoCallback & object, EWallPart part)
{
	EWallState state = object.battleGetWallState(part);
	if(state == EWallState::NONE)
		return std::nullopt;
	return state;
}

bool IBattleInfoCallbackProxy::isWallPartAttackable(const IBattleInfoCallback & object, EWallPart part)
{
	return object.isWallPartAttackable(part);
}

BattleHex IBattleInfoCallbackProxy::wallPartToBattleHex(const IBattleInfoCallback & object, EWallPart part)
{
	return object.wallPartToBattleHex(part);
}

EWallPart IBattleInfoCallbackProxy::hexToWallPart(const IBattleInfoCallback & object, BattleHex hex)
{
	return object.battleHexToWallPart(hex);
}

BattleHex IBattleInfoCallbackProxy::getTowerShooterHex(const IBattleInfoCallback & object, EWallPart part)
{
	return object.getTowerShooterHex(part);
}

int IBattleInfoCallbackProxy::getUnitsIf(lua_State * L)
{
	LuaStack S(L);

	const IBattleInfoCallback * object;
	S.getNonNull(1, object);

	if(!S.isFunction(2))
		throw LuaApiException("Invalid parameters passed into getUnitsIf!");

	battle::Units units = object->battleGetUnitsIf([&L](const battle::Unit * unit){
		LuaStack S2(L);
		lua_pushvalue(L, 2); // bring copy of the function to top of stack
		S2.push(unit);

		if (lua_pcall(L, 1, 1, 0) != LUA_OK) {
			std::string error = lua_tostring(L, S2.absindex(-1));
			throw LuaApiException("Lua getUnitsIf callback failed with message: " + error);
		}

		bool result = lua_toboolean(L, S2.absindex(-1)) != 0;
		S2.restoreInitialTop();
		return result;
	});

	S.clear();
	S.push(units);
	return 1;
}

}

VCMI_LIB_NAMESPACE_END
