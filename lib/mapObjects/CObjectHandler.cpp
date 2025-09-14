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
#include "../GameLibrary.h"
#include "../entities/ResourceTypeHandler.h"
#include "../filesystem/ResourcePath.h"
#include "../json/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

CObjectHandler::CObjectHandler()
{
	logGlobal->trace("\t\tReading resources prices ");
	for(auto & res : LIBRARY->resourceTypeHandler->getAllObjects())
	{
		auto resType = LIBRARY->resourceTypeHandler->getById(res);
		resVals[res] = static_cast<ui32>(resType->getPrice());
	}
	logGlobal->trace("\t\tDone loading resource prices!");
}

VCMI_LIB_NAMESPACE_END
