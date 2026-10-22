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

#include "../Destination.h"
#include "../../constants/Enumerations.h"

struct Bonus;
class CSpell;
class BattleHex;
class GameResID;
class MetaString;
class PlayerColor;
enum class BattleSide : int8_t;

namespace battle
{
class Unit;
}

namespace vstd
{
class RNG;
}

namespace spells
{
enum class Mode;
}

/// Outcome of IBattleActionEnvironment::moveUnit
struct UnitMoveResult
{
	int distance = 0; ///< number of traversed hexes
	bool completed = false; ///< false if an obstacle stopped the unit or the move was refused
};

/// Server operations available to a battle action while it is applied, bound to the battle of the action
class DLL_LINKAGE IBattleActionEnvironment
{
public:
	virtual ~IBattleActionEnvironment() = default;

	/// Adds the bonuses to the unit, as IBattleState::updateUnitBonus does
	virtual void updateUnitBonuses(const battle::Unit & unit, const std::vector<Bonus> & bonuses) = 0;
	virtual void addBattleLogLine(const MetaString & line) = 0;
	/// Runs combat event handlers of the unit and of every other unit reacting to the event. Spell is the one the event is about, or nullptr
	virtual void fireCombatEvent(CombatEventType event, const battle::Unit * unit, const battle::Unit * other, const CSpell * spell) = 0;
	/// Triggers obstacles on the hexes the unit occupies
	virtual void triggerObstaclesUnder(const battle::Unit & unit) = 0;
	/// Moves the unit to the hex, stopping early at obstacles on the way. Fires BEFORE_MOVE and AFTER_MOVE
	virtual UnitMoveResult moveUnit(const battle::Unit & unit, const BattleHex & destination) = 0;
	/// Makes the strikes of a melee attack at the hex of the defender, with first strike, retaliation and extra attacks. Distance is the one moved before the attack
	virtual void meleeAttack(const battle::Unit & attacker, const battle::Unit & defender, const BattleHex & targetHex, int distance, bool longWeapon) = 0;
	/// Makes the shots of a ranged attack at the hex, with first strike, ranged retaliation and extra shots
	virtual void rangedAttack(const battle::Unit & attacker, const BattleHex & destination) = 0;
	/// Casts the spell of the unit at the target with the given mode and spell level
	virtual void castSpell(const battle::Unit & caster, const CSpell * spell, spells::Mode mode, int level, const battle::Target & target) = 0;
	/// Casts the spell of the hero of the side at the target, and spends the charge of a spell cast from an artifact
	virtual void castHeroSpell(BattleSide side, const CSpell * spell, const battle::Target & target) = 0;
	/// Random generator of the server
	virtual vstd::RNG & getRandom() = 0;
	virtual void endBattle(EBattleResult result, BattleSide winner) = 0;
	virtual void giveResource(PlayerColor player, GameResID resource, int amount) = 0;
	virtual void endTacticsPhase() = 0;
};
