/*
 * api/HeroType.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "HeroType.h"

#include "EntityBindings.h"
#include "../Registry.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

void HeroTypeProxy::registerMethods(MethodRegistrar & R)
{
	EntityBindings<HeroType>::registerMethods(R);
}

}

VCMI_LIB_NAMESPACE_END
