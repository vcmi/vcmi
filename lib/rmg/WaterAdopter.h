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

VCMI_LIB_NAMESPACE_BEGIN

class WaterAdopter: public Modificator
{
public:
	MODIFICATOR(WaterAdopter);
	
	void process() override;
	void init() override;
	char dump(const int3 &) override;
	
	
	void setWaterZone(TRmgTemplateZoneId water);
	
	rmg::Area getCoastTiles() const;
	
protected:
	void createWater(EWaterContent::EWaterContent waterContent);
	
protected:
	rmg::Area noWaterArea, waterArea;
	TRmgTemplateZoneId waterZoneId;
	std::map<int3, int> distanceMap;
	std::map<int, rmg::Tileset> reverseDistanceMap;
};

VCMI_LIB_NAMESPACE_END
