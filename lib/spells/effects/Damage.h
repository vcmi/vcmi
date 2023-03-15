/*
 * Damage.h, part of VCMI engine
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

struct StacksInjured;

namespace spells
{
namespace effects
{

class Damage : public UnitEffect
{
public:
	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;

protected:
	bool isReceptive(const Mechanics * m, const battle::Unit * unit) const override;

	void serializeJsonUnitEffect(JsonSerializeFormat & handler) override final;

	int64_t damageForTarget(size_t targetIndex, const Mechanics * m, const battle::Unit * target) const;

	virtual void describeEffect(std::vector<MetaString> & log, const Mechanics * m, const battle::Unit * firstTarget, uint32_t kills, int64_t damage, bool multiple) const;

private:
	bool killByPercentage = false;
	bool killByCount = false;
};

}
}

VCMI_LIB_NAMESPACE_END
