/*
 * GenericEvents.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "GenericEvents.h"

#include "../../LuaStack.h"
#include "../../LuaCallWrapper.h"
#include "../Registry.h"
#include "SubscriptionRegistryProxy.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
namespace events
{
using ::events::GameResumed;
using ::events::PlayerGotTurn;
using ::events::TurnStarted;

VCMI_REGISTER_SCRIPT_API(GameResumedProxy, "events.GameResumed");
VCMI_REGISTER_SCRIPT_API(PlayerGotTurnProxy, "events.PlayerGotTurn");
VCMI_REGISTER_SCRIPT_API(TurnStartedProxy, "events.TurnStarted");

const std::vector<GameResumedProxy::CustomRegType> GameResumedProxy::REGISTER_CUSTOM =
{
	{
		"subscribeBefore",
		&SubscriptionRegistryProxy<GameResumedProxy>::subscribeBefore,
		true
	},
	{
		"subscribeAfter",
		&SubscriptionRegistryProxy<GameResumedProxy>::subscribeAfter,
		true
	}
};

const std::vector<PlayerGotTurnProxy::CustomRegType> PlayerGotTurnProxy::REGISTER_CUSTOM =
{
	{
		"subscribeBefore",
		&SubscriptionRegistryProxy<PlayerGotTurnProxy>::subscribeBefore,
		true
	},
	{
		"subscribeAfter",
		&SubscriptionRegistryProxy<PlayerGotTurnProxy>::subscribeAfter,
		true
	},
	{
		"getPlayer",
		LuaMethodWrapper<PlayerGotTurn, decltype(&PlayerGotTurn::getPlayerIndex), &PlayerGotTurn::getPlayerIndex>::invoke,
		false
	},
	{
		"setPlayer",
		LuaMethodWrapper<PlayerGotTurn, decltype(&PlayerGotTurn::setPlayerIndex), &PlayerGotTurn::setPlayerIndex>::invoke,
		false
	},
};

const std::vector<TurnStartedProxy::CustomRegType> TurnStartedProxy::REGISTER_CUSTOM =
{
	{
		"subscribeBefore",
		&SubscriptionRegistryProxy<TurnStartedProxy>::subscribeBefore,
		true
	},
	{
		"subscribeAfter",
		&SubscriptionRegistryProxy<TurnStartedProxy>::subscribeAfter,
		true
	}
};

}
}
}


VCMI_LIB_NAMESPACE_END
