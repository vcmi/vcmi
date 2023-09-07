/*
 * CObjectHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "CObjectHandler.h"

#include "CGObjectInstance.h"
#include "../filesystem/ResourcePath.h"

VCMI_LIB_NAMESPACE_BEGIN

CObjectHandler::CObjectHandler()
{
	logGlobal->trace("\t\tReading resources prices ");
	const JsonNode config2(JsonPath::builtin("config/resources.json"));
	for(const JsonNode &price : config2["resources_prices"].Vector())
	{
		resVals.push_back(static_cast<ui32>(price.Float()));
	}
	logGlobal->trace("\t\tDone loading resource prices!");
}

CGObjectInstanceBySubIdFinder::CGObjectInstanceBySubIdFinder(CGObjectInstance * obj) : obj(obj)
{

}

bool CGObjectInstanceBySubIdFinder::operator()(CGObjectInstance * obj) const
{
	return this->obj->subID == obj->subID;
}

VCMI_LIB_NAMESPACE_END
