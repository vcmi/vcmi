/*
 * Timed.h, part of VCMI engine
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

struct Bonus;
struct SetStackEffect;
struct MetaString;

namespace spells
{
namespace effects
{

class Timed : public UnitEffect
{
public:
	bool cumulative;
	std::vector<std::shared_ptr<Bonus>> bonus;

	Timed();
	virtual ~Timed();

	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;

protected:
	void serializeJsonUnitEffect(JsonSerializeFormat & handler) override final;

private:
	void convertBonus(const Mechanics * m, int32_t & duration, std::vector<Bonus> & converted) const;
	void describeEffect(std::vector<MetaString> & log, const Mechanics * m, const std::vector<Bonus> & bonuses, const battle::Unit * target) const;

};

}
}

VCMI_LIB_NAMESPACE_END
