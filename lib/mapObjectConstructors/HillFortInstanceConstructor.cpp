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
#include "IObjectInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

void HillFortInstanceConstructor::initTypeData(const JsonNode & config)
{
	parameters = config;
}

CGObjectInstance * HillFortInstanceConstructor::create(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	HillFort * fort = new HillFort;

	preInitObject(fort);

	if(tmpl)
		fort->appearance = tmpl;

	return fort;
}

void HillFortInstanceConstructor::configureObject(CGObjectInstance * object, CRandomGenerator & rng) const
{
	HillFort * fort = dynamic_cast<HillFort *>(object);

	if(!fort)
		throw std::runtime_error("Unexpected object instance in HillFortInstanceConstructor!");

	fort->upgradeCostPercentage = parameters["upgradeCostFactor"].convertTo<std::vector<int>>();
}

std::unique_ptr<IObjectInfo> HillFortInstanceConstructor::getObjectInfo(std::shared_ptr<const ObjectTemplate> tmpl) const
{
	return nullptr;
}

VCMI_LIB_NAMESPACE_END
