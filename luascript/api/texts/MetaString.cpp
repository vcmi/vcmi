/*
 * MetaString.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "MetaString.h"

#include "../Registry.h"
#include "../../../lib/constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

	namespace scripting
{
	namespace api
	{
		VCMI_REGISTER_CORE_SCRIPT_API(MetaStringProxy, "texts.MetaString");

		const std::vector<MetaStringProxy::CustomRegType> MetaStringProxy::REGISTER_CUSTOM =
			{
				{"new", &Wrapper::constructor, true},

				{"appendTextID", LuaMethodWrapper<MetaString, decltype(&MetaString::appendTextID), &MetaString::appendTextID>::invoke, false},
				{"appendRawString", LuaMethodWrapper<MetaString, decltype(&MetaString::appendRawString), &MetaString::appendRawString>::invoke, false},
				{"appendNumber", LuaMethodWrapper<MetaString, decltype(&MetaString::appendNumber), &MetaString::appendNumber>::invoke, false},

				{"replaceTextID", LuaMethodWrapper<MetaString, decltype(&MetaString::replaceTextID), &MetaString::replaceTextID>::invoke, false},
				{"replaceRawString", LuaMethodWrapper<MetaString, decltype(&MetaString::replaceRawString), &MetaString::replaceRawString>::invoke, false},
				{"replaceNumber", LuaMethodWrapper<MetaString, decltype(&MetaString::replaceNumber), &MetaString::replaceNumber>::invoke, false},

				{"replaceNamePlural", LuaMethodWrapper<MetaString, decltype(&MetaString::replaceNamePlural), &MetaString::replaceNamePlural>::invoke, false},
			};
	}
}

VCMI_LIB_NAMESPACE_END
