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

VCMI_LIB_NAMESPACE_BEGIN

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
	using Sub = SubscriptionRegistry<ApplyDamage>;

	using PreHandler = Sub::PreHandler;
	using PostHandler = Sub::PostHandler;
	using ExecHandler = Sub::ExecHandler;

	static Sub * getRegistry();

	virtual int64_t getInitalDamage() const = 0;
	virtual int64_t getDamage() const = 0;
	virtual void setDamage(int64_t value) = 0;
	virtual const battle::Unit * getTarget() const = 0;

	friend class SubscriptionRegistry<ApplyDamage>;
};

}

VCMI_LIB_NAMESPACE_END
