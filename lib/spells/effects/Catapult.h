/*
 * Catapult.h, part of VCMI engine
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

namespace spells
{
namespace effects
{

class Catapult : public LocationEffect
{
public:
	bool applicable(Problem & problem, const Mechanics * m) const override;
	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;

protected:
	void serializeJsonEffect(JsonSerializeFormat & handler) override;
private:
	int targetsToAttack = 0;
	//Ballistics percentage
	int gate = 0;
	int keep = 0;
	int tower = 0;
	int wall = 0;
	//Damage percentage, used for both ballistics and earthquake
	int hit = 0;
	int crit = 0;
	int noDmg = 0;
	int getCatapultHitChance(EWallPart part) const;
	int getRandomDamage(ServerCallback * server) const;
	void adjustHitChance();
	void applyMassive(ServerCallback * server, const Mechanics * m) const;
	void applyTargeted(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const;
	void removeTowerShooters(ServerCallback * server, const Mechanics * m) const;
	std::vector<EWallPart> getPotentialTargets(const Mechanics * m, bool bypassGateCheck, bool bypassTowerCheck) const;
};

}
}

VCMI_LIB_NAMESPACE_END
