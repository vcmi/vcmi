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

struct BattleUnitsChanged;

namespace spells
{
namespace effects
{

class Heal : public UnitEffect
{
public:
	Heal();
	virtual ~Heal();

	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;

protected:
	void apply(int64_t value, ServerCallback * server, const Mechanics * m, const EffectTarget & target) const;

	bool isValidTarget(const Mechanics * m, const battle::Unit * unit) const override;
	void serializeJsonUnitEffect(JsonSerializeFormat & handler) override final;

private:
    EHealLevel healLevel;
	EHealPower healPower;

	int32_t minFullUnits;

	void prepareHealEffect(int64_t value, BattleUnitsChanged & pack, RNG & rng, const Mechanics * m, const EffectTarget & target) const;
};

}
}

VCMI_LIB_NAMESPACE_END
