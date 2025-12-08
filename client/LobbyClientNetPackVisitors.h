/*
 * ClientNetPackVisitors.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../lib/networkPacks/NetPackVisitor.h"
#include "../lib/networkPacks/PacksForLobby.h"

class CClient;
class CLobbyScreen;
VCMI_LIB_NAMESPACE_BEGIN
class CGameState;
VCMI_LIB_NAMESPACE_END

class ApplyOnLobbyHandlerNetPackVisitor : public VCMI_LIB_WRAP_NAMESPACE(ICPackVisitor)
{
private:
	CServerHandler & handler;
	bool result;

public:
	ApplyOnLobbyHandlerNetPackVisitor(CServerHandler & handler)
		:handler(handler), result(true)
	{
	}

	bool getResult() const { return result; }

	void visitLobbyQuickLoadGame(LobbyQuickLoadGame & pack) override;
	void visitLobbyClientConnected(LobbyClientConnected & pack) override;
	void visitLobbyClientDisconnected(LobbyClientDisconnected & pack) override;
	void visitLobbyRestartGame(LobbyRestartGame & pack) override;
	void visitLobbyPrepareStartGame(LobbyPrepareStartGame & pack) override;
	void visitLobbyStartGame(LobbyStartGame & pack) override;
	void visitLobbyUpdateState(LobbyUpdateState & pack) override;
};

class ApplyOnLobbyScreenNetPackVisitor : public VCMI_LIB_WRAP_NAMESPACE(ICPackVisitor)
{
private:
	CServerHandler & handler;
	CLobbyScreen * lobby;

public:
	ApplyOnLobbyScreenNetPackVisitor(CServerHandler & handler, CLobbyScreen * lobby)
		:handler(handler), lobby(lobby)
	{
	}

	void visitLobbyClientDisconnected(LobbyClientDisconnected & pack) override;
	void visitLobbyChatMessage(LobbyChatMessage & pack) override;
	void visitLobbyGuiAction(LobbyGuiAction & pack) override;
	void visitLobbyStartGame(LobbyStartGame & pack) override;
	void visitLobbyLoadProgress(LobbyLoadProgress & pack) override;
	void visitLobbyUpdateState(LobbyUpdateState & pack) override;
	void visitLobbyShowMessage(LobbyShowMessage & pack) override;
	void visitLobbySetBattleOnlyModeStartInfo(LobbySetBattleOnlyModeStartInfo & pack) override;
};
