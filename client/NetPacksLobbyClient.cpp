/*
 * NetPacksLobbyClient.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "LobbyClientNetPackVisitors.h"
#include "lobby/CSelectionBase.h"
#include "lobby/CLobbyScreen.h"

#include "lobby/OptionsTab.h"
#include "lobby/RandomMapTab.h"
#include "lobby/SelectionTab.h"
#include "lobby/CBonusSelection.h"

#include "CServerHandler.h"
#include "CGameInfo.h"
#include "gui/CGuiHandler.h"
#include "gui/WindowHandler.h"
#include "widgets/Buttons.h"
#include "widgets/TextControls.h"

#include "../lib/CConfigHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/NetPacksLobby.h"
#include "../lib/serializer/Connection.h"

void ApplyOnLobbyHandlerNetPackVisitor::visitLobbyClientConnected(LobbyClientConnected & pack)
{
	result = false;

	// Check if it's LobbyClientConnected for our client
	if(pack.uuid == handler.c->uuid)
	{
		handler.c->connectionID = pack.clientId;
		if(handler.mapToStart)
			handler.setMapInfo(handler.mapToStart);
		else if(!settings["session"]["headless"].Bool())
			GH.windows().createAndPushWindow<CLobbyScreen>(static_cast<ESelectionScreen>(handler.screenType));
		handler.state = EClientState::LOBBY;
	}
}

void ApplyOnLobbyHandlerNetPackVisitor::visitLobbyClientDisconnected(LobbyClientDisconnected & pack)
{
	if(pack.clientId != pack.c->connectionID)
	{
		result = false;
		return;
	}

	handler.stopServerConnection();
}

void ApplyOnLobbyScreenNetPackVisitor::visitLobbyClientDisconnected(LobbyClientDisconnected & pack)
{
	if(auto w = GH.windows().topWindow<CLoadingScreen>())
		GH.windows().popWindow(w);
	
	if(GH.windows().count() > 0)
		GH.windows().popWindows(1);
}

void ApplyOnLobbyScreenNetPackVisitor::visitLobbyChatMessage(LobbyChatMessage & pack)
{
	if(lobby && lobby->card)
	{
		lobby->card->chat->addNewMessage(pack.playerName + ": " + pack.message);
		lobby->card->setChat(true);
		if(lobby->buttonChat)
			lobby->buttonChat->addTextOverlay(CGI->generaltexth->allTexts[531], FONT_SMALL, Colors::WHITE);
	}
}

void ApplyOnLobbyScreenNetPackVisitor::visitLobbyGuiAction(LobbyGuiAction & pack)
{
	if(!lobby || !handler.isGuest())
		return;

	switch(pack.action)
	{
	case LobbyGuiAction::NO_TAB:
		lobby->toggleTab(lobby->curTab);
		break;
	case LobbyGuiAction::OPEN_OPTIONS:
		lobby->toggleTab(lobby->tabOpt);
		break;
	case LobbyGuiAction::OPEN_SCENARIO_LIST:
		lobby->toggleTab(lobby->tabSel);
		break;
	case LobbyGuiAction::OPEN_RANDOM_MAP_OPTIONS:
		lobby->toggleTab(lobby->tabRand);
		break;
	}
}

void ApplyOnLobbyHandlerNetPackVisitor::visitLobbyEndGame(LobbyEndGame & pack)
{
	if(handler.state == EClientState::GAMEPLAY)
	{
		handler.endGameplay(pack.closeConnection, pack.restart);
	}
	
	if(pack.restart)
		handler.sendStartGame();
}

void ApplyOnLobbyHandlerNetPackVisitor::visitLobbyStartGame(LobbyStartGame & pack)
{
	if(pack.clientId != -1 && pack.clientId != handler.c->connectionID)
	{
		result = false;
		return;
	}
	
	handler.state = EClientState::STARTING;
	if(handler.si->mode != StartInfo::LOAD_GAME || pack.clientId == handler.c->connectionID)
	{
		auto modeBackup = handler.si->mode;
		handler.si = pack.initializedStartInfo;
		handler.si->mode = modeBackup;
	}
	handler.startGameplay(pack.initializedGameState);
}

void ApplyOnLobbyScreenNetPackVisitor::visitLobbyLoadProgress(LobbyLoadProgress & pack)
{
	if(auto w = GH.windows().topWindow<CLoadingScreen>())
		w->set(pack.progress);
	else
		GH.windows().createAndPushWindow<CLoadingScreen>();
}

void ApplyOnLobbyHandlerNetPackVisitor::visitLobbyUpdateState(LobbyUpdateState & pack)
{
	pack.hostChanged = pack.state.hostClientId != handler.hostClientId;
	static_cast<LobbyState &>(handler) = pack.state;
	if(handler.mapToStart && handler.mi)
	{
		handler.startMapAfterConnection(nullptr);
		handler.sendStartGame();
	}
}

void ApplyOnLobbyScreenNetPackVisitor::visitLobbyUpdateState(LobbyUpdateState & pack)
{
	if(!lobby) //stub: ignore message for game mode
		return;
		
	if(!lobby->bonusSel && handler.si->campState && handler.state == EClientState::LOBBY_CAMPAIGN)
	{
		lobby->bonusSel = std::make_shared<CBonusSelection>();
		GH.windows().pushWindow(lobby->bonusSel);
	}

	if(lobby->bonusSel)
		lobby->bonusSel->updateAfterStateChange();
	else
		lobby->updateAfterStateChange();

	if(pack.hostChanged)
		lobby->toggleMode(handler.isHost());
}

void ApplyOnLobbyScreenNetPackVisitor::visitLobbyShowMessage(LobbyShowMessage & pack)
{
	if(!lobby) //stub: ignore message for game mode
		return;
	
	lobby->buttonStart->block(false);
	handler.showServerError(pack.message);
}
