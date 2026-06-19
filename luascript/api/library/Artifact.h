/*
 * api/Artifact.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/Artifact.h>

#include "../../LuaWrapper.h"
#include "../MethodRegistrar.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

class ArtifactProxy : public RawPointerWrapper<const Artifact, ArtifactProxy>
{
public:
	static constexpr std::string_view luaName = "Artifact";
	static constexpr std::string_view luaDescription =
		"A static artifact definition from the game database. "
		"Identifies an artifact kind; concrete instances worn by heroes are a separate concept "
		"not currently exposed to scripts.";

	static void registerMethods(MethodRegistrar & R);
};

}

VCMI_LIB_NAMESPACE_END
