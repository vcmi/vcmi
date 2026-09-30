/*
 * ShootAction.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BattleActionType.h"

/// SHOOT: a ranged attack at a unit, or at any hex for area shots of SPELL_LIKE_ATTACK units
class ShootAction : public BattleActionType
{
public:
	bool isUnitAction() const override;

	void collectOptions(const CBattleInfoCallback & battle, const battle::Unit & actor, std::vector<ActionOption> & out) const override;
	int getPriority(const CBattleInfoCallback & battle, const ActionOption & option, const battle::Unit & actor, const battle::Unit * target) const override;
	ActionPreview preview(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const override;
	BattleAction build(const CBattleInfoCallback & battle, const ActionOption & option, const ActionContext & context) const override;

	bool validate(const IGameInfoCallback & game, const CBattleInfoCallback & battle, const BattleAction & action, spells::Problem & problem) const override;
	void apply(IBattleActionEnvironment & env, const CBattleInfoCallback & battle, const BattleAction & action) const override;
};
