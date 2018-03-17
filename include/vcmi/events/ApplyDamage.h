/*
 * ApplyDamage.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "Event.h"
#include "SubscriptionRegistry.h"

struct BattleStackAttacked;

namespace battle
{
	class Unit;
}

namespace events
{

class DLL_LINKAGE ApplyDamage : public Event
{
public:
	using PreHandler = SubscriptionRegistry<ApplyDamage>::PreHandler;
	using PostHandler = SubscriptionRegistry<ApplyDamage>::PostHandler;
	using BusTag = SubscriptionRegistry<ApplyDamage>::BusTag;

	static SubscriptionRegistry<ApplyDamage> * getRegistry();

	virtual int64_t getInitalDamage() const = 0;
	virtual int64_t getDamage() const = 0;
	virtual void setDamage(int64_t value) = 0;
	virtual const battle::Unit * getTarget() const = 0;

	friend class SubscriptionRegistry<ApplyDamage>;

};

}
