/*
 * GameResumed.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "GameResumed.h"

#include <vcmi/events/EventBus.h>

namespace events
{

SubscriptionRegistry<GameResumed> * GameResumed::getRegistry()
{
	static std::unique_ptr<SubscriptionRegistry<GameResumed>> Instance = make_unique<SubscriptionRegistry<GameResumed>>();
	return Instance.get();
}

void GameResumed::defaultExecute(const EventBus * bus)
{
	CGameResumed event;
	bus->executeEvent(event);
}

CGameResumed::CGameResumed() = default;

bool CGameResumed::isEnabled() const
{
	return true;
}


}
