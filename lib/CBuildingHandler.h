/*
 * CBuildingHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class DLL_LINKAGE CBuildingHandler
{
public:
	static BuildingID campToERMU(int camp, int townType, const std::set<BuildingID> & builtBuildings);
};

VCMI_LIB_NAMESPACE_END
