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
#include "lobby/BattleOnlyModeTab.h"
#include "globalLobby/GlobalLobbyWindow.h"
#include "globalLobby/GlobalLobbyServerSetup.h"
#include "globalLobby/GlobalLobbyClient.h"

#include "CServerHandler.h"
#include "GameChatHandler.h"
#include "Client.h"
#include "GameEngine.h"
#include "GameInstance.h"
#include "gui/WindowHandler.h"
#include "widgets/Buttons.h"
#include "widgets/TextControls.h"
#include "media/CMusicHandler.h"
#include "media/IVideoPlayer.h"
#include "windows/GUIClasses.h"

#include "../lib/CConfigHandler.h"
#include "../lib/campaign/CampaignState.h"
#include "../lib/serializer/GameConnection.h"
#include "../lib/texts/CGeneralTextHandler.h"

void ApplyOnLobbyHandlerNetPackVisitor::visitLobbyQuickLoadGame(LobbyQuickLoadGame & pack)
{
	assert(handler.getState() == EClientState::GAMEPLAY);

	handler.restartGameplay();
	handler.sendStartGame();
}

void ApplyOnLobbyHandlerNetPackVisitor::visitLobbyClientConnected(LobbyClientConnected & pack)
{
	result = false;

	// Check if it's LobbyClientConnected for our client
	if(pack.uuid == handler.logicConnection->uuid)
	{
		handler.logicConnection->setSerializationVersion(pack.version);
		handler.logicConnection->connectionID = pack.clientId;
		if(handler.mapToStart)
		{
			handler.setMapInfo(handler.mapToStart);
		}
		else if(!settings["session"]["headless"].Bool())
		{
			if (ENGINE->windows().topWindow<CSimpleJoinScreen>())
				ENGINE->windows().popWindows(1);

			if (!ENGINE->windows().findWindows<GlobalLobbyServerSetup>().empty())
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

			while (!ENGINE->windows().findWindows<GlobalLobbyWindow>().empty())
			{
				// if global lobby is open, pop all dialogs on top of it as well as lobby itself
				ENGINE->windows().popWindows(1);
			}

			bool hideScreen = handler.campaignStateToSend && (!handler.campaignStateToSend->campaignSet.empty() || handler.campaignStateToSend->lastScenario());
			ENGINE->windows().createAndPushWindow<CLobbyScreen>(handler.screenType, hideScreen);
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
	if(auto w = ENGINE->windows().topWindow<CLoadingScreen>())
		ENGINE->windows().popWindow(w);
	
	if(ENGINE->windows().count() > 0)
		ENGINE->windows().popWindows(1);
}

void ApplyOnLobbyScreenNetPackVisitor::visitLobbyChatMessage(LobbyChatMessage & pack)
{
	handler.getGameChat().onNewLobbyMessageReceived(pack.playerName, pack.message.toString());
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
	case LobbyGuiAction::BATTLE_MODE:
		lobby->toggleTab(lobby->tabBattleOnlyMode);
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
	handler.logicConnection->enterLobbyConnectionMode();
}

void ApplyOnLobbyHandlerNetPackVisitor::visitLobbyStartGame(LobbyStartGame & pack)
{
	handler.setState(EClientState::STARTING);
	if(handler.si->mode != EStartMode::LOAD_GAME)
	{
		auto modeBackup = handler.si->mode;
		handler.si = pack.initializedStartInfo;
		handler.si->mode = modeBackup;
	}
	handler.client = std::make_unique<CClient>();
	handler.startGameplay(pack.initializedGameState);
}

void ApplyOnLobbyScreenNetPackVisitor::visitLobbyStartGame(LobbyStartGame & pack)
{
	if(auto w = ENGINE->windows().topWindow<CLoadingScreen>())
	{
		w->finish();
		w->tick(0);
		w->redraw();
	}
}

void ApplyOnLobbyScreenNetPackVisitor::visitLobbyLoadProgress(LobbyLoadProgress & pack)
{
	if(auto w = ENGINE->windows().topWindow<CLoadingScreen>())
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
		auto bonusSel = std::make_shared<CBonusSelection>();
		lobby->bonusSel = bonusSel;
		if(!handler.si->campState->conqueredScenarios().size() && !handler.si->campState->getIntroVideo().empty() && ENGINE->video().open(handler.si->campState->getIntroVideo(), 1))
		{
			ENGINE->music().stopMusic();
			auto rim = handler.si->campState->getVideoRim().empty() ? ImagePath::builtin("INTRORIM") : handler.si->campState->getVideoRim();
			if(handler.si->campState->getVideoRim() == ImagePath::builtin("NONE"))
				rim = ImagePath();
			ENGINE->windows().createAndPushWindow<VideoWindow>(handler.si->campState->getIntroVideo(), rim, false, 1, [bonusSel](bool skipped){
				if(!GAME->server().si->campState->getMusic().empty())
					ENGINE->music().playMusic(GAME->server().si->campState->getMusic(), true, false);
				ENGINE->windows().pushWindow(bonusSel);
			});
		}
		else
			ENGINE->windows().pushWindow(bonusSel);
	}

	if(lobby->bonusSel)
		lobby->bonusSel->updateAfterStateChange();
	else
		lobby->updateAfterStateChange();

	if(pack.hostChanged || pack.refreshList)
		lobby->toggleMode(handler.isHost());
}

void ApplyOnLobbyScreenNetPackVisitor::visitLobbyShowMessage(LobbyShowMessage & pack)
{
	if(!lobby) //stub: ignore message for game mode
		return;
	
	lobby->buttonStart->block(false);
	handler.showServerError(pack.message.toString());
}

void ApplyOnLobbyScreenNetPackVisitor::visitLobbySetBattleOnlyModeStartInfo(LobbySetBattleOnlyModeStartInfo & pack)
{
	if(lobby->tabBattleOnlyMode)
		lobby->tabBattleOnlyMode->applyStartInfo(pack.startInfo);
}
