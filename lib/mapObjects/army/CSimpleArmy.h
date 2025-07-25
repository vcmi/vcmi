/*
 * CSimpleArmy.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class IArmyDescriptor
{
public:
	virtual void clearSlots() = 0;
	virtual bool setCreature(SlotID slot, CreatureID cre, TQuantity count) = 0;
};

using TSimpleSlots = std::map<SlotID, std::pair<CreatureID, TQuantity>>;

//simplified version of CCreatureSet
class DLL_LINKAGE CSimpleArmy : public IArmyDescriptor
{
public:
	TSimpleSlots army;
	void clearSlots() override
	{
		army.clear();
	}

	bool setCreature(SlotID slot, CreatureID cre, TQuantity count) override
	{
		assert(!vstd::contains(army, slot));
		army[slot] = std::make_pair(cre, count);
		return true;
	}

	operator bool() const
	{
		return !army.empty();
	}

	template<typename Handler>
	void serialize(Handler & h)
	{
		h & army;
	}
};

VCMI_LIB_NAMESPACE_END
