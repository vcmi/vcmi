/*
 * PlayerGotTurn.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "PlayerGotTurn.h"

#include <vcmi/events/EventBus.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace events
{

SubscriptionRegistry<PlayerGotTurn> * PlayerGotTurn::getRegistry()
{
	static std::unique_ptr<SubscriptionRegistry<PlayerGotTurn>> Instance = std::make_unique<SubscriptionRegistry<PlayerGotTurn>>();
	return Instance.get();
}

void PlayerGotTurn::defaultExecute(const EventBus * bus, PlayerColor & player)
{
	CPlayerGotTurn event;
	event.setPlayer(player);
	bus->executeEvent(event);
	player = event.getPlayer();
}

CPlayerGotTurn::CPlayerGotTurn() = default;

bool CPlayerGotTurn::isEnabled() const
{
	return true;
}

PlayerColor CPlayerGotTurn::getPlayer() const
{
	return player;
}

void CPlayerGotTurn::setPlayer(const PlayerColor & value)
{
	player = value;
}

int32_t CPlayerGotTurn::getPlayerIndex() const
{
	return player.getNum();
}

void CPlayerGotTurn::setPlayerIndex(int32_t value)
{
	player = PlayerColor(value);
}


}

VCMI_LIB_NAMESPACE_END
