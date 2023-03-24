/*
 * CBattleInfoCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "../GameConstants.h"

class PossiblePlayerBattleAction // actions performed at l-click
{
public:
	enum Actions {
		INVALID = -1,
		CREATURE_INFO,
		HERO_INFO,
		MOVE_TACTICS,
		CHOOSE_TACTICS_STACK,

		MOVE_STACK,
		ATTACK,
		WALK_AND_ATTACK,
		ATTACK_AND_RETURN,
		SHOOT,
		CATAPULT,
		HEAL,

		RANDOM_GENIE_SPELL,   // random spell on a friendly creature

		NO_LOCATION,          // massive spells that affect every possible target, automatic casts
		ANY_LOCATION,
		OBSTACLE,
		TELEPORT,
		SACRIFICE,
		FREE_LOCATION,        // used with Force Field and Fire Wall - all tiles affected by spell must be free
		AIMED_SPELL_CREATURE, // spell targeted at creature
	};

private:
	Actions action;
	SpellID spellToCast;

public:
	bool spellcast() const
	{
		return action == ANY_LOCATION || action == NO_LOCATION || action == OBSTACLE || action == TELEPORT ||
			   action == SACRIFICE || action == FREE_LOCATION || action == AIMED_SPELL_CREATURE;
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
};
