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

#include "../lib/NetPackVisitor.h"

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

	virtual void visitForLobby(CPackForLobby & pack) override;
	virtual void visitLobbyClientConnected(LobbyClientConnected & pack) override;
	virtual void visitLobbyClientDisconnected(LobbyClientDisconnected & pack) override;
	virtual void visitLobbyEndGame(LobbyEndGame & pack) override;
	virtual void visitLobbyStartGame(LobbyStartGame & pack) override;
	virtual void visitLobbyChangeHost(LobbyChangeHost & pack) override;
	virtual void visitLobbyChangePlayerOption(LobbyChangePlayerOption & pack) override;
	virtual void visitLobbyChatMessage(LobbyChatMessage & pack) override;
	virtual void visitLobbyGuiAction(LobbyGuiAction & pack) override;
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

	virtual void visitForLobby(CPackForLobby & pack) override;
	virtual void visitLobbyClientConnected(LobbyClientConnected & pack) override;
	virtual void visitLobbyClientDisconnected(LobbyClientDisconnected & pack) override;
	virtual void visitLobbyEndGame(LobbyEndGame & pack) override;
	virtual void visitLobbyStartGame(LobbyStartGame & pack) override;
	virtual void visitLobbyChangeHost(LobbyChangeHost & pack) override;
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

	virtual void visitLobbyClientConnected(LobbyClientConnected & pack) override;
	virtual void visitLobbyClientDisconnected(LobbyClientDisconnected & pack) override;
	virtual void visitLobbySetMap(LobbySetMap & pack) override;
	virtual void visitLobbySetCampaign(LobbySetCampaign & pack) override;
	virtual void visitLobbySetCampaignMap(LobbySetCampaignMap & pack) override;
	virtual void visitLobbySetCampaignBonus(LobbySetCampaignBonus & pack) override;
	virtual void visitLobbyEndGame(LobbyEndGame & pack) override;
	virtual void visitLobbyStartGame(LobbyStartGame & pack) override;
	virtual void visitLobbyChangeHost(LobbyChangeHost & pack) override;
	virtual void visitLobbyChangePlayerOption(LobbyChangePlayerOption & pack) override;
	virtual void visitLobbySetPlayer(LobbySetPlayer & pack) override;
	virtual void visitLobbySetTurnTime(LobbySetTurnTime & pack) override;
	virtual void visitLobbySetDifficulty(LobbySetDifficulty & pack) override;
	virtual void visitLobbyForceSetPlayer(LobbyForceSetPlayer & pack) override;
};