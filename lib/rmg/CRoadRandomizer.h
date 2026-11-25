/*
 * CRoadRandomizer.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "CRmgTemplate.h"

namespace vstd
{
class RNG;
}

class CRoadRandomizer
{
public:
	explicit CRoadRandomizer(CRmgTemplate::Zones zones);
	~CRoadRandomizer() = default;

	void dropRandomRoads(vstd::RNG & rand);

private:
	CRmgTemplate::Zones zones;
};
