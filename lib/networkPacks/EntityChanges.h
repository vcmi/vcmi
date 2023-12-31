/*
 * EInfoWindowMode.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/Metatype.h>

#include "../JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class EntityChanges
{
public:
	Metatype metatype = Metatype::UNKNOWN;
	int32_t entityIndex = 0;
	JsonNode data;
	template <typename Handler> void serialize(Handler & h, const int version)
	{
		h & metatype;
		h & entityIndex;
		h & data;
	}
};

VCMI_LIB_NAMESPACE_END

