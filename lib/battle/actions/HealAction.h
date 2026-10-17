/*
 * HealAction.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BattleActionType.h"

class CSpell;

/// STACK_HEAL: a HEALER unit, the first aid tent, heals a wounded unit of its side with the spell of its ability.
/// Without a target the most wounded unit is healed, and the turn is skipped if there is none
class HealAction : public BattleActionType
{
	/// Checks that the spell of the healer heals the unit
	bool canHeal(const CBattleInfoCallback & battle, const battle::Unit & healer, const CSpell * spell, const battle::Unit & healed, spells::Problem & problem) const;
	/// Unit with the most lost health of its top creature among the ones the healer can heal, or nullptr if there is none
	const battle::Unit * findMostWounded(const CBattleInfoCallback & battle, const battle::Unit & healer, const CSpell * spell) const;

public:
	bool isUnitAction() const override;

	void collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const override;
	int getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const override;
	ActionPreview preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const override;
	std::optional<BattleAction> build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const override;

	bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const override;
	void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const override;
};
