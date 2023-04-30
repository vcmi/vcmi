/*
 * Configuration.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "Configuration.h"

VCMI_LIB_NAMESPACE_BEGIN

Rewardable::EVisitMode Rewardable::Configuration::getVisitMode() const
{
	return static_cast<EVisitMode>(visitMode);
}

ui16 Rewardable::Configuration::getResetDuration() const
{
	return resetParameters.period;
}

VCMI_LIB_NAMESPACE_END
