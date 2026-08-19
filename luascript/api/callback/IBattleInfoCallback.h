/*
 * IBattleInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/Service.h>
#include "../../../lib/battle/IBattleInfoCallback.h"

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

struct CObstacleInstance;
class CBattleInfoCallback;
class CGTownInstance;
class Creature;

namespace scripting::api
{

class IBattleInfoCallbackProxy : public RawPointerWrapper<const IBattleInfoCallback, IBattleInfoCallbackProxy>
{
public:
	static constexpr std::string_view luaName = "Battle";
	static constexpr std::string_view luaDescription =
		"Battlefield query interface. The handle scripts receive whenever they are passed a "
		"battle context — enumerate units and obstacles, test hex accessibility and shooting "
		"penalties, inspect wall state on siege maps. Read-only; mutations go through Server.";

	static void registerMethods(MethodRegistrar & R);

	static int getAvailableHex(lua_State * L);
	static int getUnitsIf(lua_State * L);
	static bool isAccessibleForUnit(const IBattleInfoCallback & object, const battle::Unit & unit, BattleHex hex);
	static bool isAccessibleForNewUnit(const IBattleInfoCallback & object, BattleHex hex, const Creature & creature, BattleSide side);
	static int getFieldWidth(const IBattleInfoCallback & object);
	static bool hasPenaltyOnLine(const IBattleInfoCallback & object, BattleHex from, BattleHex dest, bool checkWall, bool checkMoat);
	static bool isMeleeAttackPossible(const IBattleInfoCallback & object, const battle::Unit & attacker, const battle::Unit & defender);
	static bool hasDistancePenalty(const IBattleInfoCallback & object, const battle::Unit & shooter, const battle::Unit & target, std::optional<BattleHex> shooterHex, std::optional<BattleHex> targetHex);
	static bool hasWallPenalty(const IBattleInfoCallback & object, const battle::Unit & shooter, const battle::Unit & target, std::optional<BattleHex> shooterHex, std::optional<BattleHex> targetHex);
	static const CGTownInstance * getDefendedTown(const IBattleInfoCallback & object);
	static bool isToReverse(const IBattleInfoCallback & object, const battle::Unit & attacker, const battle::Unit & defender, std::optional<BattleHex> attackerHex, std::optional<BattleHex> defenderHex);
	static const battle::Unit * getUnitByPos(const IBattleInfoCallback & object, BattleHex hex, bool onlyAlive);
	static std::vector<std::shared_ptr<const CObstacleInstance>> getAllObstacles(const IBattleInfoCallback & object);
	static std::vector<std::shared_ptr<const CObstacleInstance>> getObstaclesOnPos(const IBattleInfoCallback & object, BattleHex hex, bool onlyBlocking);
	static bool hasFortifications(const IBattleInfoCallback & object);
	static bool hasMoat(const IBattleInfoCallback & object);
	static bool hasNativeStack(const IBattleInfoCallback & object, BattleSide side);
	static BattleHexArray getAllPossibleHexes(const IBattleInfoCallback & object);
	static std::optional<EWallState> getWallState(const IBattleInfoCallback & object, EWallPart part);
	static bool isWallPartAttackable(const IBattleInfoCallback & object, EWallPart part);
	static BattleHex wallPartToBattleHex(const IBattleInfoCallback & object, EWallPart part);
	static EWallPart hexToWallPart(const IBattleInfoCallback & object, BattleHex hex);
	static BattleHex getTowerShooterHex(const IBattleInfoCallback & object, EWallPart part);
};

}
