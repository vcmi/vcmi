/*
 * Teleport.h, part of VCMI engine
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

struct BattleStackMoved;

namespace spells
{
namespace effects
{

class Teleport : public UnitEffect
{
public:
	void adjustTargetTypes(std::vector<TargetType> & types) const override;

	bool applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const override;

	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;

	EffectTarget transformTarget(const Mechanics * m,  const Target & aimPoint, const Target & spellTarget) const override;

protected:
	void serializeJsonUnitEffect(JsonSerializeFormat & handler) override;

private:
	bool triggerObstacles;
	bool isWallPassable;
	bool isMoatPassable;
};

}
}

VCMI_LIB_NAMESPACE_END
