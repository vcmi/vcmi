/*
 * IBattleActionEnvironment.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../constants/Enumerations.h"

struct CPackForClient;
class PlayerColor;
class GameResID;
enum class BattleSide : int8_t;

namespace battle
{
class Unit;
}

/// Server operations available to a battle action while it is applied, bound to the battle of the action
class DLL_LINKAGE IBattleActionEnvironment
{
public:
	virtual ~IBattleActionEnvironment() = default;

	/// Sends the pack to clients and applies it to the game state
	virtual void apply(CPackForClient & pack) = 0;
	/// Runs combat event handlers of the unit and of every other unit reacting to the event
	virtual void fireCombatEvent(CombatEventType event, const battle::Unit * unit, const battle::Unit * other) = 0;
	/// Triggers obstacles on the hexes the unit occupies
	virtual void triggerObstaclesUnder(const battle::Unit & unit) = 0;
	virtual void endBattle(EBattleResult result, BattleSide winner) = 0;
	virtual void giveResource(PlayerColor player, GameResID resource, int amount) = 0;
};
