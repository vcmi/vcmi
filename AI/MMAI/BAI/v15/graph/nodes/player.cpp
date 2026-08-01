/*
 * player.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "BAI/v15/graph/nodes/player.h"

#include "BAI/v15/graph/util.h"

#include "AI/MMAI/common.h"

namespace MMAI::BAI::V15::Graph::Nodes
{

Player::Player(const Args & args) : side(args.side)
{
	setattr(A::BATTLE_SIDE, EU(side));
	setattr(A::IS_ACTIVE, args.isActive);

	setattr(A::ARMY_VALUE_NOW_REL0, permille(args.value, args.globalValueStart));
	setattr(A::ARMY_VALUE_NOW_REL, permille(args.value, args.globalValuePrevRound));
	setattr(A::ARMY_HP_NOW_REL, permille(args.hp, args.globalHpPrevRound));
	setattr(A::VALUE_KILLED_NOW_REL, permille(args.valueKilled, args.globalValuePrevRound));
	setattr(A::VALUE_LOST_NOW_REL, permille(args.valueLost, args.globalValuePrevRound));
	setattr(A::DMG_DEALT_NOW_REL, permille(args.dmgDealt, args.globalHpPrevRound));
	setattr(A::DMG_RECEIVED_NOW_REL, permille(args.dmgReceived, args.globalHpPrevRound));

	static_assert(EU(A::_count) == 9, "whistleblower in case attributes change");
}

}
