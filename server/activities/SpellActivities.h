/*
 * SpellActivities.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "Activity.h"

/// Asks which town a town portal style spell should teleport its caster to, and completes
/// the cast once the player has chosen. Holds everything it needs by value, so nothing has
/// to stay alive while it waits.
class TownSelectionActivity : public DialogActivity
{
	SpellID spell;
	ObjectInstanceID caster;
	std::vector<ObjectInstanceID> offeredTowns;

public:
	static constexpr ActivityType TYPE = ActivityType::TownSelection;

	TownSelectionActivity(CGameHandler * owner, PlayerColor player, SpellID spell, ObjectInstanceID caster, std::vector<ObjectInstanceID> offeredTowns);

	bool acceptsAnswerWithoutValue() const override;
	void onRemoval(PlayerColor color) override;
};

/// Holds the answer to a dialog opened by a map script, for the paused script below it to
/// read when it resumes.
class ScriptDialogActivity : public DialogActivity
{
public:
	static constexpr ActivityType TYPE = ActivityType::ScriptDialog;

	ScriptDialogActivity(CGameHandler * owner, PlayerColor player);

	bool acceptsAnswerWithoutValue() const override;
	void onRemoval(PlayerColor color) override;
};
