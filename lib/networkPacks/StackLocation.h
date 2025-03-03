/*
 * StackLocation.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

struct StackLocation
{
	ObjectInstanceID army;
	SlotID slot;

	StackLocation() = default;
	StackLocation(const ObjectInstanceID & army, const SlotID & slot)
		: army(army)
		, slot(slot)
	{
	}

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & army;
		h & slot;
	}
};

VCMI_LIB_NAMESPACE_END
