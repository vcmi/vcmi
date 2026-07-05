#include "BAI/v15/graph/edges/unit_melee_dmg_unit.h"
#include "BAI/v15/graph/util.h"

namespace MMAI::BAI::V15::Graph::Edges
{

Unit_MeleeDmg_Unit::Unit_MeleeDmg_Unit(
	const std::shared_ptr<const Nodes::Unit> & srcNode,
	const std::shared_ptr<const Nodes::Unit> & dstNode,
	const Args & args
)
	: detail::Unit_MeleeDmg_Unit_Base(srcNode, dstNode)
{
	int netValue = args.vdiffAttacker - args.vdiffDefender;
	int attackerHp = static_cast<int>(srcNode->cstack.getTotalHealth());
	int defenderHp = static_cast<int>(dstNode->cstack.getTotalHealth());

	setattr(A::ESTIMATED_ATTACKER_HPDIFF_REL_SELF, permille(args.hpdiffAttacker, attackerHp));
	setattr(A::ESTIMATED_ATTACKER_HPDIFF_REL_BF, permille(args.hpdiffAttacker, args.battlefieldHp));
	setattr(A::ESTIMATED_DEFENDER_HPDIFF_REL_SELF, permille(args.hpdiffDefender, defenderHp));
	setattr(A::ESTIMATED_DEFENDER_HPDIFF_REL_BF, permille(args.hpdiffDefender, args.battlefieldHp));
	setattr(A::ESTIMATED_NET_VALUE_REL_BF, permille(netValue, args.battlefieldValue));

	static_assert(static_cast<size_t>(A::_count) == 5, "whistleblower in case attributes change");
}

}
