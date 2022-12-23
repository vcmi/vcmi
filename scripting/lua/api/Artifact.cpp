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

#include "Registry.h"

#include "../LuaStack.h"
#include "../LuaCallWrapper.h"
#include "../../../lib/HeroBonus.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{

VCMI_REGISTER_CORE_SCRIPT_API(ArtifactProxy, "Artifact");

const std::vector<ArtifactProxy::CustomRegType> ArtifactProxy::REGISTER_CUSTOM =
{
	{"getIconIndex", LuaMethodWrapper<Artifact, decltype(&Entity::getIconIndex), &Entity::getIconIndex>::invoke, false},
	{"getIndex", LuaMethodWrapper<Artifact, decltype(&Entity::getIndex), &Entity::getIndex>::invoke, false},
	{"getJsonKey", LuaMethodWrapper<Artifact, decltype(&Entity::getJsonKey), &Entity::getJsonKey>::invoke, false},
	{"getName", LuaMethodWrapper<Artifact, decltype(&Entity::getName), &Entity::getName>::invoke, false},

	{"getId", LuaMethodWrapper<Artifact, decltype(&EntityT<ArtifactID>::getId), &EntityT<ArtifactID>::getId>::invoke, false},
	{"accessBonuses", LuaMethodWrapper<Artifact, decltype(&EntityWithBonuses<ArtifactID>::accessBonuses), &EntityWithBonuses<ArtifactID>::accessBonuses>::invoke, false},

	{"getDescription", LuaMethodWrapper<Artifact, decltype(&Artifact::getDescription), &Artifact::getDescription>::invoke, false},
	{"getEventText", LuaMethodWrapper<Artifact, decltype(&Artifact::getEventText), &Artifact::getEventText>::invoke, false},
	{"isBig", LuaMethodWrapper<Artifact, decltype(&Artifact::isBig), &Artifact::isBig>::invoke, false},
	{"isTradable", LuaMethodWrapper<Artifact, decltype(&Artifact::isTradable), &Artifact::isTradable>::invoke, false},
	{"getPrice", LuaMethodWrapper<Artifact, decltype(&Artifact::getPrice), &Artifact::getPrice>::invoke, false},
};


}
}

VCMI_LIB_NAMESPACE_END
