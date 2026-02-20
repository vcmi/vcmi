/*
 * global_stats.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "battle/BattleSide.h"
#include "schema/v13/types.h"

namespace MMAI::BAI::V13
{

using CombatResult = Schema::V13::CombatResult;
using GlobalAction = Schema::V13::GlobalAction;
using GlobalAttribute = Schema::V13::GlobalAttribute;
using GlobalAttrs = Schema::V13::GlobalAttrs;
using IGlobalStats = Schema::V13::IGlobalStats;

using GlobalActionMask = std::bitset<EI(GlobalAction::_count)>;

class GlobalStats : public IGlobalStats
{
public:
	GlobalStats(BattleSide side, int value, int hp);

	int getAttr(GlobalAttribute a) const override;
	int attr(GlobalAttribute a) const;
	void update(BattleSide side, CombatResult res, int value, int hp, bool canWait);
	void setattr(GlobalAttribute a, int value);
	GlobalAttrs attrs = {};
	GlobalActionMask actmask = 0; // for active stack only
};
}
