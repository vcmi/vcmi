/*
 * LibActionEntry.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "BattleActionEntry.h"

class BattleInterface;

/// Entry of an option offered by a lib battle action type to the active unit
class LibActionEntry final : public IBattleActionEntry
{
	BattleInterface & owner;
	const CStack & actor;
	ActionOption option;

	ActionContext getContext(const BattleHex & hex) const;

public:
	LibActionEntry(BattleInterface & owner, const CStack & actor, const ActionOption & option);

	int getPriority(const CStack * actor, const CStack * target) const override;
	bool isLegal(const BattleHex & hex) const override;
	BattleActionPreview preview(const BattleHex & hex) const override;
	BattleHexArray getShadedHexes(const BattleHex & hex) const override;
	void realize(const BattleHex & hex) const override;
	SpellID getSpell() const override;
	std::optional<UnitActionButton> getPanelButton() const override;
};
