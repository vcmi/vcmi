/*
 * SiegeInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "SiegeInfo.h"

VCMI_LIB_NAMESPACE_BEGIN


SiegeInfo::SiegeInfo()
{
	for(int i = 0; i < static_cast<int>(EWallPart::PARTS_COUNT); ++i)
	{
		wallState[static_cast<EWallPart>(i)] = EWallState::NONE;
	}
	gateState = EGateState::NONE;
}

EWallState SiegeInfo::applyDamage(EWallState state, unsigned int value)
{
	if(value == 0)
		return state;

	switch(applyDamage(state, value - 1))
	{
	case EWallState::REINFORCED:
		return EWallState::INTACT;
	case EWallState::INTACT:
		return EWallState::DAMAGED;
	case EWallState::DAMAGED:
		return EWallState::DESTROYED;
	case EWallState::DESTROYED:
		return EWallState::DESTROYED;
	default:
		return EWallState::NONE;
	}
}

VCMI_LIB_NAMESPACE_END
