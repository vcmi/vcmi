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
#include "../../GameConstants.h"
#include "../../battle/BattleHex.h"
#include "../../battle/CObstacleInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

class ObstacleSideOptions
{
public:
	using RelativeShape = std::vector<std::vector<BattleHex::EDir>>;

	RelativeShape shape; //shape of single obstacle relative to obstacle position
	RelativeShape range; //position of obstacles relative to effect destination

	std::string appearSound;
	AnimationPath appearAnimation;
	AnimationPath animation;

	int offsetY = 0;

	void serializeJson(JsonSerializeFormat & handler);
};

class Obstacle : public LocationEffect
{
public:
	void adjustAffectedHexes(std::set<BattleHex> & hexes, const Mechanics * m, const Target & spellTarget) const override;

	bool applicable(Problem & problem, const Mechanics * m) const override;
	bool applicable(Problem & problem, const Mechanics * m, const EffectTarget & target) const override;

	EffectTarget transformTarget(const Mechanics * m, const Target & aimPoint, const Target & spellTarget) const override;

	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;

protected:
	void serializeJsonEffect(JsonSerializeFormat & handler) override;
	virtual void placeObstacles(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const;

	bool hidden = false;
	bool trigger = false;
	bool trap = false;
	bool removeOnTrigger = false;
	bool hideNative = false;
	SpellID triggerAbility;
private:
	int32_t patchCount = 0; //random patches to place, for massive spells should be >= 1, for non-massive ones if >= 1, then place only this number inside a target (like H5 landMine)
	bool passable = false;
	int32_t turnsRemaining = -1;

	std::array<ObstacleSideOptions, 2> sideOptions;

	static bool isHexAvailable(const CBattleInfoCallback * cb, const BattleHex & hex, const bool mustBeClear);
	static bool noRoomToPlace(Problem & problem, const Mechanics * m);
};

}
}

VCMI_LIB_NAMESPACE_END
