/*
 * api/Creature.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/Creature.h>

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

class CreatureProxy : public RawPointerWrapper<const Creature, CreatureProxy>
{
public:
	static constexpr std::string_view luaName = "Creature";
	static constexpr std::string_view luaDescription =
		"A creature type from the database (e.g. \"pikeman\"). Carries base stats, faction, "
		"recruitment cost, and the bonuses every instance starts with. Concrete battlefield "
		"or army-slot instances are Unit and StackInstance respectively.";

	static void registerMethods(MethodRegistrar & R);

	static std::string getNameTextID(const Creature & creature, int amount);
};

}

VCMI_LIB_NAMESPACE_END
