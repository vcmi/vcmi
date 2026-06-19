/*
 * api/SpellSchool.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "SpellSchool.h"

#include "EntityBindings.h"
#include "../Registry.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

void SpellSchoolProxy::registerMethods(MethodRegistrar & R)
{
	EntityBindings<spells::SpellSchoolType>::registerMethods(R);
	R.method<&Entity::getNameTextID, spells::SpellSchoolType>("getNameTextID", {},
		"Returns the text ID of the spell school name.");
}

}

VCMI_LIB_NAMESPACE_END
