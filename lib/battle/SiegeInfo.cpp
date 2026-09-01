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
	for(int i = 0; i < static_cast<int>(EWallPart::PARTS_COUNT); ++i)
	{
		wallState[static_cast<EWallPart>(i)] = EWallState::NONE;
	}
	gateState = EGateState::NONE;
}

EWallState SiegeInfo::applyDamage(EWallState state, unsigned int value)
{
	if(state == EWallState::NONE)
		return EWallState::NONE;

	// wall health is stored as EWallState value and may exceed REINFORCED for extra-fortified walls,
	// so decrement numerically instead of stepping through named states
	int reduced = static_cast<int>(state) - static_cast<int>(value);
	return static_cast<EWallState>(std::max(reduced, static_cast<int>(EWallState::DESTROYED)));
}
