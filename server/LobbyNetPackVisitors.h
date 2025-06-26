/*
 * LobbyNetPackVisitors.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/networkPacks/NetPackVisitor.h"

VCMI_LIB_NAMESPACE_BEGIN
class GameConnection;
VCMI_LIB_NAMESPACE_END

class ClientPermissionsCheckerNetPackVisitor : public VCMI_LIB_WRAP_NAMESPACE(ICPackVisitor)
{
private:
	std::shared_ptr<GameConnection> connection;
	CVCMIServer & srv;
	bool result;

public:
	ClientPermissionsCheckerNetPackVisitor(CVCMIServer & srv, const std::shared_ptr<GameConnection> & connection)
		: connection(connection)
		, srv(srv)
		, result(false)
	{
	}

	bool getResult() const
	{
		return result;
	}

	void visitForLobby(CPackForLobby & pack) override;
	void visitLobbyClientConnected(LobbyClientConnected & pack) override;
	void visitLobbyClientDisconnected(LobbyClientDisconnected & pack) override;
	void visitLobbyRestartGame(LobbyRestartGame & pack) override;
	void visitLobbyPrepareStartGame(LobbyPrepareStartGame & pack) override;
	void visitLobbyStartGame(LobbyStartGame & pack) override;
	void visitLobbyChangeHost(LobbyChangeHost & pack) override;
	void visitLobbyChangePlayerOption(LobbyChangePlayerOption & pack) override;
	void visitLobbyChatMessage(LobbyChatMessage & pack) override;
	void visitLobbyGuiAction(LobbyGuiAction & pack) override;
	void visitLobbyPvPAction(LobbyPvPAction & pack) override;
	void visitLobbyDelete(LobbyDelete & pack) override;
};

class ApplyOnServerAfterAnnounceNetPackVisitor : public VCMI_LIB_WRAP_NAMESPACE(ICPackVisitor)
{
private:
	CVCMIServer & srv;

public:
	ApplyOnServerAfterAnnounceNetPackVisitor(CVCMIServer & srv)
		: srv(srv)
	{
	}

	void visitForLobby(CPackForLobby & pack) override;
	void visitLobbyClientConnected(LobbyClientConnected & pack) override;
	void visitLobbyClientDisconnected(LobbyClientDisconnected & pack) override;
	void visitLobbyRestartGame(LobbyRestartGame & pack) override;
	void visitLobbyStartGame(LobbyStartGame & pack) override;
	void visitLobbyChangeHost(LobbyChangeHost & pack) override;
};

class ApplyOnServerNetPackVisitor : public VCMI_LIB_WRAP_NAMESPACE(ICPackVisitor)
{
private:
	std::shared_ptr<GameConnection> connection;
	CVCMIServer & srv;
	bool result;

public:
	ApplyOnServerNetPackVisitor(CVCMIServer & srv, const std::shared_ptr<GameConnection> & connection)
		: connection(connection)
		, srv(srv)
		, result(true)
	{
	}

	bool getResult() const
	{
		return result;
	}

	void visitLobbyClientConnected(LobbyClientConnected & pack) override;
	void visitLobbyClientDisconnected(LobbyClientDisconnected & pack) override;
	void visitLobbySetMap(LobbySetMap & pack) override;
	void visitLobbySetCampaign(LobbySetCampaign & pack) override;
	void visitLobbySetCampaignMap(LobbySetCampaignMap & pack) override;
	void visitLobbySetCampaignBonus(LobbySetCampaignBonus & pack) override;
	void visitLobbyRestartGame(LobbyRestartGame & pack) override;
	void visitLobbyStartGame(LobbyStartGame & pack) override;
	void visitLobbyChangeHost(LobbyChangeHost & pack) override;
	void visitLobbyChangePlayerOption(LobbyChangePlayerOption & pack) override;
	void visitLobbySetPlayer(LobbySetPlayer & pack) override;
	void visitLobbySetPlayerName(LobbySetPlayerName & pack) override;
	void visitLobbySetPlayerHandicap(LobbySetPlayerHandicap & pack) override;
	void visitLobbySetTurnTime(LobbySetTurnTime & pack) override;
	void visitLobbySetExtraOptions(LobbySetExtraOptions & pack) override;
	void visitLobbySetSimturns(LobbySetSimturns & pack) override;
	void visitLobbySetDifficulty(LobbySetDifficulty & pack) override;
	void visitLobbyForceSetPlayer(LobbyForceSetPlayer & pack) override;
	void visitLobbyPvPAction(LobbyPvPAction & pack) override;
	void visitLobbyDelete(LobbyDelete & pack) override;
};
