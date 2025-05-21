/*
 * CServerHandler.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "CServerHandler.h"
#include "Client.h"
#include "ServerRunner.h"
#include "GameChatHandler.h"
#include "CPlayerInterface.h"
#include "GameEngine.h"
#include "GameInstance.h"
#include "gui/WindowHandler.h"

#include "globalLobby/GlobalLobbyClient.h"
#include "lobby/CSelectionBase.h"
#include "lobby/CLobbyScreen.h"
#include "lobby/CBonusSelection.h"
#include "windows/InfoWindows.h"
#include "windows/GUIClasses.h"
#include "media/CMusicHandler.h"
#include "media/IVideoPlayer.h"

#include "mainmenu/CMainMenu.h"
#include "mainmenu/CPrologEpilogVideo.h"
#include "mainmenu/CHighScoreScreen.h"

#include "../lib/CConfigHandler.h"
#include "../lib/texts/CGeneralTextHandler.h"
#include "../lib/ConditionalWait.h"
#include "../lib/CThreadHelper.h"
#include "../lib/StartInfo.h"
#include "../lib/TurnTimerInfo.h"
#include "../lib/VCMIDirs.h"
#include "../lib/campaign/CampaignState.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/gameState/HighScore.h"
#include "../lib/CPlayerState.h"
#include "../lib/mapping/CMapInfo.h"
#include "../lib/mapObjects/CGTownInstance.h"
#include "../lib/mapObjects/MiscObjects.h"
#include "../lib/modding/ModIncompatibility.h"
#include "../lib/rmg/CMapGenOptions.h"
#include "../lib/serializer/Connection.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/UnlockGuard.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "LobbyClientNetPackVisitors.h"

#include <vcmi/events/EventBus.h>
#include <SDL_thread.h>

#include <boost/lexical_cast.hpp>

CServerHandler::~CServerHandler()
{
	if (serverRunner)
		serverRunner->shutdown();
	networkHandler->stop();

	if (serverRunner)
		serverRunner->wait();
	serverRunner.reset();
	if (threadNetwork.joinable())
	{
		auto unlockInterface = vstd::makeUnlockGuard(ENGINE->interfaceMutex);
		threadNetwork.join();
	}
}

void CServerHandler::endNetwork()
{
	if (client)
		client->endNetwork();
	networkHandler->stop();

	if (threadNetwork.joinable())
	{
		auto unlockInterface = vstd::makeUnlockGuard(ENGINE->interfaceMutex);
		threadNetwork.join();
	}
}

CServerHandler::CServerHandler()
	: networkHandler(INetworkHandler::createHandler())
	, lobbyClient(std::make_unique<GlobalLobbyClient>())
	, gameChat(std::make_unique<GameChatHandler>())
	, threadNetwork(&CServerHandler::threadRunNetwork, this)
	, state(EClientState::NONE)
	, serverPort(0)
	, campaignStateToSend(nullptr)
	, screenType(ESelectionScreen::unknown)
	, serverMode(EServerMode::NONE)
	, loadMode(ELoadMode::NONE)
	, client(nullptr)
{
	uuid = boost::uuids::to_string(boost::uuids::random_generator()());
}

void CServerHandler::threadRunNetwork()
{
	setThreadName("runNetwork");
	logGlobal->info("Starting network thread");
	try {
		networkHandler->run();
	}
	catch (const TerminationRequestedException &)
	{
		// VCMI can run SDL methods on network thread, leading to usage of thread-local storage by SDL
		// Such storage needs to be cleaned up manually for threads that were not created by SDL
		SDL_TLSCleanup();
		logGlobal->info("Terminating network thread");
		return;
	}
	SDL_TLSCleanup();
	logGlobal->info("Ending network thread");
}

void CServerHandler::resetStateForLobby(EStartMode mode, ESelectionScreen screen, EServerMode newServerMode, const std::vector<std::string> & playerNames)
{
	hostClientId = -1;
	setState(EClientState::NONE);
	serverMode = newServerMode;
	mapToStart = nullptr;
	th = std::make_unique<CStopWatch>();
	logicConnection.reset();
	si = std::make_shared<StartInfo>();
	localPlayerNames.clear();
	si->difficulty = 1;
	si->mode = mode;
	screenType = screen;
	localPlayerNames.clear();
	if(!playerNames.empty()) //if have custom set of player names - use it
		localPlayerNames = playerNames;
	else
	{
		std::string playerName = settings["general"]["playerName"].String();
		if(playerName == "Player")
			playerName = LIBRARY->generaltexth->translate("core.genrltxt.434");
		localPlayerNames.push_back(playerName);
	}

	gameChat->resetMatchState();
	lobbyClient->resetMatchState();
}

GameChatHandler & CServerHandler::getGameChat()
{
	return *gameChat;
}

GlobalLobbyClient & CServerHandler::getGlobalLobby()
{
	return *lobbyClient;
}

INetworkHandler & CServerHandler::getNetworkHandler()
{
	return *networkHandler;
}

void CServerHandler::startLocalServerAndConnect(bool connectToLobby)
{
	logNetwork->trace("\tLocal server startup has been requested");
#ifdef VCMI_MOBILE
	// mobile apps can't spawn separate processes - only thread mode is available
	serverRunner.reset(new ServerThreadRunner());
#else
	if (settings["server"]["useProcess"].Bool())
		serverRunner.reset(new ServerProcessRunner());
	else
		serverRunner.reset(new ServerThreadRunner());
#endif

	auto si = std::make_shared<StartInfo>();

	auto lastDifficulty = settings["general"]["lastDifficulty"];
	si->difficulty = lastDifficulty.Integer();

	logNetwork->trace("\tStarting local server");
	serverRunner->start(loadMode == ELoadMode::MULTI, connectToLobby, si);
	logNetwork->trace("\tConnecting to local server");
	connectToServer(getLocalHostname(), getLocalPort());
	logNetwork->trace("\tWaiting for connection");
}

void CServerHandler::connectToServer(const std::string & addr, const ui16 port)
{
	setState(EClientState::CONNECTING);
	serverHostname = addr;
	serverPort = port;

	if (!isServerLocal())
	{
		Settings remoteAddress = settings.write["server"]["remoteHostname"];
		remoteAddress->String() = addr;

		Settings remotePort = settings.write["server"]["remotePort"];
		remotePort->Integer() = port;

		networkHandler->connectToRemote(*this, addr, port);
	}
	else
	{
		serverRunner->connect(*networkHandler, *this);
	}
}

void CServerHandler::onConnectionFailed(const std::string & errorMessage)
{
	assert(getState() == EClientState::CONNECTING);
	std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

	if (isServerLocal())
	{
		// retry - local server might be still starting up
		logNetwork->debug("\nCannot establish connection. %s. Retrying...", errorMessage);
		networkHandler->createTimer(*this, std::chrono::milliseconds(100));
	}
	else
	{
		// remote server refused connection - show error message
		setState(EClientState::NONE);
		CInfoWindow::showInfoDialog(LIBRARY->generaltexth->translate("vcmi.mainMenu.serverConnectionFailed"), {});
	}
}

void CServerHandler::onTimer()
{
	std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

	if(getState() == EClientState::CONNECTION_CANCELLED)
	{
		logNetwork->info("Connection aborted by player!");
		serverRunner->wait();
		serverRunner.reset();
		if (ENGINE->windows().topWindow<CSimpleJoinScreen>() != nullptr)
			ENGINE->windows().popWindows(1);
		return;
	}

	assert(isServerLocal());
	serverRunner->connect(*networkHandler, *this);
}

void CServerHandler::onConnectionEstablished(const NetworkConnectionPtr & netConnection)
{
	assert(getState() == EClientState::CONNECTING);

	std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

	networkConnection = netConnection;

	logNetwork->info("Connection established");

	if (serverMode == EServerMode::LOBBY_GUEST)
	{
		// say hello to lobby to switch connection to proxy mode
		getGlobalLobby().sendProxyConnectionLogin(netConnection);
	}

	logicConnection = std::make_shared<CConnection>(netConnection);
	logicConnection->uuid = uuid;
	logicConnection->enterLobbyConnectionMode();
	sendClientConnecting();
}

void CServerHandler::applyPackOnLobbyScreen(CPackForLobby & pack)
{
	ApplyOnLobbyScreenNetPackVisitor visitor(*this, dynamic_cast<CLobbyScreen *>(SEL));
	pack.visit(visitor);
	ENGINE->windows().totalRedraw();
}

std::set<PlayerColor> CServerHandler::getHumanColors()
{
	return clientHumanColors(logicConnection->connectionID);
}

PlayerColor CServerHandler::myFirstColor() const
{
	return clientFirstColor(logicConnection->connectionID);
}

bool CServerHandler::isMyColor(PlayerColor color) const
{
	return isClientColor(logicConnection->connectionID, color);
}

ui8 CServerHandler::myFirstId() const
{
	return clientFirstId(logicConnection->connectionID);
}

EClientState CServerHandler::getState() const
{
	return state;
}

void CServerHandler::setState(EClientState newState)
{
	if (newState == EClientState::CONNECTION_CANCELLED && serverRunner != nullptr)
		serverRunner->shutdown();

	state = newState;
}

bool CServerHandler::isServerLocal() const
{
	return serverRunner != nullptr;
}

bool CServerHandler::isHost() const
{
	return logicConnection && hostClientId == logicConnection->connectionID;
}

bool CServerHandler::isGuest() const
{
	return !logicConnection || hostClientId != logicConnection->connectionID;
}

const std::string & CServerHandler::getLocalHostname() const
{
	return settings["server"]["localHostname"].String();
}

ui16 CServerHandler::getLocalPort() const
{
	return settings["server"]["localPort"].Integer();
}

const std::string & CServerHandler::getRemoteHostname() const
{
	return settings["server"]["remoteHostname"].String();
}

ui16 CServerHandler::getRemotePort() const
{
	return settings["server"]["remotePort"].Integer();
}

const std::string & CServerHandler::getCurrentHostname() const
{
	return serverHostname;
}

ui16 CServerHandler::getCurrentPort() const
{
	return serverPort;
}

void CServerHandler::sendClientConnecting() const
{
	LobbyClientConnected lcc;
	lcc.uuid = uuid;
	lcc.names = localPlayerNames;
	lcc.mode = si->mode;
	sendLobbyPack(lcc);
}

void CServerHandler::sendClientDisconnecting()
{
	// FIXME: This is workaround needed to make sure client not trying to sent anything to non existed server
	if(getState() == EClientState::DISCONNECTING)
	{
		assert(0);
		return;
	}

	setState(EClientState::DISCONNECTING);
	mapToStart = nullptr;
	LobbyClientDisconnected lcd;
	lcd.clientId = logicConnection->connectionID;
	logNetwork->info("Connection has been requested to be closed.");
	if(isServerLocal())
	{
		lcd.shutdownServer = true;
		logNetwork->info("Sent closing signal to the server");
	}
	else
	{
		logNetwork->info("Sent leaving signal to the server");
	}
	sendLobbyPack(lcd);
	networkConnection->close();
	networkConnection.reset();
	logicConnection.reset();
	waitForServerShutdown();
}

void CServerHandler::setCampaignState(std::shared_ptr<CampaignState> newCampaign)
{
	setState(EClientState::LOBBY_CAMPAIGN);
	LobbySetCampaign lsc;
	lsc.ourCampaign = newCampaign;
	sendLobbyPack(lsc);
}

void CServerHandler::setCampaignMap(CampaignScenarioID mapId) const
{
	if(getState() == EClientState::GAMEPLAY) // FIXME: UI shouldn't sent commands in first place
		return;

	LobbySetCampaignMap lscm;
	lscm.mapId = mapId;
	sendLobbyPack(lscm);
}

void CServerHandler::setCampaignBonus(int bonusId) const
{
	if(getState() == EClientState::GAMEPLAY) // FIXME: UI shouldn't sent commands in first place
		return;

	LobbySetCampaignBonus lscb;
	lscb.bonusId = bonusId;
	sendLobbyPack(lscb);
}

void CServerHandler::setMapInfo(std::shared_ptr<CMapInfo> to, std::shared_ptr<CMapGenOptions> mapGenOpts) const
{
	LobbySetMap lsm;
	lsm.mapInfo = to;
	lsm.mapGenOpts = mapGenOpts;
	sendLobbyPack(lsm);
}

void CServerHandler::setPlayer(PlayerColor color) const
{
	LobbySetPlayer lsp;
	lsp.clickedColor = color;
	sendLobbyPack(lsp);
}

void CServerHandler::setPlayerName(PlayerColor color, const std::string & name) const
{
	LobbySetPlayerName lspn;
	lspn.color = color;
	lspn.name = name;
	sendLobbyPack(lspn);
}

void CServerHandler::setPlayerHandicap(PlayerColor color, Handicap handicap) const
{
	LobbySetPlayerHandicap lsph;
	lsph.color = color;
	lsph.handicap = handicap;
	sendLobbyPack(lsph);
}

void CServerHandler::setPlayerOption(ui8 what, int32_t value, PlayerColor player) const
{
	LobbyChangePlayerOption lcpo;
	lcpo.what = what;
	lcpo.value = value;
	lcpo.color = player;
	sendLobbyPack(lcpo);
}

void CServerHandler::setDifficulty(int to) const
{
	LobbySetDifficulty lsd;
	lsd.difficulty = to;
	sendLobbyPack(lsd);
}

void CServerHandler::setSimturnsInfo(const SimturnsInfo & info) const
{
	LobbySetSimturns pack;
	pack.simturnsInfo = info;
	sendLobbyPack(pack);
}

void CServerHandler::setTurnTimerInfo(const TurnTimerInfo & info) const
{
	LobbySetTurnTime lstt;
	lstt.turnTimerInfo = info;
	sendLobbyPack(lstt);
}

void CServerHandler::setExtraOptionsInfo(const ExtraOptionsInfo & info) const
{
	LobbySetExtraOptions lseo;
	lseo.extraOptionsInfo = info;
	sendLobbyPack(lseo);
}

void CServerHandler::sendMessage(const std::string & txt) const
{
	std::istringstream readed;
	readed.str(txt);
	std::string command;
	readed >> command;
	if(command == "!passhost")
	{
		std::string id;
		readed >> id;
		if(id.length())
		{
			LobbyChangeHost lch;
			lch.newHostConnectionId = boost::lexical_cast<int>(id);
			sendLobbyPack(lch);
		}
	}
	else if(command == "!forcep")
	{
		std::string connectedId;
		std::string playerColorId;
		readed >> connectedId;
		readed >> playerColorId;
		if(connectedId.length() && playerColorId.length())
		{
			ui8 connected = boost::lexical_cast<int>(connectedId);
			auto color = PlayerColor(boost::lexical_cast<int>(playerColorId));
			if(color.isValidPlayer() && playerNames.find(connected) != playerNames.end())
			{
				LobbyForceSetPlayer lfsp;
				lfsp.targetConnectedPlayer = connected;
				lfsp.targetPlayerColor = color;
				sendLobbyPack(lfsp);
			}
		}
	}
	else
	{
		gameChat->sendMessageLobby(playerNames.find(myFirstId())->second.name, txt);
	}
}

void CServerHandler::sendGuiAction(ui8 action) const
{
	LobbyGuiAction lga;
	lga.action = static_cast<LobbyGuiAction::EAction>(action);
	sendLobbyPack(lga);
}

void CServerHandler::sendRestartGame() const
{
	if(si->campState && !si->campState->getLoadingBackground().empty())
		ENGINE->windows().createAndPushWindow<CLoadingScreen>(si->campState->getLoadingBackground());
	else
		ENGINE->windows().createAndPushWindow<CLoadingScreen>();
	
	LobbyRestartGame endGame;
	sendLobbyPack(endGame);
}

bool CServerHandler::validateGameStart(bool allowOnlyAI) const
{
	try
	{
		verifyStateBeforeStart(allowOnlyAI ? true : settings["session"]["onlyai"].Bool());
	}
	catch(ModIncompatibility & e)
	{
		logGlobal->warn("Incompatibility exception during start scenario: %s", e.what());
		std::string errorMsg;
		if(!e.whatMissing().empty())
		{
			errorMsg += LIBRARY->generaltexth->translate("vcmi.server.errors.modsToEnable") + '\n';
			errorMsg += e.whatMissing();
		}
		if(!e.whatExcessive().empty())
		{
			errorMsg += LIBRARY->generaltexth->translate("vcmi.server.errors.modsToDisable") + '\n';
			errorMsg += e.whatExcessive();
		}
		showServerError(errorMsg);
		return false;
	}
	catch(std::exception & e)
	{
		logGlobal->error("Exception during startScenario: %s", e.what());
		showServerError( std::string("Unable to start map! Reason: ") + e.what());
		return false;
	}

	return true;
}

void CServerHandler::sendStartGame(bool allowOnlyAI) const
{
	verifyStateBeforeStart(allowOnlyAI ? true : settings["session"]["onlyai"].Bool());

	if(!settings["session"]["headless"].Bool())
	{
		if(si->campState && !si->campState->getLoadingBackground().empty())
			ENGINE->windows().createAndPushWindow<CLoadingScreen>(si->campState->getLoadingBackground());
		else
			ENGINE->windows().createAndPushWindow<CLoadingScreen>();
	}
	
	LobbyPrepareStartGame lpsg;
	sendLobbyPack(lpsg);

	LobbyStartGame lsg;
	sendLobbyPack(lsg);
}

void CServerHandler::startMapAfterConnection(std::shared_ptr<CMapInfo> to)
{
	mapToStart = to;
}

void CServerHandler::startGameplay(std::shared_ptr<CGameState> gameState)
{
	if(GAME->mainmenu())
		GAME->mainmenu()->disable();

	switch(si->mode)
	{
	case EStartMode::NEW_GAME:
		client->newGame(gameState);
		break;
	case EStartMode::CAMPAIGN:
		if(si->campState->conqueredScenarios().empty())
			si->campState->highscoreParameters.clear();
		client->newGame(gameState);
		break;
	case EStartMode::LOAD_GAME:
		client->loadGame(gameState);
		break;
	default:
		throw std::runtime_error("Invalid mode");
	}
	// After everything initialized we can accept CPackToClient netpacks
	setState(EClientState::GAMEPLAY);
}

void CServerHandler::showHighScoresAndEndGameplay(PlayerColor player, bool victory, const StatisticDataSet & statistic)
{
	HighScoreParameter param = HighScore::prepareHighScores(&client->gameState(), player, victory);

	if(victory && client->gameState().getStartInfo()->campState)
	{
		startCampaignScenario(param, client->gameState().getStartInfo()->campState, statistic);
	}
	else
	{
		HighScoreCalculation scenarioHighScores;
		scenarioHighScores.parameters.push_back(param);
		scenarioHighScores.isCampaign = false;

		endGameplay();
		GAME->mainmenu()->menu->switchToTab("main");
		ENGINE->windows().createAndPushWindow<CHighScoreInputScreen>(victory, scenarioHighScores, statistic);
	}
}

void CServerHandler::endGameplay()
{
	client->finishGameplay();

	// Game is ending
	// Tell the network thread to reach a stable state
	sendClientDisconnecting();
	logNetwork->info("Closed connection.");

	client->endGame();
	client.reset();

	if (GAME->mainmenu())
	{
		GAME->mainmenu()->enable();
		GAME->mainmenu()->playMusic();
		GAME->mainmenu()->makeActiveInterface();
	}
}

void CServerHandler::restartGameplay()
{
	client->finishGameplay();
	client->endGame();
	client.reset();

	logicConnection->enterLobbyConnectionMode();
}

void CServerHandler::startCampaignScenario(HighScoreParameter param, std::shared_ptr<CampaignState> cs, const StatisticDataSet & statistic)
{
	std::shared_ptr<CampaignState> ourCampaign = cs;

	if (!cs)
		ourCampaign = si->campState;

	param.campaignName = cs->getNameTranslated();
	cs->highscoreParameters.push_back(param);
	auto campaignScoreCalculator = std::make_shared<HighScoreCalculation>();
	campaignScoreCalculator->isCampaign = true;
	campaignScoreCalculator->parameters = cs->highscoreParameters;

	endGameplay();

	auto & epilogue = ourCampaign->scenario(*ourCampaign->lastScenario()).epilog;
	auto finisher = [ourCampaign, campaignScoreCalculator, statistic]()
	{
		if(ourCampaign->campaignSet != "" && ourCampaign->isCampaignFinished())
		{
			Settings entry = persistentStorage.write["completedCampaigns"][ourCampaign->getFilename()];
			entry->Bool() = true;
		}

		if(!ourCampaign->isCampaignFinished())
			GAME->mainmenu()->openCampaignLobby(ourCampaign);
		else
		{
			GAME->mainmenu()->openCampaignScreen(ourCampaign->campaignSet);
			if(!ourCampaign->getOutroVideo().empty() && ENGINE->video().open(ourCampaign->getOutroVideo(), 1))
			{
				ENGINE->music().stopMusic();
				ENGINE->windows().createAndPushWindow<VideoWindow>(ourCampaign->getOutroVideo(), ourCampaign->getVideoRim().empty() ? ImagePath::builtin("INTRORIM") : ourCampaign->getVideoRim(), false, 1, [campaignScoreCalculator, statistic](bool skipped){
					ENGINE->windows().createAndPushWindow<CHighScoreInputScreen>(true, *campaignScoreCalculator, statistic);
				});
			}
			else
				ENGINE->windows().createAndPushWindow<CHighScoreInputScreen>(true, *campaignScoreCalculator, statistic);
		}
	};

	if(epilogue.hasPrologEpilog)
	{
		ENGINE->windows().createAndPushWindow<CPrologEpilogVideo>(epilogue, finisher);
	}
	else
	{
		finisher();
	}
}

void CServerHandler::showServerError(const std::string & txt) const
{
	if(auto w = ENGINE->windows().topWindow<CLoadingScreen>())
		ENGINE->windows().popWindow(w);
	
	CInfoWindow::showInfoDialog(txt, {});
}

int CServerHandler::howManyPlayerInterfaces()
{
	int playerInts = 0;
	for(auto pint : client->playerint)
	{
		if(dynamic_cast<CPlayerInterface *>(pint.second.get()))
			playerInts++;
	}

	return playerInts;
}

ELoadMode CServerHandler::getLoadMode()
{
	if(loadMode != ELoadMode::TUTORIAL && getState() == EClientState::GAMEPLAY)
	{
		if(si->campState)
			return ELoadMode::CAMPAIGN;
		for(auto pn : playerNames)
		{
			if(pn.second.connection != logicConnection->connectionID)
				return ELoadMode::MULTI;
		}
		if(howManyPlayerInterfaces() > 1)  //this condition will work for hotseat mode OR multiplayer with allowed more than 1 color per player to control
			return ELoadMode::MULTI;

		return ELoadMode::SINGLE;
	}
	return loadMode;
}

void CServerHandler::debugStartTest(std::string filename, bool save)
{
	logGlobal->info("Starting debug test with file: %s", filename);
	auto mapInfo = std::make_shared<CMapInfo>();
	if(save)
	{
		resetStateForLobby(EStartMode::LOAD_GAME, ESelectionScreen::loadGame, EServerMode::LOCAL, {});
		mapInfo->saveInit(ResourcePath(filename, EResType::SAVEGAME));
	}
	else
	{
		resetStateForLobby(EStartMode::NEW_GAME, ESelectionScreen::newGame, EServerMode::LOCAL, {});
		mapInfo->mapInit(filename);
	}
	if(settings["session"]["donotstartserver"].Bool())
		connectToServer(getLocalHostname(), getLocalPort());
	else
		startLocalServerAndConnect(false);

	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	while(!settings["session"]["headless"].Bool() && !ENGINE->windows().topWindow<CLobbyScreen>())
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

	while(!mi || mapInfo->fileURI != mi->fileURI)
	{
		setMapInfo(mapInfo);
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
	// "Click" on color to remove us from it
	setPlayer(myFirstColor());
	while(myFirstColor() != PlayerColor::CANNOT_DETERMINE)
		std::this_thread::sleep_for(std::chrono::milliseconds(50));

	while(true)
	{
		try
		{
			sendStartGame();
			break;
		}
		catch(...)
		{

		}
		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}
}

class ServerHandlerCPackVisitor : public VCMI_LIB_WRAP_NAMESPACE(ICPackVisitor)
{
private:
	CServerHandler & handler;

public:
	ServerHandlerCPackVisitor(CServerHandler & handler)
			:handler(handler)
	{
	}

	bool callTyped() override { return false; }

	void visitForLobby(CPackForLobby & lobbyPack) override
	{
		handler.visitForLobby(lobbyPack);
	}

	void visitForClient(CPackForClient & clientPack) override
	{
		handler.visitForClient(clientPack);
	}
};

void CServerHandler::onPacketReceived(const std::shared_ptr<INetworkConnection> &, const std::vector<std::byte> & message)
{
	std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

	if(getState() == EClientState::DISCONNECTING)
		return;

	auto pack = logicConnection->retrievePack(message);
	ServerHandlerCPackVisitor visitor(*this);
	pack->visit(visitor);
}

void CServerHandler::onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage)
{
	std::scoped_lock interfaceLock(ENGINE->interfaceMutex);

	if (connection != networkConnection)
	{
		// ServerHandler already closed this connection on its own
		// This is the final call from network thread that informs serverHandler that connection has died
		// ignore it since serverHandler have already shut down this connection (and possibly started a new one)
		return;
	}

	waitForServerShutdown();

	if(getState() == EClientState::DISCONNECTING)
	{
		// Note: this branch can be reached on app shutdown, when main thread holds mutex till destruction
		logNetwork->info("Successfully closed connection to server!");
		return;
	}

	logNetwork->error("Lost connection to server! Connection has been closed");

	if(client)
	{
		endGameplay();
		GAME->mainmenu()->menu->switchToTab("main");
		showServerError(LIBRARY->generaltexth->translate("vcmi.server.errors.disconnected"));
	}
	else
	{
		LobbyClientDisconnected lcd;
		lcd.clientId = logicConnection->connectionID;
		applyPackOnLobbyScreen(lcd);
	}

	networkConnection.reset();
}

void CServerHandler::waitForServerShutdown()
{
	if (!serverRunner)
		return; // may not exist for guest in MP

	serverRunner->wait();
	int exitCode = serverRunner->exitCode();
	serverRunner.reset();

	if (exitCode == 0)
	{
		logNetwork->info("Server closed correctly");
	}
	else
	{
		if (getState() == EClientState::CONNECTING)
		{
			showServerError(LIBRARY->generaltexth->translate("vcmi.server.errors.existingProcess"));
			setState(EClientState::CONNECTION_CANCELLED); // stop attempts to reconnect
		}
		logNetwork->error("Error: server failed to close correctly or crashed!");
		logNetwork->error("Check log file for more info");
	}

	serverRunner.reset();
}

void CServerHandler::visitForLobby(CPackForLobby & lobbyPack)
{
	ApplyOnLobbyHandlerNetPackVisitor visitor(*this);
	lobbyPack.visit(visitor);

	if(visitor.getResult())
	{
		if(!settings["session"]["headless"].Bool())
			applyPackOnLobbyScreen(lobbyPack);
	}
}

void CServerHandler::visitForClient(CPackForClient & clientPack)
{
	client->handlePack(clientPack);
}


void CServerHandler::sendLobbyPack(const CPackForLobby & pack) const
{
	if(getState() != EClientState::STARTING)
		logicConnection->sendPack(pack);
}

bool CServerHandler::inLobbyRoom() const
{
	return serverMode == EServerMode::LOBBY_HOST || serverMode == EServerMode::LOBBY_GUEST;
}

bool CServerHandler::inGame() const
{
	return logicConnection != nullptr;
}
