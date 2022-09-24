/*
 * ObjectVisitEnded.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "ObjectVisitEnded.h"

#include <vcmi/events/EventBus.h>

VCMI_LIB_NAMESPACE_BEGIN


namespace events
{

SubscriptionRegistry<ObjectVisitEnded> * ObjectVisitEnded::getRegistry()
{
	static std::unique_ptr<Sub> Instance = make_unique<Sub>();
	return Instance.get();
}

void ObjectVisitEnded::defaultExecute(const EventBus * bus, const ExecHandler & execHandler,
	const PlayerColor & player, const ObjectInstanceID & heroId)
{
	CObjectVisitEnded event(player, heroId);
	bus->executeEvent(event, execHandler);
}

CObjectVisitEnded::CObjectVisitEnded(const PlayerColor & player_, const ObjectInstanceID & heroId_)
	: player(player_),
	heroId(heroId_)
{

}

bool CObjectVisitEnded::isEnabled() const
{
	return true;
}

PlayerColor CObjectVisitEnded::getPlayer() const
{
	return player;
}

ObjectInstanceID CObjectVisitEnded::getHero() const
{
	return heroId;
}

}

VCMI_LIB_NAMESPACE_END
