/*
 * DemonSummon.h, part of VCMI engine
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

namespace spells
{
namespace effects
{

class DemonSummon : public UnitEffect
{
public:
	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;

	SpellEffectValue getHealthChange(const Mechanics * m, const EffectTarget & spellTarget) const final;

protected:
	bool isValidTarget(const Mechanics * m, const battle::Unit * s) const override;

	void serializeJsonUnitEffect(JsonSerializeFormat & handler) override final;

private:
	int32_t raisedCreatureAmount(const Mechanics * m, const battle::Unit * unit) const;

	CreatureID creature;

	bool permanent = false;
};

}
}

VCMI_LIB_NAMESPACE_END
