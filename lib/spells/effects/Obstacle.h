/*
 * Obstacle.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "LocationEffect.h"
#include "../../battle/BattleHex.h"
#include "../../battle/obstacle/Obstacle.h"

namespace spells
{
namespace effects
{

class Obstacle : public LocationEffect
{
public:
	Obstacle();
	virtual ~Obstacle();

	void adjustAffectedHexes(std::set<BattleHex> & hexes, const Mechanics * m, const Target & spellTarget) const override;

	bool applicable(Problem & problem, const Mechanics * m) const override;
	bool applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const override;
	void apply(BattleStateProxy * battleState, RNG & rng, const Mechanics * m, const EffectTarget & target) const override;

protected:
	void serializeJsonEffect(JsonSerializeFormat & handler) override;

private:
	bool hidden;
	bool passable;
	bool trigger;
	bool trap;
	bool removeOnTrigger;
	bool consistent;
	int32_t patchCount;//random patches to place, only for massive spells
	int32_t turnsRemaining;

	std::array<ObstacleArea, 2> area;
	std::array<ObstacleGraphicsInfo, 2> info;

	static bool isHexAvailable(const CBattleInfoCallback * cb, const BattleHex & hex, const bool mustBeClear);
	static bool noRoomToPlace(Problem & problem, const Mechanics * m);

	void placeObstacles(BattleStateProxy * battleState, const Mechanics * m, const EffectTarget & target, const ObstacleArea & area) const;
};

}
}
