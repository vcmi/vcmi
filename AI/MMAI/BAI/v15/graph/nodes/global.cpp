/*
 * global.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "BAI/v15/graph/nodes/global.h"
#include "battle/BattleSide.h"
#include "schema/v15/types.h"

namespace MMAI::BAI::V15::Graph::Nodes
{

Global::Global(const Args & args)
{
	static_assert(EU(S15::CombatResult::LEFT_WINS) == EU(BattleSide::LEFT_SIDE));
	static_assert(EU(S15::CombatResult::RIGHT_WINS) == EU(BattleSide::RIGHT_SIDE));
	setattr(A::BATTLE_WINNER, EU(args.res));
	setattr(A::BATTLE_ROUND, args.round);
	setattr(A::HAS_UPPER_TOWER, args.towers.hasUpperTower);
	setattr(A::HAS_MIDDLE_TOWER, args.towers.hasMiddleTower);
	setattr(A::HAS_BOTTOM_TOWER, args.towers.hasBottomTower);
	setattr(A::HAS_GATE_CORPSE, args.corpses.hasGateCorpse);
	setattr(A::HAS_BRIDGE_CORPSE, args.corpses.hasBridgeCorpse);

	static_assert(EU(A::_count) == 7);
}

}
