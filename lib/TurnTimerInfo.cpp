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

VCMI_LIB_NAMESPACE_BEGIN

bool TurnTimerInfo::isEnabled() const
{
	return turnTimer > 0 || baseTimer > 0;
}

bool TurnTimerInfo::isBattleEnabled() const
{
	return turnTimer > 0 || baseTimer > 0 || unitTimer > 0 || battleTimer > 0;
}

void TurnTimerInfo::substractTimer(int timeMs)
{
	auto const & substractTimer = [&timeMs](int & targetTimer)
	{
		if (targetTimer > timeMs)
		{
			targetTimer -= timeMs;
			timeMs = 0;
		}
		else
		{
			timeMs -= targetTimer;
			targetTimer = 0;
		}
	};

	substractTimer(unitTimer);
	substractTimer(battleTimer);
	substractTimer(turnTimer);
	substractTimer(baseTimer);
}

int TurnTimerInfo::valueMs() const
{
	return baseTimer + turnTimer + battleTimer + unitTimer;
}

bool TurnTimerInfo::operator == (const TurnTimerInfo & other) const
{
	return turnTimer == other.turnTimer &&
			baseTimer == other.baseTimer &&
			battleTimer == other.battleTimer &&
			unitTimer == other.unitTimer &&
			accumulatingTurnTimer == other.accumulatingTurnTimer &&
			accumulatingUnitTimer == other.accumulatingUnitTimer;
}

VCMI_LIB_NAMESPACE_END
