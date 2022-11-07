/*
 * BattleFake.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/rmg/CMapGenOptions.h"
#include "../../../lib/rmg/CMapGenerator.h"

#pragma once

class ZoneOptionsFake : public rmg::ZoneOptions
{
public:
	void setOwner(int ow)
	{
		this->owner = ow;
	}
};