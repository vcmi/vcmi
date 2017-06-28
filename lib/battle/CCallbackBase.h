/*
 * CCallbackBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once
#include "../GameConstants.h"

#define RETURN_IF_NOT_BATTLE(X) if(!duringBattle()) {logGlobal->errorStream() << __FUNCTION__ << " called when no battle!"; return X; }

class CGameState;
struct BattleInfo;

class CBattleInfoEssentials;

//Basic class for various callbacks (interfaces called by players to get info about game and so forth)
class DLL_LINKAGE CCallbackBase
{
	const BattleInfo * battle; //battle to which the player is engaged, nullptr if none or not applicable

	const BattleInfo * getBattle() const;

protected:
	CGameState * gs;
	boost::optional<PlayerColor> player; // not set gives access to all information, otherwise callback provides only information "visible" for player

	CCallbackBase(CGameState * GS, boost::optional<PlayerColor> Player);
	CCallbackBase();

	void setBattle(const BattleInfo * B);
	bool duringBattle() const;

public:
	boost::optional<PlayerColor> getPlayerID() const;

	friend class CBattleInfoEssentials;
};

