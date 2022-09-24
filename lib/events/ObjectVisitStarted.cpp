/*
 * ObjectVisitStarted.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "ObjectVisitStarted.h"

#include <vcmi/events/EventBus.h>

VCMI_LIB_NAMESPACE_BEGIN


namespace events
{

SubscriptionRegistry<ObjectVisitStarted> * ObjectVisitStarted::getRegistry()
{
	static std::unique_ptr<Sub> Instance = make_unique<Sub>();
	return Instance.get();
}

void ObjectVisitStarted::defaultExecute(const EventBus * bus, const ExecHandler & execHandler,
	const PlayerColor & player, const ObjectInstanceID & heroId, const ObjectInstanceID & objId)
{
	CObjectVisitStarted event(player, heroId, objId);
	bus->executeEvent(event, execHandler);
}

CObjectVisitStarted::CObjectVisitStarted(const PlayerColor & player_, const ObjectInstanceID & heroId_, const ObjectInstanceID & objId_)
	: player(player_),
	heroId(heroId_),
	objId(objId_),
	enabled(true)
{
}

PlayerColor CObjectVisitStarted::getPlayer() const
{
	return player;
}

ObjectInstanceID CObjectVisitStarted::getHero() const
{
	return heroId;
}

ObjectInstanceID CObjectVisitStarted::getObject() const
{
	return objId;
}

bool CObjectVisitStarted::isEnabled() const
{
	return enabled;
}

void CObjectVisitStarted::setEnabled(bool enable)
{
	enabled = enable;
}


}

VCMI_LIB_NAMESPACE_END
