/*
 * IClient.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

VCMI_LIB_NAMESPACE_BEGIN

class BattleID;
class BattleAction;
class BattleStateInfoForRetreat;
class PlayerColor;
struct CPackForServer;

class DLL_LINKAGE IClient
{
public:
	virtual std::optional<BattleAction> makeSurrenderRetreatDecision(PlayerColor player, const BattleID & battleID, const BattleStateInfoForRetreat & battleState) = 0;
	virtual int sendRequest(const CPackForServer & request, PlayerColor player, bool waitTillRealize) = 0;
	virtual ~IClient() = default;
};

VCMI_LIB_NAMESPACE_END
