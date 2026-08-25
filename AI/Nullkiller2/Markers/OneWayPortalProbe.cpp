/*
 * OneWayPortalProbe.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 */
#include "StdInc.h"

#include "OneWayPortalProbe.h"

namespace NK2AI
{

using namespace Goals;

bool OneWayPortalProbe::operator==(const OneWayPortalProbe & other) const
{
	return tile == other.tile;
}

std::string OneWayPortalProbe::toString() const
{
	return "Probe one-way portal at " + tile.toString();
}

}
