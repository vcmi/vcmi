/*
 * TownFake.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../../lib/constants/EntityIdentifiers.h"
#include "../../lib/mapObjects/CGTownInstance.h"

namespace test
{

class TownFake
{
public:
	explicit TownFake(FactionID faction = FactionID(FactionID::CASTLE))
		: town(std::make_shared<CGTownInstance>(nullptr))
	{
		town->subID = faction;
	}

	TownFake & withBuilding(BuildingID building)
	{
		town->addBuilding(building);
		return *this;
	}

	CGTownInstance * get() { return town.get(); }
	const CGTownInstance * get() const { return town.get(); }

private:
	std::shared_ptr<CGTownInstance> town;
};

}
