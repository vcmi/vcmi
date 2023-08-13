/*
 * TurnTimerInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "TurnTimerInfo.h"

bool TurnTimerInfo::isEnabled() const
{
	return turnTimer > 0;
}

bool TurnTimerInfo::isBattleEnabled() const
{
	return battleTimer > 0;
}
