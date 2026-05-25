/*
 * Enums.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/scripting/ApiTags.h>

#include "../../lib/constants/Enumerations.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting::api
{

class Enums : public scripting::ApiSerializable<Enums>
{
	template<typename EnumType>
	using EnumMap = std::map<std::string, EnumType>;

	EnumMap<EHealLevel> exportHealLevel() const;
	EnumMap<EHealPower> exportHealPower() const;

public:
	template<typename Serializer>
	void serializeScript(Serializer & s)
	{
		s("HealLevel", exportHealLevel());
		s("HealPower", exportHealPower());
	}
};

}

VCMI_LIB_NAMESPACE_END
