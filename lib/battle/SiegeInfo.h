/*
 * SiegeInfo.h, part of VCMI engine
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

//only for use in BattleInfo
struct DLL_LINKAGE SiegeInfo
{
	std::map<EWallPart, EWallState> wallState;
	EGateState gateState;

	SiegeInfo();

	// return EWallState decreased by value of damage points
	static EWallState applyDamage(EWallState state, unsigned int value);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & wallState;
		h & gateState;
	}
};

VCMI_LIB_NAMESPACE_END
