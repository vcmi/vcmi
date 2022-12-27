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
	DemonSummon();
	virtual ~DemonSummon();

	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;
protected:
	bool isValidTarget(const Mechanics * m, const battle::Unit * s) const override;

	void serializeJsonUnitEffect(JsonSerializeFormat & handler) override final;

private:
	CreatureID creature;

	bool permanent;
};

}
}

VCMI_LIB_NAMESPACE_END
