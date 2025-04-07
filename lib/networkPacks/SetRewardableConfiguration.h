/*
 * SetRewardableConfiguration.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "NetPacksBase.h"

#include "../rewardable/Configuration.h"

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE SetRewardableConfiguration : public CPackForClient
{
	void applyGs(CGameState * gs) override;
	void visitTyped(ICPackVisitor & visitor) override;

	ObjectInstanceID objectID;
	BuildingID buildingID;
	Rewardable::Configuration configuration;

	template <typename Handler> void serialize(Handler & h)
	{
		h & objectID;
		h & buildingID;
		h & configuration;
	}
};

VCMI_LIB_NAMESPACE_END
