/*
 * BattleEvents.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/events/ApplyDamage.h>

#include "../../LuaWrapper.h"

#include "EventBusProxy.h"

namespace scripting
{
namespace api
{
namespace events
{

class ApplyDamageProxy : public OpaqueWrapperEx<::events::ApplyDamage, ApplyDamageProxy>
{
public:
	using Wrapper = OpaqueWrapperEx<::events::ApplyDamage, ApplyDamageProxy>;
	static const std::vector<typename Wrapper::RegType> REGISTER;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
};

}
}
}


