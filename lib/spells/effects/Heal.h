/*
 * Heal.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "UnitEffect.h"
#include "../../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

struct BattleLogMessage;
struct BattleUnitsChanged;

namespace spells
{
namespace effects
{

class Heal : public UnitEffect
{
public:
	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;

protected:
	void apply(int64_t value, ServerCallback * server, const Mechanics * m, const EffectTarget & target) const;

	bool isValidTarget(const Mechanics * m, const battle::Unit * unit) const override;
	void serializeJsonUnitEffect(JsonSerializeFormat & handler) override final;

private:
	EHealLevel healLevel = EHealLevel::HEAL;
	EHealPower healPower = EHealPower::PERMANENT;

	int32_t minFullUnits = 0;

	void prepareHealEffect(int64_t value, BattleUnitsChanged & pack, BattleLogMessage & logMessage, RNG & rng, const Mechanics * m, const EffectTarget & target) const;
};

}
}

VCMI_LIB_NAMESPACE_END
