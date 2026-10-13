/*
 * TargetStepDescriptor.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/ApiTags.h>

#include "../LuaMetaString.h"

namespace spells::effects
{
struct EffectTargetStep;
}

namespace scripting::api
{

/// POD descriptor returned by the describeTargetStep function of a spell effect script.
struct TargetStepDescriptor final : ApiSerializable<TargetStepDescriptor>
{
	static constexpr std::string_view luaName = "TargetStepDescriptor";
	static constexpr std::string_view luaDescription =
		"Cursor and status bar texts that the game shows while the player chooses one destination of a spell target. "
		"Returned by the `describeTargetStep` function of a spell effect script.";

	std::string cursor;
	LuaMetaString text;
	LuaMetaString blockedText;
	std::string healthGainTextID;

	::spells::effects::EffectTargetStep toTargetStep() const;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("cursor",           cursor,           "Identifier of the cursor from `config/cursors.json` shown over a hex that is valid for the destination.");
		s("text",             text,             "Status bar text for a hex that is valid for the destination.");
		s("blockedText",      blockedText,      "Status bar text for a hex that is not valid for the destination. No text if not set.");
		s("healthGainTextID", healthGainTextID, "If set, the health change of the spell at the hovered hex is appended to `text` in brackets. A health gain uses the plural forms of this text ID.");
	}
};

}
