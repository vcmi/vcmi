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

struct Bonus;
class BattleHex;
class GameResID;
class MetaString;
class PlayerColor;
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

	/// Adds the bonuses to the unit, as IBattleState::updateUnitBonus does
	virtual void updateUnitBonuses(const battle::Unit & unit, const std::vector<Bonus> & bonuses) = 0;
	virtual void addBattleLogLine(const MetaString & line) = 0;
	/// Runs combat event handlers of the unit and of every other unit reacting to the event
	virtual void fireCombatEvent(CombatEventType event, const battle::Unit * unit, const battle::Unit * other) = 0;
	/// Triggers obstacles on the hexes the unit occupies
	virtual void triggerObstaclesUnder(const battle::Unit & unit) = 0;
	/// Moves the unit to the hex, stopping early at obstacles on the way. Fires BEFORE_MOVE and AFTER_MOVE
	virtual void moveUnit(const battle::Unit & unit, const BattleHex & destination) = 0;
	/// Makes the shots of a ranged attack at the hex, with first strike, ranged retaliation and extra shots
	virtual void rangedAttack(const battle::Unit & attacker, const BattleHex & destination) = 0;
	virtual void endBattle(EBattleResult result, BattleSide winner) = 0;
	virtual void giveResource(PlayerColor player, GameResID resource, int amount) = 0;
};
