/*
 * CalculateActualDamageImpl.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CalculateActualDamageImpl.h"

namespace events
{

std::unique_ptr<CalculateActualDamage> CalculateActualDamage::create()
{
	return make_unique<CalculateActualDamageImpl>();
}

void CalculateActualDamageImpl::execute(const EventBus * bus)
{

}

template class SubscriptionRegistry<CalculateActualDamage>;

};



