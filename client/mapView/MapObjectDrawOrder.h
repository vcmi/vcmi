/*
 * MapObjectDrawOrder.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../lib/GameConstants.h"

class CMap;
class CGObjectInstance;
class int3;

namespace MapObjectDrawOrder
{
	/// Puts the object into the list of a tile the way H3 does when it stamps an object onto the map
	void insert(std::vector<ObjectInstanceID> & container, const CMap & map, const CGObjectInstance * object, const int3 & tile);
}
