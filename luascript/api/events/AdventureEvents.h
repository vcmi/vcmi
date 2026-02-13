/*
 * AdventureEvents.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/events/AdventureEvents.h>

#include "../../LuaWrapper.h"

#include "EventBusProxy.h"

namespace scripting
{
namespace api
{
namespace events
{

using ::events::ObjectVisitStarted;

class ObjectVisitStartedProxy : public OpaqueWrapper<ObjectVisitStarted, ObjectVisitStartedProxy>
{
public:
	using Wrapper = OpaqueWrapper<ObjectVisitStarted, ObjectVisitStartedProxy>;
	static const std::vector<typename Wrapper::CustomRegType> REGISTER_CUSTOM;
};


}
}
}



VCMI_LIB_NAMESPACE_END
