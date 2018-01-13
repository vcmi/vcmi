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


SiegeInfo::SiegeInfo()
{
	for(int i = 0; i < wallState.size(); ++i)
	{
		wallState[i] = EWallState::NONE;
	}
	gateState = EGateState::NONE;
}

EWallState::EWallState SiegeInfo::applyDamage(EWallState::EWallState state, unsigned int value)
{
	if(value == 0)
		return state;

	switch(applyDamage(state, value - 1))
	{
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
