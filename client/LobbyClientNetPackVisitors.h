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

#include "../lib/NetPackVisitor.h"

class CClient;
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

	virtual void visitLobbyClientConnected(LobbyClientConnected & pack) override;
	virtual void visitLobbyClientDisconnected(LobbyClientDisconnected & pack) override;
	virtual void visitLobbyEndGame(LobbyEndGame & pack) override;
	virtual void visitLobbyStartGame(LobbyStartGame & pack) override;
	virtual void visitLobbyUpdateState(LobbyUpdateState & pack) override;
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

	virtual void visitLobbyClientDisconnected(LobbyClientDisconnected & pack) override;
	virtual void visitLobbyChatMessage(LobbyChatMessage & pack) override;
	virtual void visitLobbyGuiAction(LobbyGuiAction & pack) override;
	virtual void visitLobbyStartGame(LobbyStartGame & pack) override;
	virtual void visitLobbyUpdateState(LobbyUpdateState & pack) override;
	virtual void visitLobbyShowMessage(LobbyShowMessage & pack) override;
};
