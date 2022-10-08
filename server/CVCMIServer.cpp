/*
 * CVCMIServer.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include <boost/asio.hpp>

#include "../lib/filesystem/Filesystem.h"
#include "../lib/mapping/CCampaignHandler.h"
#include "../lib/CThreadHelper.h"
#include "../lib/serializer/Connection.h"
#include "../lib/CModHandler.h"
#include "../lib/CArtHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/CBuildingHandler.h"
#include "../lib/spells/CSpellHandler.h"
#include "../lib/CCreatureHandler.h"
#include "zlib.h"
#include "CVCMIServer.h"
#include "../lib/StartInfo.h"
#include "../lib/mapping/CMap.h"
#include "../lib/rmg/CMapGenOptions.h"
#ifdef VCMI_ANDROID
#include "lib/CAndroidVMHelper.h"
#elif !defined(VCMI_IOS)
#include "../lib/Interprocess.h"
#endif
#include "../lib/VCMI_Lib.h"
#include "../lib/VCMIDirs.h"
#include "CGameHandler.h"
#include "../lib/mapping/CMapInfo.h"
#include "../lib/GameConstants.h"
#include "../lib/logging/CBasicLogConfigurator.h"
#include "../lib/CConfigHandler.h"
#include "../lib/ScopeGuard.h"
#include "../lib/serializer/CMemorySerializer.h"
#include "../lib/serializer/Cast.h"

#include "../lib/UnlockGuard.h"

// for applier
#include "../lib/registerTypes/RegisterTypes.h"

// UUID generation
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "../lib/CGameState.h"

#if defined(__GNUC__) && !defined(__MINGW32__) && !defined(VCMI_ANDROID) && !defined(VCMI_IOS)
#include <execinfo.h>
#endif

template<typename T> class CApplyOnServer;

class CBaseForServerApply
{
public:
	virtual bool applyOnServerBefore(CVCMIServer * srv, void * pack) const =0;
	virtual void applyOnServerAfter(CVCMIServer * srv, void * pack) const =0;
	virtual ~CBaseForServerApply() {}
	template<typename U> static CBaseForServerApply * getApplier(const U * t = nullptr)
	{
		return new CApplyOnServer<U>();
	}
};

template <typename T> class CApplyOnServer : public CBaseForServerApply
{
public:
	bool applyOnServerBefore(CVCMIServer * srv, void * pack) const override
	{
		T * ptr = static_cast<T *>(pack);
		if(ptr->checkClientPermissions(srv))
		{
			boost::unique_lock<boost::mutex> stateLock(srv->stateMutex);
			return ptr->applyOnServer(srv);
		}
		else
			return false;
	}

	void applyOnServerAfter(CVCMIServer * srv, void * pack) const override
	{
		T * ptr = static_cast<T *>(pack);
		ptr->applyOnServerAfterAnnounce(srv);
	}
};

template <>
class CApplyOnServer<CPack> : public CBaseForServerApply
{
public:
	bool applyOnServerBefore(CVCMIServer * srv, void * pack) const override
	{
		logGlobal->error("Cannot apply plain CPack!");
		assert(0);
		return false;
	}
	void applyOnServerAfter(CVCMIServer * srv, void * pack) const override
	{
		logGlobal->error("Cannot apply plain CPack!");
		assert(0);
	}
};

std::string SERVER_NAME_AFFIX = "server";
std::string SERVER_NAME = GameConstants::VCMI_VERSION + std::string(" (") + SERVER_NAME_AFFIX + ')';

CVCMIServer::CVCMIServer(boost::program_options::variables_map & opts)
	: port(3030), io(std::make_shared<boost::asio::io_service>()), state(EServerState::LOBBY), cmdLineOptions(opts), currentClientId(1), currentPlayerId(1), restartGameplay(false)
{
	uuid = boost::uuids::to_string(boost::uuids::random_generator()());
	logNetwork->trace("CVCMIServer created! UUID: %s", uuid);
	applier = std::make_shared<CApplier<CBaseForServerApply>>();
	registerTypesLobbyPacks(*applier);

	if(cmdLineOptions.count("port"))
		port = cmdLineOptions["port"].as<ui16>();
	logNetwork->info("Port %d will be used", port);
	try
	{
		acceptor = std::make_shared<TAcceptor>(*io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port));
	}
	catch(...)
	{
		logNetwork->info("Port %d is busy, trying to use random port instead", port);
		if(cmdLineOptions.count("run-by-client") && !cmdLineOptions.count("enable-shm"))
		{
			logNetwork->error("Cant pass port number to client without shared memory!", port);
			exit(0);
		}
		acceptor = std::make_shared<TAcceptor>(*io, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 0));
		port = acceptor->local_endpoint().port();
	}
	logNetwork->info("Listening for connections at port %d", port);
}

CVCMIServer::~CVCMIServer()
{
	announceQueue.clear();

	if(announceLobbyThread)
		announceLobbyThread->join();
}

void CVCMIServer::run()
{
	if(!restartGameplay)
	{
		this->announceLobbyThread = vstd::make_unique<boost::thread>(&CVCMIServer::threadAnnounceLobby, this);
#if !defined(VCMI_ANDROID) && !defined(VCMI_IOS)
		if(cmdLineOptions.count("enable-shm"))
		{
			std::string sharedMemoryName = "vcmi_memory";
			if(cmdLineOptions.count("enable-shm-uuid") && cmdLineOptions.count("uuid"))
			{
				sharedMemoryName += "_" + cmdLineOptions["uuid"].as<std::string>();
			}
			shm = std::make_shared<SharedMemory>(sharedMemoryName);
		}
#endif

		startAsyncAccept();

#if defined(VCMI_ANDROID)
		CAndroidVMHelper vmHelper;
		vmHelper.callStaticVoidMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "onServerReady");
#elif !defined(VCMI_IOS)
		if(shm)
		{
			shm->sr->setToReadyAndNotify(port);
		}
#endif
	}

	while(state == EServerState::LOBBY || state == EServerState::GAMEPLAY_STARTING)
		boost::this_thread::sleep(boost::posix_time::milliseconds(50));

	logNetwork->info("Thread handling connections ended");

	if(state == EServerState::GAMEPLAY)
	{
		gh->run(si->mode == StartInfo::LOAD_GAME);
	}
	while(state == EServerState::GAMEPLAY_ENDED)
		boost::this_thread::sleep(boost::posix_time::milliseconds(50));
}

void CVCMIServer::threadAnnounceLobby()
{
	while(state != EServerState::SHUTDOWN)
	{
		{
			boost::unique_lock<boost::recursive_mutex> myLock(mx);
			while(!announceQueue.empty())
			{
				announcePack(std::move(announceQueue.front()));
				announceQueue.pop_front();
			}
			if(state != EServerState::LOBBY)
			{
				if(acceptor)
					acceptor->close();
			}

			if(acceptor)
			{
				io->reset();
				io->poll();
			}
		}

		boost::this_thread::sleep(boost::posix_time::milliseconds(50));
	}
}

void CVCMIServer::prepareToRestart()
{
	if(state == EServerState::GAMEPLAY)
	{
		restartGameplay = true;
		* si = * gh->gs->initialOpts;
		si->seedToBeUsed = si->seedPostInit = 0;
		state = EServerState::LOBBY;
		// FIXME: dirry hack to make sure old CGameHandler::run is finished
		boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
	}
	
	for(auto c : connections)
	{
		c->enterLobbyConnectionMode();
		c->disableStackSendingByID();
	}
	boost::unique_lock<boost::recursive_mutex> queueLock(mx);
	gh = nullptr;
}

bool CVCMIServer::prepareToStartGame()
{
	gh = std::make_shared<CGameHandler>(this);
	switch(si->mode)
	{
	case StartInfo::CAMPAIGN:
		logNetwork->info("Preparing to start new campaign");
		si->campState->currentMap = boost::make_optional(campaignMap);
		si->campState->chosenCampaignBonuses[campaignMap] = campaignBonus;
		gh->init(si.get());
		break;

	case StartInfo::NEW_GAME:
		logNetwork->info("Preparing to start new game");
		gh->init(si.get());
		break;

	case StartInfo::LOAD_GAME:
		logNetwork->info("Preparing to start loaded game");
		if(!gh->load(si->mapname))
			return false;
		break;
	default:
		logNetwork->error("Wrong mode in StartInfo!");
		assert(0);
		break;
	}
	
	state = EServerState::GAMEPLAY_STARTING;
	return true;
}

void CVCMIServer::startGameImmidiately()
{
	for(auto c : connections)
		c->enterGameplayConnectionMode(gh->gs);

	state = EServerState::GAMEPLAY;
}

void CVCMIServer::startAsyncAccept()
{
	assert(!upcomingConnection);
	assert(acceptor);

#if BOOST_VERSION >= 107000  // Boost version >= 1.70
	upcomingConnection = std::make_shared<TSocket>(acceptor->get_executor());
#else
	upcomingConnection = std::make_shared<TSocket>(acceptor->get_io_service());
#endif
	acceptor->async_accept(*upcomingConnection, std::bind(&CVCMIServer::connectionAccepted, this, _1));
}

void CVCMIServer::connectionAccepted(const boost::system::error_code & ec)
{
	if(ec)
	{
		if(state != EServerState::SHUTDOWN)
			logNetwork->info("Something wrong during accepting: %s", ec.message());
		return;
	}

	try
	{
		logNetwork->info("We got a new connection! :)");
		auto c = std::make_shared<CConnection>(upcomingConnection, SERVER_NAME, uuid);
		upcomingConnection.reset();
		connections.insert(c);
		c->handler = std::make_shared<boost::thread>(&CVCMIServer::threadHandleClient, this, c);
	}
	catch(std::exception & e)
	{
		logNetwork->error("Failure processing new connection! %s", e.what());
		upcomingConnection.reset();
	}

	startAsyncAccept();
}

void CVCMIServer::threadHandleClient(std::shared_ptr<CConnection> c)
{
	setThreadName("CVCMIServer::handleConnection");
	c->enterLobbyConnectionMode();

#ifndef _MSC_VER
	try
	{
#endif
		while(c->connected)
		{
			CPack * pack;
			
			try
			{
				pack = c->retrievePack();
			}
			catch(boost::system::system_error & e)
			{
				logNetwork->error("Network error receiving a pack. Connection %s dies. What happened: %s", c->toString(), e.what());

				if(state != EServerState::LOBBY)
					gh->handleClientDisconnection(c);

				break;
			}
			
			if(auto lobbyPack = dynamic_ptr_cast<CPackForLobby>(pack))
			{
				handleReceivedPack(std::unique_ptr<CPackForLobby>(lobbyPack));
			}
			else if(auto serverPack = dynamic_ptr_cast<CPackForServer>(pack))
			{
				gh->handleReceivedPack(serverPack);
			}
		}
#ifndef _MSC_VER
	 }
	catch(const std::exception & e)
	{
        (void)e;
		boost::unique_lock<boost::recursive_mutex> queueLock(mx);
		logNetwork->error("%s dies... \nWhat happened: %s", c->toString(), e.what());
	}
	catch(...)
	{
		state = EServerState::SHUTDOWN;
		handleException();
		throw;
	}
#endif

	boost::unique_lock<boost::recursive_mutex> queueLock(mx);
//	if(state != ENDING_AND_STARTING_GAME)
	if(c->connected)
	{
		auto lcd = vstd::make_unique<LobbyClientDisconnected>();
		lcd->c = c;
		lcd->clientId = c->connectionID;
		handleReceivedPack(std::move(lcd));
	}

	logNetwork->info("Thread listening for %s ended", c->toString());
	c->handler.reset();
}

void CVCMIServer::handleReceivedPack(std::unique_ptr<CPackForLobby> pack)
{
	CBaseForServerApply * apply = applier->getApplier(typeList.getTypeID(pack.get()));
	if(apply->applyOnServerBefore(this, pack.get()))
		addToAnnounceQueue(std::move(pack));
}

void CVCMIServer::announcePack(std::unique_ptr<CPackForLobby> pack)
{
	for(auto c : connections)
	{
		// FIXME: we need to avoid sending something to client that not yet get answer for LobbyClientConnected
		// Until UUID set we only pass LobbyClientConnected to this client
		if(c->uuid == uuid && !dynamic_cast<LobbyClientConnected *>(pack.get()))
			continue;

		c->sendPack(pack.get());
	}

	applier->getApplier(typeList.getTypeID(pack.get()))->applyOnServerAfter(this, pack.get());
}

void CVCMIServer::announceMessage(const std::string & txt)
{
	logNetwork->info("Show message: %s", txt);
	auto cm = vstd::make_unique<LobbyShowMessage>();
	cm->message = txt;
	addToAnnounceQueue(std::move(cm));
}

void CVCMIServer::announceTxt(const std::string & txt, const std::string & playerName)
{
	logNetwork->info("%s says: %s", playerName, txt);
	auto cm = vstd::make_unique<LobbyChatMessage>();
	cm->playerName = playerName;
	cm->message = txt;
	addToAnnounceQueue(std::move(cm));
}

void CVCMIServer::addToAnnounceQueue(std::unique_ptr<CPackForLobby> pack)
{
	boost::unique_lock<boost::recursive_mutex> queueLock(mx);
	announceQueue.push_back(std::move(pack));
}

bool CVCMIServer::passHost(int toConnectionId)
{
	for(auto c : connections)
	{
		if(isClientHost(c->connectionID))
			continue;
		if(c->connectionID != toConnectionId)
			continue;

		hostClient = c;
		hostClientId = c->connectionID;
		announceTxt(boost::str(boost::format("Pass host to connection %d") % toConnectionId));
		return true;
	}
	return false;
}

void CVCMIServer::clientConnected(std::shared_ptr<CConnection> c, std::vector<std::string> & names, std::string uuid, StartInfo::EMode mode)
{
	c->connectionID = currentClientId++;

	if(!hostClient)
	{
		hostClient = c;
		hostClientId = c->connectionID;
		si->mode = mode;
	}

	logNetwork->info("Connection with client %d established. UUID: %s", c->connectionID, c->uuid);
	for(auto & name : names)
	{
		logNetwork->info("Client %d player: %s", c->connectionID, name);
		ui8 id = currentPlayerId++;

		ClientPlayer cp;
		cp.connection = c->connectionID;
		cp.name = name;
		playerNames.insert(std::make_pair(id, cp));
		announceTxt(boost::str(boost::format("%s (pid %d cid %d) joins the game") % name % id % c->connectionID));

		//put new player in first slot with AI
		for(auto & elem : si->playerInfos)
		{
			if(elem.second.isControlledByAI() && !elem.second.compOnly)
			{
				setPlayerConnectedId(elem.second, id);
				break;
			}
		}
	}
}

void CVCMIServer::clientDisconnected(std::shared_ptr<CConnection> c)
{
	connections -= c;
	for(auto it = playerNames.begin(); it != playerNames.end();)
	{
		if(it->second.connection != c->connectionID)
		{
			it++;
			continue;
		}

		int id = it->first;
		announceTxt(boost::str(boost::format("%s (pid %d cid %d) left the game") % id % playerNames[id].name % c->connectionID));
		playerNames.erase(it++);

		// Reset in-game players client used back to AI
		if(PlayerSettings * s = si->getPlayersSettings(id))
		{
			setPlayerConnectedId(*s, PlayerSettings::PLAYER_AI);
		}
	}
}

void CVCMIServer::setPlayerConnectedId(PlayerSettings & pset, ui8 player) const
{
	if(vstd::contains(playerNames, player))
		pset.name = playerNames.find(player)->second.name;
	else
		pset.name = VLC->generaltexth->allTexts[468]; //Computer

	pset.connectedPlayerIDs.clear();
	if(player != PlayerSettings::PLAYER_AI)
		pset.connectedPlayerIDs.insert(player);
}

void CVCMIServer::updateStartInfoOnMapChange(std::shared_ptr<CMapInfo> mapInfo, std::shared_ptr<CMapGenOptions> mapGenOpts)
{
	mi = mapInfo;
	if(!mi)
		return;

	auto namesIt = playerNames.cbegin();
	si->playerInfos.clear();
	if(mi->scenarioOptionsOfSave)
	{
		si = CMemorySerializer::deepCopy(*mi->scenarioOptionsOfSave);
		si->mode = StartInfo::LOAD_GAME;
		if(si->campState)
			campaignMap = si->campState->currentMap.get();

		for(auto & ps : si->playerInfos)
		{
			if(!ps.second.compOnly && ps.second.connectedPlayerIDs.size() && namesIt != playerNames.cend())
			{
				setPlayerConnectedId(ps.second, namesIt++->first);
			}
			else
			{
				setPlayerConnectedId(ps.second, PlayerSettings::PLAYER_AI);
			}
		}
	}
	else if(si->mode == StartInfo::NEW_GAME || si->mode == StartInfo::CAMPAIGN)
	{
		if(mi->campaignHeader)
			return;

		for(int i = 0; i < mi->mapHeader->players.size(); i++)
		{
			const PlayerInfo & pinfo = mi->mapHeader->players[i];

			//neither computer nor human can play - no player
			if(!(pinfo.canHumanPlay || pinfo.canComputerPlay))
				continue;

			PlayerSettings & pset = si->playerInfos[PlayerColor(i)];
			pset.color = PlayerColor(i);
			if(pinfo.canHumanPlay && namesIt != playerNames.cend())
			{
				setPlayerConnectedId(pset, namesIt++->first);
			}
			else
			{
				setPlayerConnectedId(pset, PlayerSettings::PLAYER_AI);
				if(!pinfo.canHumanPlay)
				{
					pset.compOnly = true;
				}
			}

			pset.castle = pinfo.defaultCastle();
			pset.hero = pinfo.defaultHero();

			if(pset.hero != PlayerSettings::RANDOM && pinfo.hasCustomMainHero())
			{
				pset.hero = pinfo.mainCustomHeroId;
				pset.heroName = pinfo.mainCustomHeroName;
				pset.heroPortrait = pinfo.mainCustomHeroPortrait;
			}

			pset.handicap = PlayerSettings::NO_HANDICAP;
		}

		if(mi->isRandomMap && mapGenOpts)
			si->mapGenOptions = std::shared_ptr<CMapGenOptions>(mapGenOpts);
		else
			si->mapGenOptions.reset();
	}
	si->mapname = mi->fileURI;
}

void CVCMIServer::updateAndPropagateLobbyState()
{
	boost::unique_lock<boost::mutex> stateLock(stateMutex);
	// Update player settings for RMG
	// TODO: find appropriate location for this code
	if(si->mapGenOptions && si->mode == StartInfo::NEW_GAME)
	{
		for(const auto & psetPair : si->playerInfos)
		{
			const auto & pset = psetPair.second;
			si->mapGenOptions->setStartingTownForPlayer(pset.color, pset.castle);
			if(pset.isControlledByHuman())
			{
				si->mapGenOptions->setPlayerTypeForStandardPlayer(pset.color, EPlayerType::HUMAN);
			}
		}
	}

	auto lus = vstd::make_unique<LobbyUpdateState>();
	lus->state = *this;
	addToAnnounceQueue(std::move(lus));
}

void CVCMIServer::setPlayer(PlayerColor clickedColor)
{
	struct PlayerToRestore
	{
		PlayerColor color;
		int id;
		void reset() { id = -1; color = PlayerColor::CANNOT_DETERMINE; }
		PlayerToRestore(){ reset(); }
	} playerToRestore;


	PlayerSettings & clicked = si->playerInfos[clickedColor];

	//identify clicked player
	int clickedNameID = 0; //number of player - zero means AI, assume it initially
	if(clicked.isControlledByHuman())
		clickedNameID = *(clicked.connectedPlayerIDs.begin()); //if not AI - set appropiate ID

	if(clickedNameID > 0 && playerToRestore.id == clickedNameID) //player to restore is about to being replaced -> put him back to the old place
	{
		PlayerSettings & restPos = si->playerInfos[playerToRestore.color];
		setPlayerConnectedId(restPos, playerToRestore.id);
		playerToRestore.reset();
	}

	int newPlayer; //which player will take clicked position

	//who will be put here?
	if(!clickedNameID) //AI player clicked -> if possible replace computer with unallocated player
	{
		newPlayer = getIdOfFirstUnallocatedPlayer();
		if(!newPlayer) //no "free" player -> get just first one
			newPlayer = playerNames.begin()->first;
	}
	else //human clicked -> take next
	{
		auto i = playerNames.find(clickedNameID); //clicked one
		i++; //player AFTER clicked one

		if(i != playerNames.end())
			newPlayer = i->first;
		else
			newPlayer = 0; //AI if we scrolled through all players
	}

	setPlayerConnectedId(clicked, newPlayer); //put player

	//if that player was somewhere else, we need to replace him with computer
	if(newPlayer) //not AI
	{
		for(auto i = si->playerInfos.begin(); i != si->playerInfos.end(); i++)
		{
			int curNameID = *(i->second.connectedPlayerIDs.begin());
			if(i->first != clickedColor && curNameID == newPlayer)
			{
				assert(i->second.connectedPlayerIDs.size());
				playerToRestore.color = i->first;
				playerToRestore.id = newPlayer;
				setPlayerConnectedId(i->second, PlayerSettings::PLAYER_AI); //set computer
				break;
			}
		}
	}
}

void CVCMIServer::optionNextCastle(PlayerColor player, int dir)
{
	PlayerSettings & s = si->playerInfos[player];
	si16 & cur = s.castle;
	auto & allowed = getPlayerInfo(player.getNum()).allowedFactions;
	const bool allowRandomTown = getPlayerInfo(player.getNum()).isFactionRandom;

	if(cur == PlayerSettings::NONE) //no change
		return;

	if(cur == PlayerSettings::RANDOM) //first/last available
	{
		if(dir > 0)
			cur = *allowed.begin(); //id of first town
		else
			cur = *allowed.rbegin(); //id of last town

	}
	else // next/previous available
	{
		if((cur == *allowed.begin() && dir < 0) || (cur == *allowed.rbegin() && dir > 0))
		{
			if(allowRandomTown)
			{
				cur = PlayerSettings::RANDOM;
			}
			else
			{
				if(dir > 0)
					cur = *allowed.begin();
				else
					cur = *allowed.rbegin();
			}
		}
		else
		{
			assert(dir >= -1 && dir <= 1); //othervice std::advance may go out of range
			auto iter = allowed.find((ui8)cur);
			std::advance(iter, dir);
			cur = *iter;
		}
	}

	if(s.hero >= 0 && !getPlayerInfo(player.getNum()).hasCustomMainHero()) // remove hero unless it set to fixed one in map editor
	{
		s.hero = PlayerSettings::RANDOM;
	}
	if(cur < 0 && s.bonus == PlayerSettings::RESOURCE)
		s.bonus = PlayerSettings::RANDOM;
}

void CVCMIServer::setCampaignMap(int mapId)
{
	campaignMap = mapId;
	si->difficulty = si->campState->camp->scenarios[mapId].difficulty;
	campaignBonus = -1;
	updateStartInfoOnMapChange(si->campState->getMapInfo(mapId));
}

void CVCMIServer::setCampaignBonus(int bonusId)
{
	campaignBonus = bonusId;

	const CCampaignScenario & scenario = si->campState->camp->scenarios[campaignMap];
	const std::vector<CScenarioTravel::STravelBonus> & bonDescs = scenario.travelOptions.bonusesToChoose;
	if(bonDescs[bonusId].type == CScenarioTravel::STravelBonus::HERO)
	{
		for(auto & elem : si->playerInfos)
		{
			if(elem.first == PlayerColor(bonDescs[bonusId].info1))
				setPlayerConnectedId(elem.second, 1);
			else
				setPlayerConnectedId(elem.second, PlayerSettings::PLAYER_AI);
		}
	}
}

void CVCMIServer::optionNextHero(PlayerColor player, int dir)
{
	PlayerSettings & s = si->playerInfos[player];
	if(s.castle < 0 || s.hero == PlayerSettings::NONE)
		return;

	if(s.hero == PlayerSettings::RANDOM) // first/last available
	{
		int max = static_cast<int>(VLC->heroh->size()),
			min = 0;
		s.hero = nextAllowedHero(player, min, max, 0, dir);
	}
	else
	{
		if(dir > 0)
			s.hero = nextAllowedHero(player, s.hero, (int)VLC->heroh->size(), 1, dir);
		else
			s.hero = nextAllowedHero(player, -1, s.hero, 1, dir); // min needs to be -1 -- hero at index 0 would be skipped otherwise
	}
}

int CVCMIServer::nextAllowedHero(PlayerColor player, int min, int max, int incl, int dir)
{
	if(dir > 0)
	{
		for(int i = min + incl; i <= max - incl; i++)
			if(canUseThisHero(player, i))
				return i;
	}
	else
	{
		for(int i = max - incl; i >= min + incl; i--)
			if(canUseThisHero(player, i))
				return i;
	}
	return -1;
}

void CVCMIServer::optionNextBonus(PlayerColor player, int dir)
{
	PlayerSettings & s = si->playerInfos[player];
	PlayerSettings::Ebonus & ret = s.bonus = static_cast<PlayerSettings::Ebonus>(static_cast<int>(s.bonus) + dir);

	if(s.hero == PlayerSettings::NONE &&
		!getPlayerInfo(player.getNum()).heroesNames.size() &&
		ret == PlayerSettings::ARTIFACT) //no hero - can't be artifact
	{
		if(dir < 0)
			ret = PlayerSettings::RANDOM;
		else
			ret = PlayerSettings::GOLD;
	}

	if(ret > PlayerSettings::RESOURCE)
		ret = PlayerSettings::RANDOM;
	if(ret < PlayerSettings::RANDOM)
		ret = PlayerSettings::RESOURCE;

	if(s.castle == PlayerSettings::RANDOM && ret == PlayerSettings::RESOURCE) //random castle - can't be resource
	{
		if(dir < 0)
			ret = PlayerSettings::GOLD;
		else
			ret = PlayerSettings::RANDOM;
	}
}

bool CVCMIServer::canUseThisHero(PlayerColor player, int ID)
{
	return VLC->heroh->size() > ID
		&& si->playerInfos[player].castle == VLC->heroh->objects[ID]->heroClass->faction
		&& !vstd::contains(getUsedHeroes(), ID)
		&& mi->mapHeader->allowedHeroes[ID];
}

std::vector<int> CVCMIServer::getUsedHeroes()
{
	std::vector<int> heroIds;
	for(auto & p : si->playerInfos)
	{
		const auto & heroes = getPlayerInfo(p.first.getNum()).heroesNames;
		for(auto & hero : heroes)
			if(hero.heroId >= 0) //in VCMI map format heroId = -1 means random hero
				heroIds.push_back(hero.heroId);

		if(p.second.hero != PlayerSettings::RANDOM)
			heroIds.push_back(p.second.hero);
	}
	return heroIds;
}

ui8 CVCMIServer::getIdOfFirstUnallocatedPlayer() const
{
	for(auto i = playerNames.cbegin(); i != playerNames.cend(); i++)
	{
		if(!si->getPlayersSettings(i->first))
			return i->first;
	}

	return 0;
}

#if defined(__GNUC__) && !defined(__MINGW32__) && !defined(VCMI_ANDROID) && !defined(VCMI_IOS)
void handleLinuxSignal(int sig)
{
	const int STACKTRACE_SIZE = 100;
	void * buffer[STACKTRACE_SIZE];
	int ptrCount = backtrace(buffer, STACKTRACE_SIZE);
	char * * strings;

	logGlobal->error("Error: signal %d :", sig);
	strings = backtrace_symbols(buffer, ptrCount);
	if(strings == nullptr)
	{
		logGlobal->error("There are no symbols.");
	}
	else
	{
		for(int i = 0; i < ptrCount; ++i)
		{
			logGlobal->error(strings[i]);
		}
		free(strings);
	}

	_exit(EXIT_FAILURE);
}
#endif

static void handleCommandOptions(int argc, char * argv[], boost::program_options::variables_map & options)
{
	namespace po = boost::program_options;
#ifdef SINGLE_PROCESS_APP
	options.emplace("run-by-client", po::variable_value{true, true});
	options.emplace("uuid", po::variable_value{std::string{argv[1]}, true});
#else
	po::options_description opts("Allowed options");
	opts.add_options()
	("help,h", "display help and exit")
	("version,v", "display version information and exit")
	("run-by-client", "indicate that server launched by client on same machine")
	("uuid", po::value<std::string>(), "")
	("enable-shm-uuid", "use UUID for shared memory identifier")
	("enable-shm", "enable usage of shared memory")
	("port", po::value<ui16>(), "port at which server will listen to connections from client");

	if(argc > 1)
	{
		try
		{
			po::store(po::parse_command_line(argc, argv, opts), options);
		}
		catch(std::exception & e)
		{
			std::cerr << "Failure during parsing command-line options:\n" << e.what() << std::endl;
		}
	}
#endif

	po::notify(options);

#ifndef SINGLE_PROCESS_APP
	if(options.count("help"))
	{
		auto time = std::time(0);
		printf("%s - A Heroes of Might and Magic 3 clone\n", GameConstants::VCMI_VERSION.c_str());
		printf("Copyright (C) 2007-%d VCMI dev team - see AUTHORS file\n", std::localtime(&time)->tm_year + 1900);
		printf("This is free software; see the source for copying conditions. There is NO\n");
		printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");
		printf("\n");
		std::cout << opts;
		exit(0);
	}

	if(options.count("version"))
	{
		printf("%s\n", GameConstants::VCMI_VERSION.c_str());
		std::cout << VCMIDirs::get().genHelpString();
		exit(0);
	}
#endif
}

#ifdef SINGLE_PROCESS_APP
#define main server_main
#endif
int main(int argc, char * argv[])
{
#if !defined(VCMI_ANDROID) && !defined(VCMI_IOS)
	// Correct working dir executable folder (not bundle folder) so we can use executable relative paths
	boost::filesystem::current_path(boost::filesystem::system_complete(argv[0]).parent_path());
#endif
	// Installs a sig sev segmentation violation handler
	// to log stacktrace
#if defined(__GNUC__) && !defined(__MINGW32__) && !defined(VCMI_ANDROID) && !defined(VCMI_IOS)
	signal(SIGSEGV, handleLinuxSignal);
#endif

#ifndef VCMI_IOS
	console = new CConsoleHandler();
#endif
	CBasicLogConfigurator logConfig(VCMIDirs::get().userLogsPath() / "VCMI_Server_log.txt", console);
	logConfig.configureDefault();
	logGlobal->info(SERVER_NAME);

	boost::program_options::variables_map opts;
	handleCommandOptions(argc, argv, opts);
	preinitDLL(console);
	settings.init();
	logConfig.configure();

	loadDLLClasses();
	srand((ui32)time(nullptr));

#ifdef SINGLE_PROCESS_APP
	boost::condition_variable * cond = reinterpret_cast<boost::condition_variable *>(argv[0]);
	cond->notify_one();
#endif

	try
	{
		boost::asio::io_service io_service;
		CVCMIServer server(opts);

		try
		{
			while(server.state != EServerState::SHUTDOWN)
			{
				server.run();
			}
			io_service.run();
		}
		catch(boost::system::system_error & e) //for boost errors just log, not crash - probably client shut down connection
		{
			logNetwork->error(e.what());
			server.state = EServerState::SHUTDOWN;
		}
		catch(...)
		{
			handleException();
		}
	}
	catch(boost::system::system_error & e)
	{
		logNetwork->error(e.what());
		//catch any startup errors (e.g. can't access port) errors
		//and return non-zero status so client can detect error
		throw;
	}
#ifdef VCMI_ANDROID
	CAndroidVMHelper envHelper;
	envHelper.callStaticVoidMethod(CAndroidVMHelper::NATIVE_METHODS_DEFAULT_CLASS, "killServer");
#endif
	logConfig.deconfigure();
	vstd::clear_pointer(VLC);
	return 0;
}

#ifdef VCMI_ANDROID
void CVCMIServer::create()
{
	const char * foo[1] = {"android-server"};

	main(1, const_cast<char **>(foo));
}
#elif defined(SINGLE_PROCESS_APP)
void CVCMIServer::create(boost::condition_variable * cond, const std::string & uuid)
{
	const std::initializer_list<const void *> argv = {
		cond,
		uuid.c_str(),
	};
	main(argv.size(), reinterpret_cast<char **>(const_cast<void **>(argv.begin())));
}
#endif
