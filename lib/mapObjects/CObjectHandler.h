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

/// function object which can be used to find an object with an specific sub ID
class CGObjectInstanceBySubIdFinder
{
public:
	CGObjectInstanceBySubIdFinder(CGObjectInstance * obj);
	bool operator()(CGObjectInstance * obj) const;

private:
	CGObjectInstance * obj;
};

class DLL_LINKAGE CObjectHandler
{
public:
	std::vector<ui32> resVals; //default values of resources in gold

	CObjectHandler();

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & resVals;
	}
};

VCMI_LIB_NAMESPACE_END
