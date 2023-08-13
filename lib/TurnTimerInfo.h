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
	int creatureTimer = 0; //in ms, counts down when player is choosing action in battle
	
	bool isEnabled() const;
	bool isBattleEnabled() const;
	
	template <typename Handler>
	void serialize(Handler &h, const int version)
	{
		h & turnTimer;
		h & baseTimer;
		h & battleTimer;
		h & creatureTimer;
	}
};

VCMI_LIB_NAMESPACE_END
