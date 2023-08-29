/*
 * TurnTimerHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#pragma once

#include "../lib/TurnTimerInfo.h"

VCMI_LIB_NAMESPACE_BEGIN

class CStack;
class PlayerColor;

VCMI_LIB_NAMESPACE_END

class CGameHandler;

class TurnTimerHandler
{	
	CGameHandler & gameHandler;
	const int turnTimePropagateFrequency = 5000;
	const int turnTimePropagateFrequencyCrit = 1000;
	const int turnTimePropagateThreshold = 3000;
	std::map<PlayerColor, TurnTimerInfo> timers;
	std::map<PlayerColor, int> lastUpdate;
	std::recursive_mutex mx;
	
	void onPlayerMakingTurn(PlayerColor player, int waitTime);
	void onBattleLoop(int waitTime);
	
	bool timerCountDown(int & timerToApply, int initialTimer, PlayerColor player, int waitTime);
	bool isPvpBattle() const;
	void sendTimerUpdate(PlayerColor player);
	
public:
	TurnTimerHandler(CGameHandler &);
	
	void onGameplayStart(PlayerColor player);
	void onPlayerGetTurn(PlayerColor player);
	void onBattleStart();
	void onBattleNextStack(const CStack & stack);
	void onBattleEnd();
	void update(int waitTime);
	void setTimerEnabled(PlayerColor player, bool enabled);
};
