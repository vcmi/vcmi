/*
 * attack_log.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "AI/MMAI/common.h" // IWYU pragma: keep

#include "BAI/v15/graph/nodes/unit.h"
#include "networkPacks/PacksForClientBattle.h"
#include "schema/v15/types.h"

namespace MMAI::BAI::V15
{

class AttackLog : public Schema::V15::IAttackLog
{
	using UnitPtr = std::shared_ptr<const Graph::Nodes::Unit>;

public:
	AttackLog(
		const BattleStackAttacked & bsa,
		const UnitPtr & attacker,
		const UnitPtr & defender,
		const int dmg,
		const int dmgPermille,
		const int units,
		const int value,
		const int valuePermille
	)
		: bsa(bsa), attacker(attacker), defender(defender), dmg(dmg), dmgPermille(dmgPermille), units(units), value(value), valuePermille(valuePermille) {};

	int getDamageDealt() const override
	{
		return dmg;
	}
	int getDamageDealtPermille() const override
	{
		return dmgPermille;
	}
	int getUnitsKilled() const override
	{
		return units;
	}
	int getValueKilled() const override
	{
		return value;
	}
	int getValueKilledPermille() const override
	{
		return valuePermille;
	}

	/*
	 * attacker dealing dmg might be our friendly fire
	 * If we look at Attacker POV, we would count our friendly fire as "dmg dealt"
	 * So we look at Defender POV, so our friendly fire is counted as "dmg received"
	 * This means that if the enemy does friendly fire dmg,
	 *  we would count it as our dmg dealt - that is OK (we have "tricked" the enemy!)
	 * => store only defender slot
	 */

	const BattleStackAttacked bsa;
	const UnitPtr attacker;
	const UnitPtr defender;
	const int dmg;
	const int dmgPermille;
	const int units;
	const int value;
	const int valuePermille;
};
}
