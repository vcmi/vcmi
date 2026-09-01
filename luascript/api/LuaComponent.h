/*
 * LuaComponent.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/ApiTags.h>

#include "SignatureOf.h"

struct Component;

namespace scripting::api
{

/// POD descriptor used by Lua scripts to build a Component (an icon shown in a message window).
struct LuaComponent final : ApiSerializable<LuaComponent>
{
	static constexpr std::string_view luaName = "Component";
	static constexpr std::string_view luaDescription =
		"Descriptor for an icon shown in a message window (a creature, artifact, resource, skill, "
		"... with an optional amount). `type` selects the kind and `subType` the specific entity.";

	int type = -1;   ///< ComponentType index (creature, artifact, resource, ...)
	int subType = 0; ///< identifier index of the entity, interpreted according to `type`
	std::optional<int> value; ///< optional amount (positive = gained, negative = lost)

	Component toComponent() const;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("type",    type,    "Component kind (ComponentType index).");
		s("subType", subType, "Identifier index of the entity, interpreted according to `type`.");
		s("value",   value,   "Optional amount: positive means gained, negative means lost.");
	}
};

}
