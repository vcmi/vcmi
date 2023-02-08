/*
 * RemoveObstacle.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "LocationEffect.h"

#include "../../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

struct CObstacleInstance;
struct BattleObstaclesChanged;

namespace spells
{
namespace effects
{

class RemoveObstacle : public LocationEffect
{
public:
	RemoveObstacle();
	virtual ~RemoveObstacle() = default;

	bool applicable(Problem & problem, const Mechanics * m) const override;
	bool applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const override;

	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;

protected:
	void serializeJsonEffect(JsonSerializeFormat & handler) override;

private:
    bool removeAbsolute;
    bool removeUsual;
    bool removeAllSpells;

    std::set<SpellID> removeSpells;

    bool canRemove(const CObstacleInstance * obstacle) const;

	std::set<const CObstacleInstance *> getTargets(const Mechanics * m, const EffectTarget & target, bool alwaysMassive) const;
};

}
}

VCMI_LIB_NAMESPACE_END
