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

#include "BAI/v13/stack.h"
#include "schema/v13/types.h"

namespace MMAI::BAI::V13
{

struct AttackLogData
{
	const std::shared_ptr<Stack> attacker; // XXX: can be nullptr if dmg is not from creature
	const std::shared_ptr<Stack> defender;
	const CStack * cattacker;
	const CStack * cdefender;
	const int dmg;
	const int dmgPermille;
	const int units;
	const int value;
	const int valuePermille;
};

class AttackLog : public Schema::V13::IAttackLog
{
public:
	explicit AttackLog(const AttackLogData & data) : data(data) {};

	const AttackLogData data;

	// IAttackLog impl
	Stack * getAttacker() const override
	{
		return data.attacker.get();
	}
	Stack * getDefender() const override
	{
		return data.defender.get();
	}
	int getDamageDealt() const override
	{
		return data.dmg;
	}
	int getDamageDealtPermille() const override
	{
		return data.dmgPermille;
	}
	int getUnitsKilled() const override
	{
		return data.units;
	}
	int getValueKilled() const override
	{
		return data.value;
	}
	int getValueKilledPermille() const override
	{
		return data.valuePermille;
	}

	/*
	 * attacker dealing dmg might be our friendly fire
	 * If we look at Attacker POV, we would count our friendly fire as "dmg dealt"
	 * So we look at Defender POV, so our friendly fire is counted as "dmg received"
	 * This means that if the enemy does friendly fire dmg,
	 *  we would count it as our dmg dealt - that is OK (we have "tricked" the enemy!)
	 * => store only defender slot
	 */
};
}
