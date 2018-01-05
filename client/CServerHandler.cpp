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

#include "../lib/CConfigHandler.h"
#include "../lib/CThreadHelper.h"
#ifndef VCMI_ANDROID
#include "../lib/Interprocess.h"
#endif
#include "../lib/NetPacks.h"
#include "../lib/VCMIDirs.h"
#include "../lib/serializer/Connection.h"

#include "../lib/StartInfo.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "../lib/rmg/CMapGenOptions.h"
#include "../lib/mapping/CCampaignHandler.h"

#include "CGameInfo.h"
#include "../lib/mapping/CMap.h"
#include "../lib/mapping/CMapInfo.h"
#include "../lib/CGeneralTextHandler.h"

// For map options
#include "../lib/CHeroHandler.h"

// netpacks serialization
#include "../lib/CCreatureHandler.h"

// Applier
#include "../lib/registerTypes/RegisterTypes.h"
#include "../lib/NetPacks.h"
#include "lobby/CSelectionBase.h"
#include "lobby/CLobbyScreen.h"

// To sent netpacks to client
#include "Client.h"

// FIXME: For pushing events
#include "gui/CGuiHandler.h"

// For startGame
#include "CPlayerInterface.h"

// MPTODO: for campaign advance
#include "../lib/serializer/CMemorySerializer.h"

// getLoadMode
#include "mainmenu/CMainMenu.h"

// CGCreature
#include "../lib/mapObjects/MiscObjects.h"

// For debug save start
#include "../lib/serializer/CSerializer.h"

template<typename T> class CApplyOnLobby;

class CBaseForLobbyApply
{
public:
	virtual bool applyImmidiately(CLobbyScreen * selScr, void * pack) const = 0;
	virtual void applyOnLobby(CLobbyScreen * selScr, void * pack) const = 0;
	virtual ~CBaseForLobbyApply(){};
	template<typename U> static CBaseForLobbyApply * getApplier(const U * t = nullptr)
	{
		return new CApplyOnLobby<U>();
	}
};

template<typename T> class CApplyOnLobby : public CBaseForLobbyApply
{
public:
	bool applyImmidiately(CLobbyScreen * selScr, void * pack) const override
	{
		T * ptr = static_cast<T *>(pack);
		logNetwork->trace("\tImmidiately apply on lobby: %s", typeList.getTypeInfo(ptr)->name());
		return ptr->applyOnLobbyImmidiately(selScr);
	}

	void applyOnLobby(CLobbyScreen * selScr, void * pack) const override
	{
		T * ptr = static_cast<T *>(pack);
		logNetwork->trace("\tApply on lobby from queue: %s", typeList.getTypeInfo(ptr)->name());
		ptr->applyOnLobby(selScr);
	}
};

template<> class CApplyOnLobby<CPack>: public CBaseForLobbyApply
{
public:
	bool applyImmidiately(CLobbyScreen * selScr, void * pack) const override
	{
		logGlobal->error("Cannot apply plain CPack!");
		assert(0);
		return false;
	}

	void applyOnLobby(CLobbyScreen * selScr, void * pack) const override
	{
		logGlobal->error("Cannot apply plain CPack!");
		assert(0);
	}
};

extern std::string NAME;

CServerHandler::CServerHandler()
	: state(EClientState::NONE), threadRunLocalServer(nullptr), shm(nullptr), threadConnectionToServer(nullptr), mx(new boost::recursive_mutex), client(nullptr), loadMode(0), campaignState(nullptr), campaignPassed(false), campaignSent(false)
{
	uuid = boost::uuids::to_string(boost::uuids::random_generator()());
	applier = new CApplier<CBaseForLobbyApply>();
	registerTypesLobbyPacks(*applier);
}

CServerHandler::~CServerHandler()
{
	vstd::clear_pointer(mx);
	vstd::clear_pointer(shm);
	vstd::clear_pointer(threadRunLocalServer);
	vstd::clear_pointer(threadConnectionToServer);
	vstd::clear_pointer(applier);
}

void CServerHandler::resetStateForLobby(const StartInfo::EMode mode, const std::vector<std::string> * names)
{
	campaignPassed = false;
	campaignSent = false;
	state = EClientState::NONE;
	th = make_unique<CStopWatch>();
	incomingPacks.clear();
	c.reset();
	si.reset(new StartInfo());
	playerNames.clear();
	si->difficulty = 1;
	si->mode = mode;
	myNames.clear();
	if(names && !names->empty()) //if have custom set of player names - use it
		myNames = *names;
	else
		myNames.push_back(settings["general"]["playerName"].String());

#ifndef VCMI_ANDROID
	if(shm)
		vstd::clear_pointer(shm);

	if(!settings["session"]["disable-shm"].Bool())
	{
		std::string sharedMemoryName = "vcmi_memory";
		if(settings["session"]["enable-shm-uuid"].Bool())
		{
			//used or automated testing when multiple clients start simultaneously
			sharedMemoryName += "_" + uuid;
		}
		try
		{
			shm = new SharedMemory(sharedMemoryName, true);
		}
		catch(...)
		{
			vstd::clear_pointer(shm);
			logNetwork->error("Cannot open interprocess memory. Continue without it...");
		}
	}
#endif
}

void CServerHandler::startLocalServerAndConnect()
{
	if(threadRunLocalServer)
		threadRunLocalServer->join();

	th->update();
#ifdef VCMI_ANDROID
	CAndroidVMHelper envHelper;
	envHelper.callStaticVoidMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "startServer", true);
#else
	threadRunLocalServer = new boost::thread(&CServerHandler::threadRunServer, this); //runs server executable;
#endif
	logNetwork->trace("Setting up thread calling server: %d ms", th->getDiff());

	th->update();

#ifndef VCMI_ANDROID
	if(shm)
		shm->sr->waitTillReady();
#else
	logNetwork->info("waiting for server");
	while (!androidTestServerReadyFlag.load())
	{
		logNetwork->info("still waiting...");
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	}
	logNetwork->info("waiting for server finished...");
	androidTestServerReadyFlag = false;
#endif
	logNetwork->trace("Waiting for server: %d ms", th->getDiff());

	th->update(); //put breakpoint here to attach to server before it does something stupid

#ifndef VCMI_ANDROID
	justConnectToServer(settings["server"]["server"].String(), shm ? shm->sr->port : 0);
#else
	justConnectToServer(settings["server"]["server"].String());
#endif

	logNetwork->trace("\tConnecting to the server: %d ms", th->getDiff());
}

void CServerHandler::justConnectToServer(const std::string & addr, const ui16 port)
{
	state = EClientState::CONNECTING;
	while(!c && state != EClientState::CONNECTION_CANCELLED)
	{
		try
		{
			logNetwork->info("Establishing connection...");
			c = std::make_shared<CConnection>(
					addr.size() ? addr : settings["server"]["server"].String(),
					port ? port : getDefaultPort(),
					NAME, uuid);
		}
		catch(...)
		{
			logNetwork->error("\nCannot establish connection! Retrying within 1 second");
			boost::this_thread::sleep(boost::posix_time::seconds(1));
		}
	}
	if(state == EClientState::CONNECTION_CANCELLED)
	{
		logNetwork->info("Connection aborted by player!");
		return;
	}
	threadConnectionToServer = new boost::thread(&CServerHandler::threadHandleConnection, this);
}

void CServerHandler::processIncomingPacks()
{
	if(!threadConnectionToServer)
		return;

	boost::unique_lock<boost::recursive_mutex> lock(*mx);
	while(!incomingPacks.empty())
	{
		CPackForLobby * pack = incomingPacks.front();
		incomingPacks.pop_front();
		CBaseForLobbyApply * apply = applier->getApplier(typeList.getTypeID(pack)); //find the applier
		apply->applyOnLobby(static_cast<CLobbyScreen *>(SEL), pack);
		delete pack;
	}
}

void CServerHandler::stopServerConnection()
{
	if(threadConnectionToServer)
	{
		while(!threadConnectionToServer->timed_join(boost::posix_time::milliseconds(50)))
			processIncomingPacks();
		threadConnectionToServer->join();
		vstd::clear_pointer(threadConnectionToServer);
	}
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
	if(threadRunLocalServer)
		return true;

	return false;
}

bool CServerHandler::isHost() const
{
	return c && hostClientId == c->connectionID;
}

bool CServerHandler::isGuest() const
{
	return !c || hostClientId != c->connectionID;
}

ui16 CServerHandler::getDefaultPort()
{
	if(settings["session"]["serverport"].Integer())
		return settings["session"]["serverport"].Integer();
	else
		return settings["server"]["port"].Integer();
}

std::string CServerHandler::getDefaultPortStr()
{
	return boost::lexical_cast<std::string>(getDefaultPort());
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
	state = EClientState::DISCONNECTING;
	LobbyClientDisconnected lcd;
	lcd.clientId = c->connectionID;
	lcd.shutdownServer = isServerLocal();
	sendLobbyPack(lcd);
}

void CServerHandler::setCampaignState(std::shared_ptr<CCampaignState> newCampaign)
{
	state = EClientState::LOBBY_CAMPAIGN;
	LobbySetCampaign lsc;
	lsc.ourCampaign = newCampaign;
	sendLobbyPack(lsc);
}

void CServerHandler::setCampaignMap(int mapId) const
{
	LobbySetCampaignMap lscm;
	lscm.mapId = mapId;
	sendLobbyPack(lscm);
}

void CServerHandler::setCampaignBonus(int bonusId) const
{
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

void CServerHandler::setPlayerOption(ui8 what, ui8 dir, PlayerColor player) const
{
	LobbyChangePlayerOption lcpo;
	lcpo.what = what;
	lcpo.direction = dir;
	lcpo.color = player;
	sendLobbyPack(lcpo);
}

void CServerHandler::setDifficulty(int to) const
{
	LobbySetDifficulty lsd;
	lsd.difficulty = to;
	sendLobbyPack(lsd);
}

void CServerHandler::setTurnLength(int npos) const
{
	vstd::amin(npos, ARRAY_COUNT(GameConstants::POSSIBLE_TURNTIME) - 1);
	LobbySetTurnTime lstt;
	lstt.turnTime = GameConstants::POSSIBLE_TURNTIME[npos];
	sendLobbyPack(lstt);
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
		std::string connectedId, playerColorId;
		readed >> connectedId;
		readed >> playerColorId;
		if(connectedId.length(), playerColorId.length())
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

void CServerHandler::sendStartGame() const
{
	verifyStateBeforeStart(settings["session"]["onlyai"].Bool());
	LobbyStartGame lsg;
	sendLobbyPack(lsg);
}

void CServerHandler::startGameplay()
{
	client = new CClient();

	switch(si->mode)
	{
	case StartInfo::NEW_GAME:
		client->newGame();
		break;
	case StartInfo::CAMPAIGN:
		client->newGame();
		break;
	case StartInfo::LOAD_GAME:
		client->loadGame();
		break;
	}
	// After everything initialized we can accept CPackToClient netpacks
	c->enterGameplayConnectionMode(client->gameState());
	state = EClientState::GAMEPLAY;
}

void CServerHandler::endGameplay()
{
	client->endGame();
	vstd::clear_pointer(client);
}

void CServerHandler::startCampaignScenario(std::shared_ptr<CCampaignState> cs)
{
	SDL_Event event;
	event.type = SDL_USEREVENT;
	event.user.code = EUserEvent::CAMPAIGN_START_SCENARIO;
	if(cs)
		event.user.data1 = CMemorySerializer::deepCopy(*cs.get()).release();
	else
		event.user.data1 = CMemorySerializer::deepCopy(*si->campState.get()).release();
	SDL_PushEvent(&event);
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

ui8 CServerHandler::getLoadMode()
{
	if(state == EClientState::GAMEPLAY)
	{
		if(si->campState)
			return ELoadMode::CAMPAIGN;
		for(auto pn : playerNames)
		{
			if(pn.second.connection != c->connectionID)
				return ELoadMode::MULTI;
		}

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
		resetStateForLobby(StartInfo::LOAD_GAME);
		// FIXME: copy-paste from SelectionTab::parseSaves
		auto file = ResourceID(filename, EResType::CLIENT_SAVEGAME);
		CLoadFile lf(*CResourceHandler::get()->getResourceName(file), MINIMAL_SERIALIZATION_VERSION);
		lf.checkMagicBytes(SAVEGAME_MAGIC);
		mapInfo->mapHeader = make_unique<CMapHeader>();
		mapInfo->scenarioOpts = nullptr; //to be created by serialiser
		lf >> *(mapInfo->mapHeader.get()) >> mapInfo->scenarioOpts;
		mapInfo->fileURI = file.getName();
		mapInfo->countPlayers();
		std::time_t time = boost::filesystem::last_write_time(*CResourceHandler::get()->getResourceName(file));
		mapInfo->date = std::asctime(std::localtime(&time));
		mapInfo->isSaveGame = true;
		mapInfo->mapHeader->triggeredEvents.clear();
		screenType = ESelectionScreen::loadGame;
	}
	else
	{
		resetStateForLobby(StartInfo::NEW_GAME);
		mapInfo->mapInit(filename);
		screenType = ESelectionScreen::newGame;
	}
	if(settings["session"]["donotstartserver"].Bool())
		justConnectToServer("127.0.0.1", 3030);
	else
		startLocalServerAndConnect();

	while(!dynamic_cast<CLobbyScreen *>(GH.topInt()))
	{
		processIncomingPacks();
		boost::this_thread::sleep(boost::posix_time::milliseconds(50));
	}
	while(!mi || mapInfo->fileURI != CSH->mi->fileURI)
	{
		setMapInfo(mapInfo);
		processIncomingPacks();
		boost::this_thread::sleep(boost::posix_time::milliseconds(50));
	}
	// "Click" on color to remove us from it
	setPlayer(myFirstColor());
	while(myFirstColor() != PlayerColor::CANNOT_DETERMINE)
	{
		processIncomingPacks();
		boost::this_thread::sleep(boost::posix_time::milliseconds(50));
	}

	while(true)
	{
		try
		{
			processIncomingPacks();
			sendStartGame();
			break;
		}
		catch(...)
		{

		}
		boost::this_thread::sleep(boost::posix_time::milliseconds(50));
	}
	while(state != EClientState::GAMEPLAY)
	{
		processIncomingPacks();
	}
}

void CServerHandler::threadHandleConnection()
{
	setThreadName("CServerHandler::threadHandleConnection");
	c->enterLobbyConnectionMode();

	try
	{
		sendClientConnecting();
		while(c->connected)
		{
			while(state == EClientState::STARTING)
				boost::this_thread::sleep(boost::posix_time::milliseconds(10));

			CPack * pack = c->retreivePack();
			if(state == EClientState::DISCONNECTING)
			{
				// FIXME: server shouldn't really send netpacks after it's tells client to disconnect
				// Though currently they'll be delivered and might cause crash.
				vstd::clear_pointer(pack);
			}
			else if(auto lobbyPack = dynamic_cast<CPackForLobby *>(pack))
			{
				if(applier->getApplier(typeList.getTypeID(pack))->applyImmidiately(static_cast<CLobbyScreen *>(SEL), pack))
				{
					boost::unique_lock<boost::recursive_mutex> lock(*mx);
					incomingPacks.push_back(lobbyPack);
				}
			}
			else if(auto clientPack = dynamic_cast<CPackForClient *>(pack))
			{
				client->handlePack(clientPack);
			}
		}
	}
	//catch only asio exceptions
	catch(const boost::system::system_error & e)
	{
		if(state == EClientState::DISCONNECTING)
		{
			logNetwork->info("Successfully closed connection to server, ending listening thread!");
		}
		else
		{
			logNetwork->error("Lost connection to server, ending listening thread!");
			logNetwork->error(e.what());
			if(client)
			{
				CGuiHandler::pushSDLEvent(SDL_USEREVENT, EUserEvent::RETURN_TO_MAIN_MENU);
			}
			else
			{
				auto lcd = new LobbyClientDisconnected();
				lcd->clientId = c->connectionID;
				boost::unique_lock<boost::recursive_mutex> lock(*mx);
				incomingPacks.push_back(lcd);
			}
		}
	}
	catch(...)
	{
		handleException();
		throw;
	}
}

void CServerHandler::threadRunServer()
{
#ifndef VCMI_ANDROID
	setThreadName("CServerHandler::threadRunServer");
	const std::string logName = (VCMIDirs::get().userCachePath() / "server_log.txt").string();
	std::string comm = VCMIDirs::get().serverPath().string()
		+ " --port=" + getDefaultPortStr()
		+ " --run-by-client"
		+ " --uuid=" + uuid;
	if(shm)
	{
		comm += " --enable-shm";
		if(settings["session"]["enable-shm-uuid"].Bool())
			comm += " --enable-shm-uuid";
	}
	comm += " > \"" + logName + '\"';

	int result = std::system(comm.c_str());
	if (result == 0)
	{
		logNetwork->info("Server closed correctly");
	}
	else
	{
		logNetwork->error("Error: server failed to close correctly or crashed!");
		logNetwork->error("Check %s for more info", logName);
	}
	vstd::clear_pointer(threadRunLocalServer);
#endif
}

void CServerHandler::sendLobbyPack(const CPackForLobby & pack) const
{
	if(state != EClientState::STARTING)
		c->sendPack(&pack);
}
