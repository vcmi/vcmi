/*
 * api/Creature.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Creature.h"

#include "EntityBindings.h"
#include "../Registry.h"
#include "../../../lib/bonuses/IBonusBearer.h"
#include "../../../lib/constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

void CreatureProxy::registerMethods(MethodRegistrar & R)
{
	EntityBindings<Creature>::registerMethods(R);

	R.method<&Creature::getMaxHealth, Creature>("getMaxHealth", {},
		"Returns the base maximum hit points of a single creature of this type. Equivalent to getBaseHitPoints.");
	R.function<&CreatureProxy::getNameTextID>("getNameTextID",
		{{"amount", "Stack size; passes through to the singular form when equal to 1, plural otherwise."}}, {},
		"Select either the singular or plural text ID for the requested amount.");
	R.method<&Creature::getAdvMapAmountMin>("getMapAmountMin", {},
		"Returns the minimum stack size for this creature when generated on the adventure map.");
	R.method<&Creature::getAdvMapAmountMax>("getMapAmountMax", {},
		"Returns the maximum stack size for this creature when generated on the adventure map.");
	R.method<&Creature::getAIValue>("getAIValue", {},
		"Returns the AI value used for army strength calculations.");
	R.method<&Creature::getFightValue>("getFightValue", {},
		"Returns the combat power value of one creature of this type.");
	R.method<&Creature::getLevel>("getLevel", {},
		"Returns the creature level, usually in (1..7) range.");
	R.method<&Creature::getGrowth>("getGrowth", {},
		"Returns the base weekly growth rate for this creature in town dwellings.");
	R.method<&Creature::getHorde>("getHorde", {},
		"Returns the extra growth granted by the horde building (0 if none).");

	R.method<&Creature::getRecruitCost>("getRecruitCost",
		{{"resourceID", "JSON key of the resource whose cost is being queried (e.g. `gold`, `ore`)."}}, {},
		"Returns the recruitment cost in the specified resource.");
	R.method<&Creature::isDoubleWide>("isDoubleWide", {},
		"True if the creature occupies two hexes on the battlefield.");
}

std::string CreatureProxy::getNameTextID(const Creature & creature, int amount)
{
	return amount == 1 ? creature.getNameSingularTextID() : creature.getNamePluralTextID();
}

}

VCMI_LIB_NAMESPACE_END
