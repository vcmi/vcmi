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

#include <vcmi/scripting/Service.h>
#include <vcmi/spells/Spell.h>

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"
#include "../../../lib/battle/CObstacleInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

class ObstacleProxy : public SharedPointerWrapper<const CObstacleInstance, ObstacleProxy>
{
public:
	static constexpr std::string_view luaName = "Obstacle";
	static constexpr std::string_view luaDescription =
		"A battlefield obstacle (static map decoration, moat tile, or spell-created hazard). "
		"Exposed read-only — to add or remove obstacles use the `server:addObstacle` or "
		"`server:removeObstacle` methods.";

	static void registerMethods(MethodRegistrar & R);

	static CObstacleInstance::EObstacleType getObstacleType(std::shared_ptr<const CObstacleInstance> obstacle);
	static BattleHex getPosition(std::shared_ptr<const CObstacleInstance> obstacle);
	static const ::spells::Spell * getSpell(std::shared_ptr<const CObstacleInstance> obstacle);
};

}

VCMI_LIB_NAMESPACE_END
