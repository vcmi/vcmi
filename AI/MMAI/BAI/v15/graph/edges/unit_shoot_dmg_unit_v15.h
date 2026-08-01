/*
 * unit_shoot_dmg_unit.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "AI/MMAI/common.h" // IWYU pragma: keep

#include "BAI/v15/graph/edges/base_v15.h"
#include "BAI/v15/graph/nodes/unit_v15.h"
#include "schema/v15/constants.h"

namespace MMAI::BAI::V15::Graph::Edges
{
namespace S15 = Schema::V15;

namespace detail
{
	using Unit_ShootDmg_Unit_Traits = S15::EncodingTraits<S15::Graph::EdgeAttributes::Unit_ShootDmg_Unit>;
	using Unit_ShootDmg_Unit_Base = Base<Nodes::Unit, Nodes::Unit, Unit_ShootDmg_Unit_Traits>;
}

class Unit_ShootDmg_Unit : public detail::Unit_ShootDmg_Unit_Base
{
public:
	struct Args
	{
		const int vdiffAttacker;
		const int vdiffDefender;
		const int hpdiffAttacker;
		const int hpdiffDefender;
		const int battlefieldValue;
		const int battlefieldHp;
	};

	static std::shared_ptr<const Unit_ShootDmg_Unit>
	Create(const std::shared_ptr<const Nodes::Unit> & srcNode, const std::shared_ptr<const Nodes::Unit> & dstNode, const Args & args)
	{
		return std::make_shared<const Unit_ShootDmg_Unit>(srcNode, dstNode, args);
	};

	explicit Unit_ShootDmg_Unit(const std::shared_ptr<const Nodes::Unit> & srcNode, const std::shared_ptr<const Nodes::Unit> & dstNode, const Args & args);
};
}
