/*
 * SpellObstacleDescriptor.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "SpellObstacleDescriptor.h"

#include "../../../lib/battle/CObstacleInstance.h"
#include "../../../lib/filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

SpellCreatedObstacle SpellObstacleDescriptor::toObstacle() const
{
	SpellCreatedObstacle obstacle;
	obstacle.pos              = pos;
	obstacle.obstacleType     = obstacleType;
	obstacle.ID               = spell ? spell->getId() : SpellID(SpellID::NONE);
	obstacle.turnsRemaining   = turnsRemaining;
	obstacle.casterSpellPower = casterSpellPower;
	obstacle.spellLevel       = spellLevel;
	obstacle.casterSide       = casterSide;
	obstacle.minimalDamage    = minimalDamage;

	obstacle.hidden          = hidden;
	obstacle.passable        = passable;
	obstacle.trap            = trap;
	obstacle.removeOnTrigger = removeOnTrigger;
	obstacle.nativeVisible   = nativeVisible;

	obstacle.trigger = trigger.empty() ? SpellID(SpellID::NONE) : SpellID(SpellID::decode(trigger));

	obstacle.appearSound     = AudioPath::builtin(appearSound);
	obstacle.appearAnimation = AnimationPath::builtin(appearAnimation);
	obstacle.animation       = AnimationPath::builtin(animation);

	for(const BattleHex & hex : customSize)
		obstacle.customSize.insert(hex);

	return obstacle;
}

}

VCMI_LIB_NAMESPACE_END
