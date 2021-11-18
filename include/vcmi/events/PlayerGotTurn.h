/*
 * PlayerGotTurn.h, part of VCMI engine
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

class PlayerColor;

namespace events
{

class DLL_LINKAGE PlayerGotTurn : public Event
{
public:
	using Sub = SubscriptionRegistry<PlayerGotTurn>;
	using PreHandler = Sub::PreHandler;
	using PostHandler = Sub::PostHandler;
	using ExecHandler = Sub::ExecHandler;

	static Sub * getRegistry();
	static void defaultExecute(const EventBus * bus, const ExecHandler & execHandler, PlayerColor & player);

	virtual PlayerColor getPlayer() const = 0;
	virtual void setPlayer(const PlayerColor & value) = 0;

	virtual int32_t getPlayerIndex() const = 0;
	virtual void setPlayerIndex(int32_t value) = 0;

	friend class SubscriptionRegistry<PlayerGotTurn>;
};

}
