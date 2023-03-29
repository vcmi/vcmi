/*
 * Moat.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Obstacle.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

class Moat : public Obstacle
{
private:
	ObstacleSideOptions sideOptions; //Defender only
	std::vector<std::vector<BattleHex>> moatHexes; //Determine number of moat patches and hexes
	std::vector<std::shared_ptr<Bonus>> bonus; //For battle-wide bonuses
	bool dispellable; //For Tower landmines
	int moatDamage; // Minimal moat damage
public:
	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;
protected:
	void serializeJsonEffect(JsonSerializeFormat & handler) override;
	void placeObstacles(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;
	void convertBonus(const Mechanics * m, std::vector<Bonus> & converted) const;
};

}
}

VCMI_LIB_NAMESPACE_END
