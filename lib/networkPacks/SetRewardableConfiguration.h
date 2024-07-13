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
#include "../mapObjectConstructors/CBankInstanceConstructor.h"

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE SetRewardableConfiguration : public CPackForClient
{
	void applyGs(CGameState * gs);
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

struct DLL_LINKAGE SetBankConfiguration : public CPackForClient
{
	void applyGs(CGameState * gs);
	void visitTyped(ICPackVisitor & visitor) override;

	ObjectInstanceID objectID;
	BankConfig configuration;

	template <typename Handler> void serialize(Handler & h)
	{
		h & objectID;
		h & configuration;
	}
};

VCMI_LIB_NAMESPACE_END
