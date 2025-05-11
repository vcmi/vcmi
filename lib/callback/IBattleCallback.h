/*
 * IBattleCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../constants/EntityIdentifiers.h"

VCMI_LIB_NAMESPACE_BEGIN

class BattleAction;
class BattleStateInfoForRetreat;

class CPlayerBattleCallback;

class IBattleCallback
{
public:
	virtual ~IBattleCallback() = default;

	bool waitTillRealize = false; //if true, request functions will return after they are realized by server
	bool unlockGsWhenWaiting = false;//if true after sending each request, gs mutex will be unlocked so the changes can be applied; NOTICE caller must have gs mx locked prior to any call to actiob callback!
	//battle
	virtual void battleMakeSpellAction(const BattleID & battleID, const BattleAction & action) = 0;
	virtual void battleMakeUnitAction(const BattleID & battleID, const BattleAction & action) = 0;
	virtual void battleMakeTacticAction(const BattleID & battleID, const BattleAction & action) = 0;
	virtual std::optional<BattleAction> makeSurrenderRetreatDecision(const BattleID & battleID, const BattleStateInfoForRetreat & battleState) = 0;

	virtual std::shared_ptr<CPlayerBattleCallback> getBattle(const BattleID & battleID) = 0;
	virtual std::optional<PlayerColor> getPlayerID() const = 0;
};

VCMI_LIB_NAMESPACE_END
