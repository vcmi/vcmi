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
class MeleeAttackBase : public BattleActionType
{
public:
	using BattleActionType::BattleActionType;

	bool isUnitAction() const override;

	std::vector<ActionApproach> getApproaches(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const BattleHex & hoveredHex) const override;
	ActionPreview preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const override;
	std::optional<battle::Target> buildTarget(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const override;

	bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const override;
	void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const override;

	/// Attack that clicking the defender from the hex makes: the melee attack option of the actor with the best priority that strikes the defender from the hex.
	/// Nothing if no option does
	static std::optional<BattleAction> buildAutomaticAttack(const CBattleInfoCallback & battle, const battle::Unit & actor, const battle::Unit & defender, const BattleHex & attackFrom);

protected:
	/// Strikes over one free hex instead of at an adjacent unit, and is not retaliated
	virtual bool usesLongWeapon() const;
	/// A unit with RETURN_AFTER_STRIKE moves back to the position it attacked from
	virtual bool returnsAfterStrike() const;

private:
	bool canStrikeFrom(const CBattleInfoCallback & battle, const battle::Unit & attacker, const battle::Unit & defender, const BattleHex & attackerPos) const;
	/// Hexes of units struck by the attack besides the hovered one, such as by breath or ATTACKS_ALL_ADJACENT
	BattleHexArray getAttackedHexes(const CBattleInfoCallback & battle, const battle::Unit & attacker, const BattleHex & destinationTile, const BattleHex & attackerPos) const;
};

/// MELEE_ATTACK: melee attack, after which a unit with RETURN_AFTER_STRIKE returns
class MeleeAttackAction : public MeleeAttackBase
{
public:
	using MeleeAttackBase::MeleeAttackBase;

	void collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const override;
	int getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const override;

protected:
	bool returnsAfterStrike() const override;
};

/// MELEE_ATTACK_NO_RETURN: melee attack of a unit that stays at its target
class MeleeAttackNoReturnAction : public MeleeAttackBase
{
public:
	using MeleeAttackBase::MeleeAttackBase;

	void collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const override;
	int getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const override;
};

/// LONG_WEAPON_ATTACK: attack of a LONG_WEAPON unit over one free hex
class LongWeaponAttackAction : public MeleeAttackBase
{
public:
	using MeleeAttackBase::MeleeAttackBase;

	void collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const override;
	int getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const override;

protected:
	bool usesLongWeapon() const override;
};
