/*
 * WaterAdopter.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "Zone.h"

class DLL_LINKAGE WaterAdopter: public Modificator
{
public:
	MODIFICATOR(WaterAdopter);
	
	void process() override;
	void init() override;
	
	void setWaterZone(TRmgTemplateZoneId water);
	
	rmg::Area getCoastTiles() const;
	
protected:
	void createWater(EWaterContent::EWaterContent waterContent);
	void reinitWaterZone(Zone & zone);
	
protected:
	TRmgTemplateZoneId waterZoneId;
	std::map<int3, int> distanceMap;
	std::map<int, rmg::Tileset> reverseDistanceMap;
};
