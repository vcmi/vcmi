/*
 * WaterRoutes.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once
#include "../Zone.h"

VCMI_LIB_NAMESPACE_BEGIN

struct RouteInfo;

class WaterRoutes: public Modificator
{
public:
	MODIFICATOR(WaterRoutes);
	
	void process() override;
	void init() override;
	char dump(const int3 &) override;
	
private:
	std::vector<RouteInfo> result;
};

VCMI_LIB_NAMESPACE_END
