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
#include "lobby/TurnOptionsTab.h"
#include "lobby/ExtraOptionsTab.h"
#include "lobby/SelectionTab.h"
#include "lobby/CBonusSelection.h"
#include "globalLobby/GlobalLobbyWindow.h"
#include "globalLobby/GlobalLobbyServerSetup.h"
#include "globalLobby/GlobalLobbyClient.h"

#include "CServerHandler.h"
#include "GameChatHandler.h"
#include "CGameInfo.h"
#include "Client.h"
#include "gui/CGuiHandler.h"
#include "gui/WindowHandler.h"
#include "widgets/Buttons.h"
#include "widgets/TextControls.h"

#include "../lib/CConfigHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/serializer/Connection.h"

void ApplyOnLobbyHandlerNetPackVisitor::visitLobbyClientConnected(LobbyClientConnected & pack)
{
	result = false;

	// Check if it's LobbyClientConnected for our client
	if(pack.uuid == handler.logicConnection->uuid)
	{
		handler.logicConnection->connectionID = pack.clientId;
		if(handler.mapToStart)
		{
			handler.setMapInfo(handler.mapToStart);
		}
		else if(!settings["session"]["headless"].Bool())
		{
			if (GH.windows().topWindow<CSimpleJoinScreen>())
				GH.windows().popWindows(1);

			if (!GH.windows().findWindows<GlobalLobbyServerSetup>().empty())
			{
				assert(handler.serverMode == EServerMode::LOBBY_HOST);
				// announce opened game room
				// TODO: find better approach?
				int roomType = settings["lobby"]["roomType"].Integer();
				int roomPlayerLimit = settings["lobby"]["roomPlayerLimit"].Integer();

				if (roomType != 0)
					handler.getGlobalLobby().sendOpenRoom("private", roomPlayerLimit);
				else
					handler.getGlobalLobby().sendOpenRoom("public", roomPlayerLimit);
			}

			while (!GH.windows().findWindows<GlobalLobbyWindow>().empty())
			{
				// if global lobby is open, pop all dialogs on top of it as well as lobby itself
				GH.windows().popWindows(1);
			}

			GH.windows().createAndPushWindow<CLobbyScreen>(handler.screenType);
		}
		handler.setState(EClientState::LOBBY);
	}
}

void ApplyOnLobbyHandlerNetPackVisitor::visitLobbyClientDisconnected(LobbyClientDisconnected & pack)
{
	if(pack.clientId != handler.logicConnection->connectionID)
	{
		result = false;
		return;
	}
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
	handler.getGameChat().onNewLobbyMessageReceived(pack.playerName, pack.message);
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
	case LobbyGuiAction::OPEN_TURN_OPTIONS:
		lobby->toggleTab(lobby->tabTurnOptions);
		break;
	case LobbyGuiAction::OPEN_EXTRA_OPTIONS:
		lobby->toggleTab(lobby->tabExtraOptions);
		break;
	}
}

void ApplyOnLobbyHandlerNetPackVisitor::visitLobbyRestartGame(LobbyRestartGame & pack)
{
	assert(handler.getState() == EClientState::GAMEPLAY);

	handler.restartGameplay();
	handler.sendStartGame();
}

void ApplyOnLobbyHandlerNetPackVisitor::visitLobbyPrepareStartGame(LobbyPrepareStartGame & pack)
{
	handler.client = std::make_unique<CClient>();
	handler.logicConnection->enterLobbyConnectionMode();
	handler.logicConnection->setCallback(handler.client.get());
}

void ApplyOnLobbyHandlerNetPackVisitor::visitLobbyStartGame(LobbyStartGame & pack)
{
	if(pack.clientId != -1 && pack.clientId != handler.logicConnection->connectionID)
	{
		result = false;
		return;
	}
	
	handler.setState(EClientState::STARTING);
	if(handler.si->mode != EStartMode::LOAD_GAME || pack.clientId == handler.logicConnection->connectionID)
	{
		auto modeBackup = handler.si->mode;
		handler.si = pack.initializedStartInfo;
		handler.si->mode = modeBackup;
	}
	handler.startGameplay(pack.initializedGameState);
}

void ApplyOnLobbyScreenNetPackVisitor::visitLobbyStartGame(LobbyStartGame & pack)
{
	if(auto w = GH.windows().topWindow<CLoadingScreen>())
	{
		w->finish();
		w->tick(0);
		w->redraw();
	}
}

void ApplyOnLobbyScreenNetPackVisitor::visitLobbyLoadProgress(LobbyLoadProgress & pack)
{
	if(auto w = GH.windows().topWindow<CLoadingScreen>())
	{
		w->set(pack.progress);
		w->tick(0);
		w->redraw();
	}
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
		
	if(!lobby->bonusSel && handler.si->campState && handler.getState() == EClientState::LOBBY_CAMPAIGN)
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
