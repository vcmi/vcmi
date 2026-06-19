/*
 * HeroInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "HeroInstance.h"

#include "../Registry.h"
#include "../library/BonusBearerBindings.h"

#include "StackInstance.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

void HeroInstanceProxy::registerMethods(MethodRegistrar & R)
{
	BonusBearerBindings<CGHeroInstance>::registerMethods(R);

	R.method<&CCreatureSet::getStackPtr, CGHeroInstance>("getStack",
		{{"slot", "Army slot to query (1-based)."}}, {},
		"Returns the stack instance in the given army slot, or nil if the slot is empty.");
	R.method<&CGObjectInstance::getOwner, CGHeroInstance>("getOwner", {},
		"Returns the player color that owns this hero.");
	R.method<&CGHeroInstance::getNameTextID>("getNameTextID", {},
		"Returns the text ID of the hero's name.");
	R.function<&HeroInstanceProxy::isMale>("isMale", {},
		"True if the hero's gender is male.");
	R.function<&HeroInstanceProxy::isFemale>("isFemale", {},
		"True if the hero's gender is female.");
}

bool HeroInstanceProxy::isMale(const CGHeroInstance & hero)
{
	return hero.gender == EHeroGender::MALE;
}

bool HeroInstanceProxy::isFemale(const CGHeroInstance & hero)
{
	return hero.gender == EHeroGender::FEMALE;
}

}

VCMI_LIB_NAMESPACE_END
