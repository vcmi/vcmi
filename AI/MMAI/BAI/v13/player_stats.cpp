/*
 * player_stats.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "BAI/v13/global_stats.h"
#include "BAI/v13/player_stats.h"
#include "schema/v13/constants.h"

namespace MMAI::BAI::V13
{

namespace S13 = Schema::V13;
using Side = Schema::Side;
using GA = Schema::V13::GlobalAttribute;
using A = Schema::V13::PlayerAttribute;

static_assert(EI(Side::LEFT) == EI(BattleSide::LEFT_SIDE));
static_assert(EI(Side::RIGHT) == EI(BattleSide::RIGHT_SIDE));

PlayerStats::PlayerStats(BattleSide side, int value, int hp)
{
	// Fill with NA to guard against "forgotten" attrs
	// (all attrs are strict so encoder will throw if NAs are found)
	attrs.fill(S13::NULL_VALUE_UNENCODED);

	static_assert(EI(A::_count) == 23, "whistleblower in case attributes change");

	setattr(A::BATTLE_SIDE, EI(side));
	setattr(A::VALUE_KILLED_ACC_ABS, 0);
	setattr(A::VALUE_LOST_ACC_ABS, 0);
	setattr(A::DMG_DEALT_ACC_ABS, 0);
	setattr(A::DMG_RECEIVED_ACC_ABS, 0);
};

void PlayerStats::update(const GlobalStats * gstats, int value, int hp, int dmgDealt, int dmgReceived, int valueKilled, int valueLost)
{
	// ll (long long) ensures long is 64-bit even on 32-bit systems
	setattr(A::ARMY_VALUE_NOW_ABS, value);
	setattr(A::ARMY_VALUE_NOW_REL, 1000LL * value / gstats->attr(GA::BFIELD_VALUE_NOW_ABS));
	setattr(A::ARMY_VALUE_NOW_REL0, 1000LL * value / gstats->attr(GA::BFIELD_VALUE_START_ABS));
	setattr(A::ARMY_HP_NOW_ABS, hp);
	setattr(A::ARMY_HP_NOW_REL, 1000LL * hp / gstats->attr(GA::BFIELD_HP_NOW_ABS));
	setattr(A::ARMY_HP_NOW_REL0, 1000LL * hp / gstats->attr(GA::BFIELD_HP_START_ABS));
	setattr(A::VALUE_KILLED_NOW_ABS, valueKilled);
	setattr(A::VALUE_KILLED_NOW_REL, 1000LL * valueKilled / gstats->attr(GA::BFIELD_VALUE_NOW_ABS));
	addattr(A::VALUE_KILLED_ACC_ABS, valueKilled);
	setattr(A::VALUE_KILLED_ACC_REL0, 1000LL * attr(A::VALUE_KILLED_ACC_ABS) / gstats->attr(GA::BFIELD_VALUE_START_ABS));
	setattr(A::VALUE_LOST_NOW_ABS, valueLost);
	setattr(A::VALUE_LOST_NOW_REL, 1000LL * valueLost / gstats->attr(GA::BFIELD_VALUE_NOW_ABS));
	addattr(A::VALUE_LOST_ACC_ABS, valueLost);
	setattr(A::VALUE_LOST_ACC_REL0, 1000LL * attr(A::VALUE_LOST_ACC_ABS) / gstats->attr(GA::BFIELD_VALUE_START_ABS));
	setattr(A::DMG_DEALT_NOW_ABS, dmgDealt);
	setattr(A::DMG_DEALT_NOW_REL, 1000LL * dmgDealt / gstats->attr(GA::BFIELD_HP_NOW_ABS));
	addattr(A::DMG_DEALT_ACC_ABS, dmgDealt);
	setattr(A::DMG_DEALT_ACC_REL0, 1000LL * attr(A::DMG_DEALT_ACC_ABS) / gstats->attr(GA::BFIELD_HP_START_ABS));
	setattr(A::DMG_RECEIVED_NOW_ABS, dmgReceived);
	setattr(A::DMG_RECEIVED_NOW_REL, 1000LL * dmgReceived / gstats->attr(GA::BFIELD_HP_NOW_ABS));
	addattr(A::DMG_RECEIVED_ACC_ABS, dmgReceived);
	setattr(A::DMG_RECEIVED_ACC_REL0, 1000LL * attr(A::DMG_RECEIVED_ACC_ABS) / gstats->attr(GA::BFIELD_HP_START_ABS));
}

int PlayerStats::getAttr(PlayerAttribute a) const
{
	return attr(a);
}

int PlayerStats::attr(PlayerAttribute a) const
{
	return attrs.at(EI(a));
};

void PlayerStats::setattr(PlayerAttribute a, int value)
{
	attrs.at(EI(a)) = value;
};

void PlayerStats::addattr(PlayerAttribute a, int value)
{
	attrs.at(EI(a)) += value;
};
}
