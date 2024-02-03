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

#include "../ConstTransitivePtr.h"
#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class CArmedInstance;
class CStackInstance;

struct StackLocation
{
	ConstTransitivePtr<CArmedInstance> army;
	SlotID slot;

	StackLocation() = default;
	StackLocation(const CArmedInstance * Army, const SlotID & Slot)
		: army(const_cast<CArmedInstance *>(Army))  //we are allowed here to const cast -> change will go through one of our packages... do not abuse!
		, slot(Slot)
	{
	}

	DLL_LINKAGE const CStackInstance * getStack();
	template <typename Handler> void serialize(Handler & h)
	{
		h & army;
		h & slot;
	}
};

VCMI_LIB_NAMESPACE_END
