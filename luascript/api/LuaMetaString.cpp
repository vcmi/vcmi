/*
 * LuaMetaString.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LuaMetaString.h"

#include "../../lib/texts/MetaString.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

MetaString LuaMetaString::toMetaString() const
{
	MetaString result;
	for(const auto & textID : append)
		result.appendTextID(textID);
	for(const auto & textID : replaceStrings)
		result.replaceTextID(textID);
	for(int64_t number : replaceNumbers)
		result.replaceNumber(number);
	return result;
}

}

VCMI_LIB_NAMESPACE_END
