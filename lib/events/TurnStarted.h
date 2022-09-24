/*
 * TurnStarted.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include <vcmi/events/TurnStarted.h>

VCMI_LIB_NAMESPACE_BEGIN

namespace events
{

class DLL_LINKAGE CTurnStarted : public TurnStarted
{
public:
	CTurnStarted();
	bool isEnabled() const override;
	static void defaultExecute(const EventBus * bus);
};

}

VCMI_LIB_NAMESPACE_END
