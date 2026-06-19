/*
 * Obstacle.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"

#include "Obstacle.h"

#include "../Enums.h"
#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"
#include "../Registry.h"

#include "BattleHex.h"
#include "../library/Spell.h"

#include "../../../lib/constants/EntityIdentifiers.h"
#include "../../../lib/GameLibrary.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

void ObstacleProxy::registerMethods(MethodRegistrar & R)
{
	R.function<&ObstacleProxy::getObstacleType>("getObstacleType", {},
		"Returns the obstacle category: usual, absolute, moat, or spell-created.");
	R.function<&ObstacleProxy::getPosition>("getPosition", {},
		"Returns the hex that serves as anchor of the obstacle, usually - located in bottom-left corner of the obstacle");
	R.function<&ObstacleProxy::getSpell>("getSpell", {},
		"Returns the Spell that created this obstacle, or nil for non-spell obstacles.");
}

CObstacleInstance::EObstacleType ObstacleProxy::getObstacleType(std::shared_ptr<const CObstacleInstance> obstacle)
{
	return obstacle->obstacleType;
}

BattleHex ObstacleProxy::getPosition(std::shared_ptr<const CObstacleInstance> obstacle)
{
	return obstacle->pos;
}

const spells::Spell * ObstacleProxy::getSpell(std::shared_ptr<const CObstacleInstance> obstacle)
{
	if(obstacle->obstacleType != CObstacleInstance::SPELL_CREATED)
		return nullptr;
	return SpellID(obstacle->ID).toEntity(LIBRARY);
}

}

VCMI_LIB_NAMESPACE_END
