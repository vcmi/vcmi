/*
* HillFortInstanceConstructor.cpp, part of VCMI engine
*
* Authors: listed in file AUTHORS in main folder
*
* License: GNU General Public License v2.0 or later
* Full text of license available in license.txt file, in main folder
*
*/
#include "StdInc.h"
#include "HillFortInstanceConstructor.h"

#include "../mapObjects/MiscObjects.h"

VCMI_LIB_NAMESPACE_BEGIN

void HillFortInstanceConstructor::initTypeData(const JsonNode & config)
{
	parameters = config;
}

void HillFortInstanceConstructor::initializeObject(HillFort * fort) const
{
	fort->upgradeCostPercentage = parameters["upgradeCostFactor"].convertTo<std::vector<int>>();
}

VCMI_LIB_NAMESPACE_END
