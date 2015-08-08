/*
 * MapComparer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
 
#pragma once
 
#include "../lib/mapping/CMap.h"

struct MapComparer
{
	bool operator() (const std::unique_ptr<CMap>& lhs, const std::unique_ptr<CMap>& rhs);
};


