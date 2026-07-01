/*
 * EntityBindings.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/Entity.h>

#include "../MethodRegistrar.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

/// Shared bindings for proxies whose underlying C++ type derives from Entity.
/// Adds the two universal entity accessors so every leaf gets them without copy-paste.
template<class Leaf>
class EntityBindings
{
public:
	static void registerMethods(MethodRegistrar & R)
	{
		R.template method<&Entity::getJsonKey, Leaf>("getJsonKey", {},
			"Returns the JSON key (mod-scoped identifier) of this entity.");
	}
};

}

VCMI_LIB_NAMESPACE_END
