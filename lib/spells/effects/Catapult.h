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

VCMI_LIB_NAMESPACE_BEGIN

namespace spells
{
namespace effects
{

class Catapult : public LocationEffect
{
public:
	Catapult();
	virtual ~Catapult();

	bool applicable(Problem & problem, const Mechanics * m) const override;
	void apply(ServerCallback * server, const Mechanics * m, const EffectTarget & target) const override;

protected:
	void serializeJsonEffect(JsonSerializeFormat & handler) override;
private:
	int targetsToAttack;
};

}
}

VCMI_LIB_NAMESPACE_END
