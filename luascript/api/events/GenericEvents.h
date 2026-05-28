/*
 * GenericEvents.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/events/GenericEvents.h>

#include "../../LuaWrapper.h"

#include "EventBusProxy.h"

VCMI_LIB_NAMESPACE_BEGIN

namespace scripting
{
namespace api
{
namespace events
{

class GameResumedProxy : public RawPointerWrapper<::events::GameResumed, GameResumedProxy>
{
public:
	using Wrapper = RawPointerWrapper<::events::GameResumed, GameResumedProxy>;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
};

class PlayerGotTurnProxy : public RawPointerWrapper<::events::PlayerGotTurn, PlayerGotTurnProxy>
{
public:
	using Wrapper = RawPointerWrapper<::events::PlayerGotTurn, PlayerGotTurnProxy>;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
};

class TurnStartedProxy : public RawPointerWrapper<::events::TurnStarted, TurnStartedProxy>
{
public:
	using Wrapper = RawPointerWrapper<::events::TurnStarted, TurnStartedProxy>;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
};

}
}
}

VCMI_LIB_NAMESPACE_END
