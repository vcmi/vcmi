/*
 * unit_shoot_dmg_unit.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "BAI/v15/graph/edges/unit_shoot_dmg_unit_v15.h"

#include "BAI/v15/graph/util_v15.h"

namespace MMAI::BAI::V15::Graph::Edges
{

Unit_ShootDmg_Unit::Unit_ShootDmg_Unit(
	const std::shared_ptr<const Nodes::Unit> & srcNode,
	const std::shared_ptr<const Nodes::Unit> & dstNode,
	const Args & args
)
	: detail::Unit_ShootDmg_Unit_Base(srcNode, dstNode)
{
	auto netValue = args.vdiffAttacker - args.vdiffDefender;
	auto attackerHp = static_cast<int>(srcNode->cstack.getTotalHealth());
	auto defenderHp = static_cast<int>(dstNode->cstack.getTotalHealth());

	setattr(A::ESTIMATED_NET_VALUE_REL_BF, permille(netValue, args.battlefieldValue));
	setattr(A::ESTIMATED_ATTACKER_HPDIFF_REL_SELF, permille(args.hpdiffAttacker, attackerHp));
	setattr(A::ESTIMATED_ATTACKER_HPDIFF_REL_BF, permille(args.hpdiffAttacker, args.battlefieldHp));
	setattr(A::ESTIMATED_DEFENDER_HPDIFF_REL_SELF, permille(args.hpdiffDefender, defenderHp));
	setattr(A::ESTIMATED_DEFENDER_HPDIFF_REL_BF, permille(args.hpdiffDefender, args.battlefieldHp));

	static_assert(static_cast<size_t>(A::_count) == 5, "whistleblower in case attributes change");
}

}
