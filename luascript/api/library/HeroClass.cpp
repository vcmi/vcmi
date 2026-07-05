/*
 * api/HeroClass.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "HeroClass.h"

#include "EntityBindings.h"
#include "../Registry.h"

namespace scripting::api
{

void HeroClassProxy::registerMethods(MethodRegistrar & R)
{
	EntityBindings<HeroClass>::registerMethods(R);
}

}
