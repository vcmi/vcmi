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

class ClientPermissionsCheckerNetPackVisitor : public VCMI_LIB_WRAP_NAMESPACE(ICPackVisitor)
{
private:
	bool result;
	CVCMIServer & srv;

public:
	ClientPermissionsCheckerNetPackVisitor(CVCMIServer & srv)
		:srv(srv), result(false)
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
};

class ApplyOnServerAfterAnnounceNetPackVisitor : public VCMI_LIB_WRAP_NAMESPACE(ICPackVisitor)
{
private:
	CVCMIServer & srv;

public:
	ApplyOnServerAfterAnnounceNetPackVisitor(CVCMIServer & srv)
		:srv(srv)
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
	bool result;
	CVCMIServer & srv;

public:
	ApplyOnServerNetPackVisitor(CVCMIServer & srv)
		:srv(srv), result(true)
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
	void visitLobbySetTurnTime(LobbySetTurnTime & pack) override;
	void visitLobbySetExtraOptions(LobbySetExtraOptions & pack) override;
	void visitLobbySetSimturns(LobbySetSimturns & pack) override;
	void visitLobbySetDifficulty(LobbySetDifficulty & pack) override;
	void visitLobbyForceSetPlayer(LobbyForceSetPlayer & pack) override;
};
