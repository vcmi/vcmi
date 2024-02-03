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
#include "CGameInfo.h"
#include "CPlayerInterface.h"
#include "gui/CGuiHandler.h"
#include "gui/WindowHandler.h"

#include "globalLobby/GlobalLobbyClient.h"
#include "lobby/CSelectionBase.h"
#include "lobby/CLobbyScreen.h"
#include "windows/InfoWindows.h"

#include "mainmenu/CMainMenu.h"
#include "mainmenu/CPrologEpilogVideo.h"
#include "mainmenu/CHighScoreScreen.h"

#ifdef VCMI_ANDROID
#include "../lib/CAndroidVMHelper.h"
#elif defined(VCMI_IOS)
#include "ios/utils.h"
#include <dispatch/dispatch.h>
#endif

#ifdef SINGLE_PROCESS_APP
#include "../server/CVCMIServer.h"
#endif

#include "../lib/CConfigHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CThreadHelper.h"
#include "../lib/StartInfo.h"
#include "../lib/TurnTimerInfo.h"
#include "../lib/VCMIDirs.h"
#include "../lib/campaign/CampaignState.h"
#include "../lib/mapping/CMapInfo.h"
#include "../lib/mapObjects/MiscObjects.h"
#include "../lib/modding/ModIncompatibility.h"
#include "../lib/rmg/CMapGenOptions.h"
#include "../lib/serializer/Connection.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/registerTypes/RegisterTypesLobbyPacks.h"
#include "../lib/serializer/CMemorySerializer.h"
#include "../lib/UnlockGuard.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include "../lib/serializer/Cast.h"
#include "LobbyClientNetPackVisitors.h"

#include <vcmi/events/EventBus.h>

#ifdef VCMI_WINDOWS
#include <windows.h>
#endif

template<typename T> class CApplyOnLobby;

#if defined(VCMI_ANDROID) && !defined(SINGLE_PROCESS_APP)
extern std::atomic_bool androidTestServerReadyFlag;
#endif

class CBaseForLobbyApply
{
public:
	virtual bool applyOnLobbyHandler(CServerHandler * handler, CPackForLobby & pack) const = 0;
	virtual void applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler, CPackForLobby & pack) const = 0;
	virtual ~CBaseForLobbyApply(){};
	template<typename U> static CBaseForLobbyApply * getApplier(const U * t = nullptr)
	{
		return new CApplyOnLobby<U>();
	}
};

template<typename T> class CApplyOnLobby : public CBaseForLobbyApply
{
public:
	bool applyOnLobbyHandler(CServerHandler * handler, CPackForLobby & pack) const override
	{
		boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);

		auto & ref = static_cast<T&>(pack);
		ApplyOnLobbyHandlerNetPackVisitor visitor(*handler);

		logNetwork->trace("\tImmediately apply on lobby: %s", typeid(ref).name());
		ref.visit(visitor);

		return visitor.getResult();
	}

	void applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler, CPackForLobby & pack) const override
	{
		auto & ref = static_cast<T &>(pack);
		ApplyOnLobbyScreenNetPackVisitor visitor(*handler, lobby);

		logNetwork->trace("\tApply on lobby from queue: %s", typeid(ref).name());
		ref.visit(visitor);
	}
};

template<> class CApplyOnLobby<CPack>: public CBaseForLobbyApply
{
public:
	bool applyOnLobbyHandler(CServerHandler * handler, CPackForLobby & pack) const override
	{
		logGlobal->error("Cannot apply plain CPack!");
		assert(0);
		return false;
	}

	void applyOnLobbyScreen(CLobbyScreen * lobby, CServerHandler * handler, CPackForLobby & pack) const override
	{
		logGlobal->error("Cannot apply plain CPack!");
		assert(0);
	}
};

CServerHandler::~CServerHandler()
{
	networkHandler->stop();
	try
	{
		threadNetwork.join();
	}
	catch (const std::runtime_error & e)
	{
		logGlobal->error("Failed to shut down network thread! Reason: %s", e.what());
		assert(0);
	}
}

CServerHandler::CServerHandler()
	: applier(std::make_unique<CApplier<CBaseForLobbyApply>>())
	, lobbyClient(std::make_unique<GlobalLobbyClient>())
	, networkHandler(INetworkHandler::createHandler())
	, threadNetwork(&CServerHandler::threadRunNetwork, this)
	, state(EClientState::NONE)
	, campaignStateToSend(nullptr)
	, screenType(ESelectionScreen::unknown)
	, serverMode(EServerMode::NONE)
	, loadMode(ELoadMode::NONE)
	, client(nullptr)
{
	uuid = boost::uuids::to_string(boost::uuids::random_generator()());
	registerTypesLobbyPacks(*applier);
}

void CServerHandler::threadRunNetwork()
{
	logGlobal->info("Starting network thread");
	setThreadName("runNetwork");
	networkHandler->run();
	logGlobal->info("Ending network thread");
}

void CServerHandler::resetStateForLobby(EStartMode mode, ESelectionScreen screen, EServerMode newServerMode, const std::vector<std::string> & names)
{
	hostClientId = -1;
	state = EClientState::NONE;
	serverMode = newServerMode;
	mapToStart = nullptr;
	th = std::make_unique<CStopWatch>();
	c.reset();
	si = std::make_shared<StartInfo>();
	playerNames.clear();
	si->difficulty = 1;
	si->mode = mode;
	screenType = screen;
	myNames.clear();
	if(!names.empty()) //if have custom set of player names - use it
		myNames = names;
	else
		myNames.push_back(settings["general"]["playerName"].String());
}

GlobalLobbyClient & CServerHandler::getGlobalLobby()
{
	return *lobbyClient;
}

void CServerHandler::startLocalServerAndConnect(bool connectToLobby)
{
	if(threadRunLocalServer.joinable())
		threadRunLocalServer.join();

	th->update();

#if defined(SINGLE_PROCESS_APP)
	boost::condition_variable cond;
	std::vector<std::string> args{"--port=" + std::to_string(getLocalPort())};
	if(connectToLobby)
		args.push_back("--lobby");

	threadRunLocalServer = boost::thread([&cond, args] {
		setThreadName("CVCMIServer");
		CVCMIServer::create(&cond, args);
	});
#elif defined(VCMI_ANDROID)
	{
		CAndroidVMHelper envHelper;
		envHelper.callStaticVoidMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "startServer", true);
	}
#else
	threadRunLocalServer = boost::thread(&CServerHandler::threadRunServer, this, connectToLobby); //runs server executable;
#endif
	logNetwork->trace("Setting up thread calling server: %d ms", th->getDiff());

	th->update();

#ifdef SINGLE_PROCESS_APP
	{
#ifdef VCMI_IOS
		dispatch_sync(dispatch_get_main_queue(), ^{
			iOS_utils::showLoadingIndicator();
		});
#endif

		boost::mutex m;
		boost::unique_lock<boost::mutex> lock{m};
		logNetwork->info("waiting for server");
		cond.wait(lock);
		logNetwork->info("server is ready");

#ifdef VCMI_IOS
		dispatch_sync(dispatch_get_main_queue(), ^{
			iOS_utils::hideLoadingIndicator();
		});
#endif
	}
#elif defined(VCMI_ANDROID)
	logNetwork->info("waiting for server");
	while(!androidTestServerReadyFlag.load())
	{
		logNetwork->info("still waiting...");
		boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
	}
	logNetwork->info("waiting for server finished...");
	androidTestServerReadyFlag = false;
#endif
	logNetwork->trace("Waiting for server: %d ms", th->getDiff());

	th->update(); //put breakpoint here to attach to server before it does something stupid

	connectToServer(getLocalHostname(), getLocalPort());

	logNetwork->trace("\tConnecting to the server: %d ms", th->getDiff());
}

void CServerHandler::connectToServer(const std::string & addr, const ui16 port)
{
	logNetwork->info("Establishing connection to %s:%d...", addr, port);
	state = EClientState::CONNECTING;
	serverHostname = addr;
	serverPort = port;

	if (!isServerLocal())
	{
		Settings remoteAddress = settings.write["server"]["remoteHostname"];
		remoteAddress->String() = addr;

		Settings remotePort = settings.write["server"]["remotePort"];
		remotePort->Integer() = port;
	}

	networkHandler->connectToRemote(*this, addr, port);
}

void CServerHandler::onConnectionFailed(const std::string & errorMessage)
{
	if (isServerLocal())
	{
		// retry - local server might be still starting up
		logNetwork->debug("\nCannot establish connection. %s. Retrying...", errorMessage);
		networkHandler->createTimer(*this, std::chrono::milliseconds(100));
	}
	else
	{
		// remote server refused connection - show error message
		state = EClientState::CONNECTION_FAILED;
		CInfoWindow::showInfoDialog(CGI->generaltexth->translate("vcmi.mainMenu.serverConnectionFailed"), {});
	}
}

void CServerHandler::onTimer()
{
	if(state == EClientState::CONNECTION_CANCELLED)
	{
		logNetwork->info("Connection aborted by player!");
		return;
	}

	assert(isServerLocal());
	networkHandler->connectToRemote(*this, getLocalHostname(), getLocalPort());
}

void CServerHandler::onConnectionEstablished(const NetworkConnectionPtr & netConnection)
{
	networkConnection = netConnection;

	logNetwork->info("Connection established");

	if (serverMode == EServerMode::LOBBY_GUEST)
	{
		// say hello to lobby to switch connection to proxy mode
		getGlobalLobby().sendProxyConnectionLogin(netConnection);
	}

	c = std::make_shared<CConnection>(netConnection);
	nextClient = std::make_unique<CClient>();
	c->uuid = uuid;
	c->enterLobbyConnectionMode();
	c->setCallback(nextClient.get());
	sendClientConnecting();
}

void CServerHandler::applyPackOnLobbyScreen(CPackForLobby & pack)
{
	boost::mutex::scoped_lock interfaceLock(GH.interfaceMutex);
	const CBaseForLobbyApply * apply = applier->getApplier(CTypeList::getInstance().getTypeID(&pack)); //find the applier
	apply->applyOnLobbyScreen(dynamic_cast<CLobbyScreen *>(SEL), this, pack);
	GH.windows().totalRedraw();
}

std::set<PlayerColor> CServerHandler::getHumanColors()
{
	return clientHumanColors(c->connectionID);
}

PlayerColor CServerHandler::myFirstColor() const
{
	return clientFirstColor(c->connectionID);
}

bool CServerHandler::isMyColor(PlayerColor color) const
{
	return isClientColor(c->connectionID, color);
}

ui8 CServerHandler::myFirstId() const
{
	return clientFirstId(c->connectionID);
}

bool CServerHandler::isServerLocal() const
{
	return threadRunLocalServer.joinable();
}

bool CServerHandler::isHost() const
{
	return c && hostClientId == c->connectionID;
}

bool CServerHandler::isGuest() const
{
	return !c || hostClientId != c->connectionID;
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
	lcc.names = myNames;
	lcc.mode = si->mode;
	sendLobbyPack(lcc);
}

void CServerHandler::sendClientDisconnecting()
{
	// FIXME: This is workaround needed to make sure client not trying to sent anything to non existed server
	if(state == EClientState::DISCONNECTING)
	{
		assert(0);
		return;
	}

	state = EClientState::DISCONNECTING;
	mapToStart = nullptr;
	LobbyClientDisconnected lcd;
	lcd.clientId = c->connectionID;
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
	c->getConnection()->close();
	c.reset();
}

void CServerHandler::setCampaignState(std::shared_ptr<CampaignState> newCampaign)
{
	state = EClientState::LOBBY_CAMPAIGN;
	LobbySetCampaign lsc;
	lsc.ourCampaign = newCampaign;
	sendLobbyPack(lsc);
}

void CServerHandler::setCampaignMap(CampaignScenarioID mapId) const
{
	if(state == EClientState::GAMEPLAY) // FIXME: UI shouldn't sent commands in first place
		return;

	LobbySetCampaignMap lscm;
	lscm.mapId = mapId;
	sendLobbyPack(lscm);
}

void CServerHandler::setCampaignBonus(int bonusId) const
{
	if(state == EClientState::GAMEPLAY) // FIXME: UI shouldn't sent commands in first place
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
		LobbyChatMessage lcm;
		lcm.message = txt;
		lcm.playerName = playerNames.find(myFirstId())->second.name;
		sendLobbyPack(lcm);
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
	GH.windows().createAndPushWindow<CLoadingScreen>();
	
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
			errorMsg += VLC->generaltexth->translate("vcmi.server.errors.modsToEnable") + '\n';
			errorMsg += e.whatMissing();
		}
		if(!e.whatExcessive().empty())
		{
			errorMsg += VLC->generaltexth->translate("vcmi.server.errors.modsToDisable") + '\n';
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
		GH.windows().createAndPushWindow<CLoadingScreen>();
	
	LobbyStartGame lsg;
	if(client)
	{
		lsg.initializedStartInfo = std::make_shared<StartInfo>(* const_cast<StartInfo *>(client->getStartInfo(true)));
		lsg.initializedStartInfo->mode = EStartMode::NEW_GAME;
		lsg.initializedStartInfo->seedToBeUsed = lsg.initializedStartInfo->seedPostInit = 0;
		* si = * lsg.initializedStartInfo;
	}
	sendLobbyPack(lsg);
	c->enterLobbyConnectionMode();
}

void CServerHandler::startMapAfterConnection(std::shared_ptr<CMapInfo> to)
{
	mapToStart = to;
}

void CServerHandler::startGameplay(VCMI_LIB_WRAP_NAMESPACE(CGameState) * gameState)
{
	if(CMM)
		CMM->disable();

	std::swap(client, nextClient);

	highScoreCalc = nullptr;

	switch(si->mode)
	{
	case EStartMode::NEW_GAME:
		client->newGame(gameState);
		break;
	case EStartMode::CAMPAIGN:
		client->newGame(gameState);
		break;
	case EStartMode::LOAD_GAME:
		client->loadGame(gameState);
		break;
	default:
		throw std::runtime_error("Invalid mode");
	}
	// After everything initialized we can accept CPackToClient netpacks
	c->enterGameplayConnectionMode(client->gameState());
	state = EClientState::GAMEPLAY;
}

void CServerHandler::endGameplay()
{
	// Game is ending
	// Tell the network thread to reach a stable state
	CSH->sendClientDisconnecting();
	logNetwork->info("Closed connection.");

	client->endGame();
	client.reset();

	if(CMM)
	{
		GH.curInt = CMM.get();
		CMM->enable();
	}
	else
	{
		GH.curInt = CMainMenu::create().get();
	}
}

void CServerHandler::restartGameplay()
{
	client->endGame();
	client.reset();

	nextClient = std::make_unique<CClient>();
	c->setCallback(nextClient.get());
	c->enterLobbyConnectionMode();
}

void CServerHandler::startCampaignScenario(HighScoreParameter param, std::shared_ptr<CampaignState> cs)
{
	std::shared_ptr<CampaignState> ourCampaign = cs;

	if (!cs)
		ourCampaign = si->campState;

	if(highScoreCalc == nullptr)
	{
		highScoreCalc = std::make_shared<HighScoreCalculation>();
		highScoreCalc->isCampaign = true;
		highScoreCalc->parameters.clear();
	}
	param.campaignName = cs->getNameTranslated();
	highScoreCalc->parameters.push_back(param);

	GH.dispatchMainThread([ourCampaign, this]()
	{
		CSH->endGameplay();

		auto & epilogue = ourCampaign->scenario(*ourCampaign->lastScenario()).epilog;
		auto finisher = [=]()
		{
			if(ourCampaign->campaignSet != "" && ourCampaign->isCampaignFinished())
			{
				Settings entry = persistentStorage.write["completedCampaigns"][ourCampaign->getFilename()];
				entry->Bool() = true;
			}

			GH.windows().pushWindow(CMM);
			GH.windows().pushWindow(CMM->menu);

			if(!ourCampaign->isCampaignFinished())
				CMM->openCampaignLobby(ourCampaign);
			else
			{
				CMM->openCampaignScreen(ourCampaign->campaignSet);
				GH.windows().createAndPushWindow<CHighScoreInputScreen>(true, *highScoreCalc);
			}
		};

		threadRunLocalServer.join();
		if(epilogue.hasPrologEpilog)
		{
			GH.windows().createAndPushWindow<CPrologEpilogVideo>(epilogue, finisher);
		}
		else
		{
			finisher();
		}
	});
}

void CServerHandler::showServerError(const std::string & txt) const
{
	if(auto w = GH.windows().topWindow<CLoadingScreen>())
		GH.windows().popWindow(w);
	
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
	if(loadMode != ELoadMode::TUTORIAL && state == EClientState::GAMEPLAY)
	{
		if(si->campState)
			return ELoadMode::CAMPAIGN;
		for(auto pn : playerNames)
		{
			if(pn.second.connection != c->connectionID)
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

	boost::this_thread::sleep_for(boost::chrono::milliseconds(100));

	while(!settings["session"]["headless"].Bool() && !GH.windows().topWindow<CLobbyScreen>())
		boost::this_thread::sleep_for(boost::chrono::milliseconds(50));

	while(!mi || mapInfo->fileURI != CSH->mi->fileURI)
	{
		setMapInfo(mapInfo);
		boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
	}
	// "Click" on color to remove us from it
	setPlayer(myFirstColor());
	while(myFirstColor() != PlayerColor::CANNOT_DETERMINE)
		boost::this_thread::sleep_for(boost::chrono::milliseconds(50));

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
		boost::this_thread::sleep_for(boost::chrono::milliseconds(50));
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

	virtual bool callTyped() override { return false; }

	virtual void visitForLobby(CPackForLobby & lobbyPack) override
	{
		handler.visitForLobby(lobbyPack);
	}

	virtual void visitForClient(CPackForClient & clientPack) override
	{
		handler.visitForClient(clientPack);
	}
};

void CServerHandler::onPacketReceived(const std::shared_ptr<INetworkConnection> &, const std::vector<std::byte> & message)
{
	if(state == EClientState::DISCONNECTING)
	{
		assert(0); //Should not be possible - socket must be closed at this point
		return;
	}

	CPack * pack = c->retrievePack(message);
	ServerHandlerCPackVisitor visitor(*this);
	pack->visit(visitor);
}

void CServerHandler::onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage)
{
	networkConnection.reset();

	if(state == EClientState::DISCONNECTING)
	{
		logNetwork->info("Successfully closed connection to server, ending listening thread!");
	}
	else
	{
		logNetwork->error("Lost connection to server, ending listening thread! Connection has been closed");

		if(client)
		{
			state = EClientState::DISCONNECTING;

			GH.dispatchMainThread([]()
			{
				CSH->endGameplay();
				GH.defActionsDef = 63;
				CMM->menu->switchToTab("main");
			});
		}
		else
		{
			LobbyClientDisconnected lcd;
			lcd.clientId = c->connectionID;
			applyPackOnLobbyScreen(lcd);
		}
	}
}

void CServerHandler::visitForLobby(CPackForLobby & lobbyPack)
{
	if(applier->getApplier(CTypeList::getInstance().getTypeID(&lobbyPack))->applyOnLobbyHandler(this, lobbyPack))
	{
		if(!settings["session"]["headless"].Bool())
			applyPackOnLobbyScreen(lobbyPack);
	}
}

void CServerHandler::visitForClient(CPackForClient & clientPack)
{
	client->handlePack(&clientPack);
}

void CServerHandler::threadRunServer(bool connectToLobby)
{
#if !defined(VCMI_MOBILE)
	setThreadName("runServer");
	const std::string logName = (VCMIDirs::get().userLogsPath() / "server_log.txt").string();
	std::string comm = VCMIDirs::get().serverPath().string()
		+ " --port=" + std::to_string(getLocalPort())
		+ " --run-by-client";
	if(connectToLobby)
		comm += " --lobby";

	comm += " > \"" + logName + '\"';
	logGlobal->info("Server command line: %s", comm);

#ifdef VCMI_WINDOWS
	int result = -1;
	const auto bufSize = ::MultiByteToWideChar(CP_UTF8, 0, comm.c_str(), comm.size(), nullptr, 0);
	if(bufSize > 0)
	{
		std::wstring wComm(bufSize, {});
		const auto convertResult = ::MultiByteToWideChar(CP_UTF8, 0, comm.c_str(), comm.size(), &wComm[0], bufSize);
		if(convertResult > 0)
			result = ::_wsystem(wComm.c_str());
		else
			logNetwork->error("Error " + std::to_string(GetLastError()) + ": failed to convert server launch command to wide string: " + comm);
	}
	else
		logNetwork->error("Error " + std::to_string(GetLastError()) + ": failed to obtain buffer length to convert server launch command to wide string : " + comm);
#else
	int result = std::system(comm.c_str());
#endif
	if (result == 0)
	{
		logNetwork->info("Server closed correctly");
	}
	else
	{
		if (state != EClientState::DISCONNECTING)
		{
			if (state == EClientState::CONNECTING)
				CInfoWindow::showInfoDialog(CGI->generaltexth->translate("vcmi.server.errors.existingProcess"), {});
			else
				CInfoWindow::showInfoDialog(CGI->generaltexth->translate("vcmi.server.errors.serverCrashed"), {});
		}

		state = EClientState::CONNECTION_CANCELLED; // stop attempts to reconnect
		logNetwork->error("Error: server failed to close correctly or crashed!");
		logNetwork->error("Check %s for more info", logName);
	}
#endif
}

void CServerHandler::sendLobbyPack(const CPackForLobby & pack) const
{
	if(state != EClientState::STARTING)
		c->sendPack(&pack);
}
