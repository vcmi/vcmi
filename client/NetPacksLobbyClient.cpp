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

#include "lobby/CSelectionBase.h"
#include "lobby/CLobbyScreen.h"

#include "lobby/OptionsTab.h"
#include "lobby/RandomMapTab.h"
#include "lobby/SelectionTab.h"
#include "lobby/CBonusSelection.h"

#include "CServerHandler.h"
#include "CGameInfo.h"
#include "gui/CGuiHandler.h"
#include "widgets/Buttons.h"
#include "widgets/TextControls.h"

#include "../lib/CConfigHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/NetPacksLobby.h"
#include "../lib/serializer/Connection.h"

bool LobbyClientConnected::applyOnLobbyHandler(CServerHandler * handler)
{
	// Check if it's LobbyClientConnected for our client
	if(uuid == handler->c->uuid)
	{
		handler->c->connectionID = clientId;
		if(!settings["session"]["headless"].Bool())
			GH.pushIntT<CLobbyScreen>(static_cast<ESelectionScreen>(handler->screenType));
		handler->state = EClientState::LOBBY;
		return true;
	}
	return false;
}

void LobbyClientConnected::applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler)
{
}

bool LobbyClientDisconnected::applyOnLobbyHandler(CServerHandler * handler)
{
	if(clientId != c->connectionID)
		return false;

	handler->stopServerConnection();
	return true;
}

void LobbyClientDisconnected::applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler)
{
	if(GH.listInt.size())
		GH.popInts(1);
}

void LobbyChatMessage::applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler)
{
	if(lobby)
	{
		lobby->card->chat->addNewMessage(playerName + ": " + message);
		lobby->card->setChat(true);
		if(lobby->buttonChat)
			lobby->buttonChat->addTextOverlay(CGI->generaltexth->allTexts[531], FONT_SMALL);
	}
}

void LobbyGuiAction::applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler)
{
	if(!lobby || !handler->isGuest())
		return;

	switch(action)
	{
	case NO_TAB:
		lobby->toggleTab(lobby->curTab);
		break;
	case OPEN_OPTIONS:
		lobby->toggleTab(lobby->tabOpt);
		break;
	case OPEN_SCENARIO_LIST:
		lobby->toggleTab(lobby->tabSel);
		break;
	case OPEN_RANDOM_MAP_OPTIONS:
		lobby->toggleTab(lobby->tabRand);
		break;
	}
}

bool LobbyEndGame::applyOnLobbyHandler(CServerHandler * handler)
{
	if(handler->state == EClientState::GAMEPLAY)
	{
		handler->endGameplay(closeConnection, restart);
	}
	
	if(restart)
		handler->sendStartGame();
	
	return true;
}

bool LobbyStartGame::applyOnLobbyHandler(CServerHandler * handler)
{
	handler->state = EClientState::STARTING;
	if(handler->si->mode != StartInfo::LOAD_GAME)
	{
		handler->si = initializedStartInfo;
	}
	if(settings["session"]["headless"].Bool())
		handler->startGameplay(initializedGameState);
	return true;
}

void LobbyStartGame::applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler)
{
	GH.pushIntT<CLoadingScreen>(std::bind(&CServerHandler::startGameplay, handler, initializedGameState));
}

bool LobbyUpdateState::applyOnLobbyHandler(CServerHandler * handler)
{
	hostChanged = state.hostClientId != handler->hostClientId;
	static_cast<LobbyState &>(*handler) = state;
	return true;
}

void LobbyUpdateState::applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler)
{
	if(!lobby) //stub: ignore message for game mode
		return;
		
	if(!lobby->bonusSel && handler->si->campState && handler->state == EClientState::LOBBY_CAMPAIGN)
	{
		lobby->bonusSel = std::make_shared<CBonusSelection>();
		GH.pushInt(lobby->bonusSel);
	}

	if(lobby->bonusSel)
		lobby->bonusSel->updateAfterStateChange();
	else
		lobby->updateAfterStateChange();

	if(hostChanged)
		lobby->toggleMode(handler->isHost());
}

void LobbyShowMessage::applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler)
{
	if(!lobby) //stub: ignore message for game mode
		return;
	
	lobby->buttonStart->block(false);
	handler->showServerError(message);
}
