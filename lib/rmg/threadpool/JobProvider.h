/*
 * JobProvider.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "StdInc.h"
#include "../../GameConstants.h"

VCMI_LIB_NAMESPACE_BEGIN

typedef std::function<void()> TRMGfunction ;
typedef std::optional<TRMGfunction> TRMGJob;

class DLL_LINKAGE IJobProvider
{
public:
	//TODO: Think about some mutex protection

	virtual TRMGJob getNextJob() = 0;
	virtual bool hasJobs() = 0;
};

VCMI_LIB_NAMESPACE_END