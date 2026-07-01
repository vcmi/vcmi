/*
 * StackInstance.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "StackInstance.h"

#include "../Registry.h"

#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"
#include <vcmi/Creature.h>

#include "../library/Creature.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{
void StackInstanceProxy::registerMethods(MethodRegistrar & R)
{
	R.method<&CStackBasicDescriptor::getType, CStackInstance>("getType", {},
		"Returns the Creature type of the units in this stack.");
	R.method<&CStackBasicDescriptor::getCount, CStackInstance>("getCount", {},
		"Returns the number of creatures currently in this stack.");
}


}

VCMI_LIB_NAMESPACE_END
