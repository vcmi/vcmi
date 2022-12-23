/*
 * Clone.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "UnitEffect.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

class Clone : public UnitEffect
{
public:
	Clone();
	virtual ~Clone();

	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;
protected:
	bool isReceptive(const Mechanics * m, const battle::Unit * s) const override;
	bool isValidTarget(const Mechanics * m, const battle::Unit * s) const override;

	void serializeJsonUnitEffect(JsonSerializeFormat & handler) override final;
private:
	int maxTier;
};

}
}

VCMI_LIB_NAMESPACE_END
