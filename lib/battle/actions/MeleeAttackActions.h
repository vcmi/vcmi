/*
 * MeleeAttackActions.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BattleActionType.h"

/// Base of the melee attack types: the unit moves to a hex and strikes a unit from there.
/// The action targets the hex to attack from, followed by the attacked hex or unit
class MeleeAttackAction : public BattleActionType
{
public:
	bool isUnitAction() const override;

	bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const override;
	void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const override;

protected:
	/// Strikes over one free hex instead of at an adjacent unit, and is not retaliated
	virtual bool usesLongWeapon() const;
	/// A unit with RETURN_AFTER_STRIKE moves back to the position it attacked from
	virtual bool returnsAfterStrike() const;

private:
	bool canStrikeFrom(const CBattleInfoCallback & battle, const battle::Unit & attacker, const battle::Unit & defender, const BattleHex & attackerPos) const;
};

/// WALK_AND_ATTACK: melee attack, after which a unit with RETURN_AFTER_STRIKE returns
class WalkAndAttackAction : public MeleeAttackAction
{
protected:
	bool returnsAfterStrike() const override;
};

/// WALK_AND_ATTACK_NO_RETURN: melee attack of a unit that stays at its target
class WalkAndAttackNoReturnAction : public MeleeAttackAction
{
};

/// LONG_WEAPON_ATTACK: attack of a LONG_WEAPON unit over one free hex
class LongWeaponAttackAction : public MeleeAttackAction
{
protected:
	bool usesLongWeapon() const override;
};
