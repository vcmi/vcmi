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

#include "../LuaWrapper.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{

class ArtifactProxy : public OpaqueWrapper<const Artifact, ArtifactProxy>
{
public:
	using Wrapper = OpaqueWrapper<const Artifact, ArtifactProxy>;

	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
};

}
}

VCMI_LIB_NAMESPACE_END
