/*
 * api/Artifact.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "Artifact.h"

#include "EntityBindings.h"
#include "../Registry.h"
#include "../../../lib/bonuses/IBonusBearer.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

void ArtifactProxy::registerMethods(MethodRegistrar & R)
{
	EntityBindings<Artifact>::registerMethods(R);

	R.method<&Artifact::isBig>("isBig", {},
		"True if the artifact occupies the 'big' artifact slot (cannot be carried in the backpack).");
	R.method<&Artifact::isTradable>("isTradable", {},
		"True if the artifact can be traded between heroes or sold at the marketplace.");
	R.method<&Artifact::getPrice>("getPrice", {},
		"Returns the artifact's gold value.");
}

}

VCMI_LIB_NAMESPACE_END
