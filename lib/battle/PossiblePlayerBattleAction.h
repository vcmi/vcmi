/*
 * PossiblePlayerBattleAction.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../GameConstants.h"

class PossiblePlayerBattleAction // actions performed at l-click
{
public:
	enum Actions {
		INVALID = -1,
		WALK_AND_SPELLCAST,
	};

private:
	Actions action;
	SpellID spellToCast;

public:
	bool spellcast() const
	{
		return action == WALK_AND_SPELLCAST;
	}

	Actions get() const
	{
		return action;
	}

	SpellID spell() const
	{
		return spellToCast;
	}

	PossiblePlayerBattleAction(Actions action, SpellID spellToCast = SpellID::NONE):
		action(static_cast<Actions>(action)),
		spellToCast(spellToCast)
	{
		assert((spellToCast != SpellID::NONE) == spellcast());
	}

	bool operator == (const PossiblePlayerBattleAction & other) const
	{
		return action == other.action && spellToCast == other.spellToCast;
	}

	bool operator != (const PossiblePlayerBattleAction & other) const
	{
		return action != other.action || spellToCast != other.spellToCast;
	}
};
