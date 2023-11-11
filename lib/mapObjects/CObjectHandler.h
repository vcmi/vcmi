/*
 * CObjectHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

class CGObjectInstance;
class int3;

class DLL_LINKAGE CObjectHandler
{
public:
	std::vector<ui32> resVals; //default values of resources in gold

	CObjectHandler();
};

VCMI_LIB_NAMESPACE_END
