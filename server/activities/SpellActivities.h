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

/// Asks which town a town-portal style spell should send its caster to, and finishes
/// the cast once the player has chosen. Everything it needs to do that is held by
/// value, so it does not depend on anything staying alive across the wait.
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

/// Holds the answer to a dialog a map script put up, for the paused script sitting
/// underneath it to take when it resumes.
class ScriptDialogActivity : public DialogActivity
{
public:
	static constexpr ActivityType TYPE = ActivityType::ScriptDialog;

	ScriptDialogActivity(CGameHandler * owner, PlayerColor player);

	bool acceptsAnswerWithoutValue() const override;
	void onRemoval(PlayerColor color) override;
};
