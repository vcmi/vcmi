/*
 * TurnTimerInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

VCMI_LIB_NAMESPACE_BEGIN

struct DLL_LINKAGE TurnTimerInfo
{
	int turnTimer = 0; //in ms, counts down when player is making his turn on adventure map
	int baseTimer = 0; //in ms, counts down only when turn timer runs out
	int battleTimer = 0; //in ms, counts down during battles when creature timer runs out
	int unitTimer = 0; //in ms, counts down when player is choosing action in battle

	bool accumulatingTurnTimer = false;
	bool accumulatingUnitTimer = false;
	
	bool isActive = false; //is being counting down
	bool isBattle = false; //indicator for current timer mode
	
	bool isEnabled() const;
	bool isBattleEnabled() const;

	void subtractTimer(int timeMs);
	int valueMs() const;

	bool operator == (const TurnTimerInfo & other) const;

	template <typename Handler>
	void serialize(Handler &h)
	{
		h & turnTimer;
		h & baseTimer;
		h & battleTimer;
		h & unitTimer;
		h & accumulatingTurnTimer;
		h & accumulatingUnitTimer;
		h & isActive;
		h & isBattle;
	}
};

VCMI_LIB_NAMESPACE_END
