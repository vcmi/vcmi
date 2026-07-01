/*
 * LuaMetaString.h, part of VCMI engine
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

VCMI_LIB_NAMESPACE_BEGIN

class MetaString;

namespace scripting::api
{

/// POD descriptor used by Lua scripts to build a MetaString.
struct LuaMetaString final : ApiSerializable<LuaMetaString>
{
	static constexpr std::string_view luaName = "MetaString";
	static constexpr std::string_view luaDescription =
		"Object for constructing strings with translation support. Supports appending text via `append` field and "
		"`replaceStrings` / `replaceNumbers` to fill the %s and %d placeholders. Used in all places that expect translatable string with formatting"
		"such as battle log and spell-problem messages. In case of multiplayer, string is translated on each client according to their language settings";

	std::vector<std::string> append;
	std::vector<std::string> replaceStrings;
	std::vector<int64_t>     replaceNumbers;

	MetaString toMetaString() const;

	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("append",         append,         "Sequence of text-ID tokens to concatenate.");
		s("replaceStrings", replaceStrings, "Values that fill %s placeholders in the appended tokens, in order.");
		s("replaceNumbers", replaceNumbers, "Values that fill %d placeholders in the appended tokens, in order.");
	}
};

}

VCMI_LIB_NAMESPACE_END
