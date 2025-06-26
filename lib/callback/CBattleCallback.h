/*
 * CBattleCallback.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "IBattleCallback.h"

VCMI_LIB_NAMESPACE_BEGIN

struct CPackForServer;

class IBattleInfo;
class IClient;

class DLL_LINKAGE CBattleCallback : public IBattleCallback
{
	std::map<BattleID, std::shared_ptr<CPlayerBattleCallback>> activeBattles;
	std::optional<PlayerColor> player;
	IClient *cl;

protected:
	int sendRequest(const CPackForServer & request); //returns requestID (that'll be matched to requestID in PackageApplied)

public:
	CBattleCallback(std::optional<PlayerColor> player, IClient * C);
	void battleMakeSpellAction(const BattleID & battleID, const BattleAction & action) override;//for casting spells by hero - DO NOT use it for moving active stack
	void battleMakeUnitAction(const BattleID & battleID, const BattleAction & action) override;
	void battleMakeTacticAction(const BattleID & battleID, const BattleAction & action) override; // performs tactic phase actions
	std::optional<BattleAction> makeSurrenderRetreatDecision(const BattleID & battleID, const BattleStateInfoForRetreat & battleState) override;

	std::shared_ptr<CPlayerBattleCallback> getBattle(const BattleID & battleID) override;
	std::optional<PlayerColor> getPlayerID() const override;

	void onBattleStarted(const IBattleInfo * info);
	void onBattleEnded(const BattleID & battleID);
};

VCMI_LIB_NAMESPACE_END
