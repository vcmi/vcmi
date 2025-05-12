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
#include "CVCMIServer.h"

#include "CGameHandler.h"
#include "GlobalLobbyProcessor.h"
#include "LobbyNetPackVisitors.h"
#include "processors/PlayerMessageProcessor.h"

#include "../lib/CPlayerState.h"
#include "../lib/CThreadHelper.h"
#include "../lib/campaign/CampaignState.h"
#include "../lib/entities/hero/CHeroHandler.h"
#include "../lib/entities/hero/CHeroClass.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/mapping/CMapDefines.h"
#include "../lib/mapping/CMapInfo.h"
#include "../lib/mapping/CMapHeader.h"
#include "../lib/rmg/CMapGenOptions.h"
#include "../lib/serializer/CMemorySerializer.h"
#include "../lib/serializer/Connection.h"
#include "../lib/texts/CGeneralTextHandler.h"

// UUID generation
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/program_options.hpp>

class CVCMIServerPackVisitor : public VCMI_LIB_WRAP_NAMESPACE(ICPackVisitor)
{
private:
	CVCMIServer & handler;
	std::shared_ptr<CGameHandler> gh;
	std::shared_ptr<CConnection> connection;

public:
	CVCMIServerPackVisitor(CVCMIServer & handler, const std::shared_ptr<CGameHandler> & gh, const std::shared_ptr<CConnection> & connection)
		: handler(handler)
		, gh(gh)
		, connection(connection)
	{
	}

	bool callTyped() override { return false; }

	void visitForLobby(CPackForLobby & packForLobby) override
	{
		handler.handleReceivedPack(connection, packForLobby);
	}

	void visitForServer(CPackForServer & serverPack) override
	{
		if (gh)
			gh->handleReceivedPack(connection, serverPack);
		else
			logNetwork->error("Received pack for game server while in lobby!");
	}

	void visitForClient(CPackForClient & clientPack) override
	{
	}
};

CVCMIServer::CVCMIServer(uint16_t port, bool runByClient)
	: currentClientId(1)
	, currentPlayerId(1)
	, port(port)
	, runByClient(runByClient)
{
	uuid = boost::uuids::to_string(boost::uuids::random_generator()());
	logNetwork->trace("CVCMIServer created! UUID: %s", uuid);

	networkHandler = INetworkHandler::createHandler();
}

CVCMIServer::~CVCMIServer() = default;

uint16_t CVCMIServer::prepare(bool connectToLobby, bool listenForConnections) {
	if(connectToLobby) {
		lobbyProcessor = std::make_unique<GlobalLobbyProcessor>(*this);
		return 0;
	} else {
		return startAcceptingIncomingConnections(listenForConnections);
	}
}

uint16_t CVCMIServer::startAcceptingIncomingConnections(bool listenForConnections)
{
	networkServer = networkHandler->createServerTCP(*this);

	port
		? logNetwork->info("Port %d will be used", port)
		: logNetwork->info("Randomly assigned port will be used");

	// config port may be 0 => srvport will contain the OS-assigned port value

	if (listenForConnections)
	{
		auto srvport = networkServer->start(port);
		logNetwork->info("Listening for connections at port %d", srvport);
		return srvport;
	}
	else
		return 0;
}

void CVCMIServer::onNewConnection(const std::shared_ptr<INetworkConnection> & connection)
{
	if(getState() == EServerState::LOBBY)
	{
		activeConnections.push_back(std::make_shared<CConnection>(connection));
		activeConnections.back()->enterLobbyConnectionMode();
	}
	else
	{
		// TODO: reconnection support
		connection->close();
	}
}

void CVCMIServer::onPacketReceived(const std::shared_ptr<INetworkConnection> & connection, const std::vector<std::byte> & message)
{
	std::shared_ptr<CConnection> c = findConnection(connection);
	if (c == nullptr)
		throw std::out_of_range("Unknown connection received in CVCMIServer::findConnection");

	auto pack = c->retrievePack(message);
	CVCMIServerPackVisitor visitor(*this, this->gh, c);
	pack->visit(visitor);
}

void CVCMIServer::setState(EServerState value)
{
	if (value == EServerState::SHUTDOWN && state == EServerState::SHUTDOWN)
		logGlobal->warn("Attempt to shutdown already shutdown server!");

	// do not attempt to restart dying server
	assert(state != EServerState::SHUTDOWN || state == value);
	state = value;

	if (state == EServerState::SHUTDOWN)
		networkHandler->stop();
}

EServerState CVCMIServer::getState() const
{
	return state;
}

std::shared_ptr<CConnection> CVCMIServer::findConnection(const std::shared_ptr<INetworkConnection> & netConnection)
{
	for(const auto & gameConnection : activeConnections)
	{
		if (gameConnection->isMyConnection(netConnection))
			return gameConnection;
	}

	return nullptr;
}

bool CVCMIServer::wasStartedByClient() const
{
	return runByClient;
}

void CVCMIServer::run()
{
	networkHandler->run();
}

void CVCMIServer::onTimer()
{
	// we might receive onTimer call after transitioning from GAMEPLAY to LOBBY state, e.g. on game restart
	if (getState() != EServerState::GAMEPLAY)
		return;

	static const auto serverUpdateInterval = std::chrono::milliseconds(100);

	auto timeNow = std::chrono::steady_clock::now();
	auto timePassedBefore = lastTimerUpdateTime - gameplayStartTime;
	auto timePassedNow = timeNow - gameplayStartTime;

	lastTimerUpdateTime = timeNow;

	auto msPassedBefore = std::chrono::duration_cast<std::chrono::milliseconds>(timePassedBefore);
	auto msPassedNow = std::chrono::duration_cast<std::chrono::milliseconds>(timePassedNow);
	auto msDelta = msPassedNow - msPassedBefore;

	if (msDelta.count())
		gh->tick(msDelta.count());
	networkHandler->createTimer(*this, serverUpdateInterval);
}

void CVCMIServer::prepareToRestart()
{
	if(getState() != EServerState::GAMEPLAY)
	{
		assert(0);
		return;
	}

	* si = * gh->gs->getInitialStartInfo();
	setState(EServerState::LOBBY);
	if (si->campState)
	{
		assert(si->campState->currentScenario().has_value());
		campaignMap = si->campState->currentScenario().value_or(CampaignScenarioID(0));
		campaignBonus = si->campState->getBonusID(campaignMap).value_or(-1);
	}

	for(auto activeConnection : activeConnections)
		activeConnection->enterLobbyConnectionMode();

	gh = nullptr;
}

bool CVCMIServer::prepareToStartGame()
{
	Load::ProgressAccumulator progressTracking;
	Load::Progress current(1);
	progressTracking.include(current);

	if (lobbyProcessor)
		lobbyProcessor->sendGameStarted();

	auto progressTrackingThread = std::thread([this, &progressTracking]()
	{
		setThreadName("progressTrackingThread");
		auto currentProgress = std::numeric_limits<Load::Type>::max();

		while(!progressTracking.finished())
		{
			if(progressTracking.get() != currentProgress)
			{
				//FIXME: UNGUARDED MULTITHREADED ACCESS!!!
				currentProgress = progressTracking.get();
				LobbyLoadProgress loadProgress;
				loadProgress.progress = currentProgress;
				announcePack(loadProgress);
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(50));
		}
	});

	gh = std::make_shared<CGameHandler>(this);
	switch(si->mode)
	{
	case EStartMode::CAMPAIGN:
		logNetwork->info("Preparing to start new campaign");
		si->startTime = std::time(nullptr);
		si->fileURI = mi->fileURI;
		si->campState->setCurrentMap(campaignMap);
		si->campState->setCurrentMapBonus(campaignBonus);
		gh->init(si.get(), progressTracking);
		break;

	case EStartMode::NEW_GAME:
		logNetwork->info("Preparing to start new game");
		si->startTime = std::time(nullptr);
		si->fileURI = mi->fileURI;
		gh->init(si.get(), progressTracking);
		break;

	case EStartMode::LOAD_GAME:
		logNetwork->info("Preparing to start loaded game");
		if(!gh->load(si->mapname))
		{
			current.finish();
			progressTrackingThread.join();
			return false;
		}
		break;
	default:
		logNetwork->error("Wrong mode in StartInfo!");
		assert(0);
		break;
	}

	current.finish();
	progressTrackingThread.join();

	return true;
}

void CVCMIServer::startGameImmediately()
{
	for(auto activeConnection : activeConnections)
		activeConnection->enterGameplayConnectionMode(*gh->gs);

	gh->start(si->mode == EStartMode::LOAD_GAME);
	setState(EServerState::GAMEPLAY);
	lastTimerUpdateTime = gameplayStartTime = std::chrono::steady_clock::now();
	onTimer();

	multiplayerWelcomeMessage();
}

void CVCMIServer::onDisconnected(const std::shared_ptr<INetworkConnection> & connection, const std::string & errorMessage)
{
	logNetwork->error("Network error receiving a pack. Connection has been closed");

	std::shared_ptr<CConnection> c = findConnection(connection);

	// player may have already disconnected via clientDisconnected call
	if (c)
	{
		LobbyClientDisconnected lcd;
		lcd.clientId = c->connectionID;
		handleReceivedPack(c, lcd);
	}
}

void CVCMIServer::handleReceivedPack(std::shared_ptr<CConnection> connection, CPackForLobby & pack)
{
	ClientPermissionsCheckerNetPackVisitor checker(*this, connection);
	pack.visit(checker);

	if(checker.getResult())
	{
		ApplyOnServerNetPackVisitor applier(*this, connection);
		pack.visit(applier);
		if (applier.getResult())
			announcePack(pack);
	}
}

void CVCMIServer::announcePack(CPackForLobby & pack)
{
	for(auto activeConnection : activeConnections)
	{
		// FIXME: we need to avoid sending something to client that not yet get answer for LobbyClientConnected
		// Until UUID set we only pass LobbyClientConnected to this client
		//if(c->uuid == uuid && !dynamic_cast<LobbyClientConnected *>(pack.get()))
		//	continue;
		activeConnection->sendPack(pack);
	}

	ApplyOnServerAfterAnnounceNetPackVisitor applier(*this);
	pack.visit(applier);
}

void CVCMIServer::announceMessage(const MetaString & txt)
{
	logNetwork->info("Show message: %s", txt.toString());
	LobbyShowMessage cm;
	cm.message = txt;
	announcePack(cm);
}

void CVCMIServer::announceMessage(const std::string & txt)
{
	MetaString str;
	str.appendRawString(txt);
	announceMessage(str);
}

void CVCMIServer::announceTxt(const MetaString & txt, const std::string & playerName)
{
	logNetwork->info("%s says: %s", playerName, txt.toString());
	LobbyChatMessage cm;
	cm.playerName = playerName;
	cm.message = txt;
	announcePack(cm);
}

void CVCMIServer::announceTxt(const std::string & txt, const std::string & playerName)
{
	MetaString str;
	str.appendRawString(txt);
	announceTxt(str, playerName);
}

bool CVCMIServer::passHost(int toConnectionId)
{
	for(auto activeConnection : activeConnections)
	{
		if(isClientHost(activeConnection->connectionID))
			continue;
		if(activeConnection->connectionID != toConnectionId)
			continue;

		hostClientId = activeConnection->connectionID;
		announceTxt(boost::str(boost::format("Pass host to connection %d") % toConnectionId));
		return true;
	}
	return false;
}

void CVCMIServer::clientConnected(std::shared_ptr<CConnection> c, std::vector<std::string> & names, const std::string & uuid, EStartMode mode)
{
	assert(getState() == EServerState::LOBBY);

	c->connectionID = currentClientId++;
	c->uuid = uuid;

	if(hostClientId == -1)
	{
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

void CVCMIServer::clientDisconnected(std::shared_ptr<CConnection> connection)
{
	assert(vstd::contains(activeConnections, connection));
	logGlobal->trace("Received disconnection request");
	vstd::erase(activeConnections, connection);

	if(activeConnections.empty() || hostClientId == connection->connectionID)
	{
		setState(EServerState::SHUTDOWN);
		return;
	}

	if(gh && getState() == EServerState::GAMEPLAY)
	{
		gh->handleClientDisconnection(connection);
	}
}

void CVCMIServer::reconnectPlayer(int connId)
{
	PlayerReinitInterface startAiPack;
	startAiPack.playerConnectionId = connId;

	if(gh && si && getState() == EServerState::GAMEPLAY)
	{
		for(auto it = playerNames.begin(); it != playerNames.end(); ++it)
		{
			if(it->second.connection != connId)
				continue;

			int id = it->first;
			auto * playerSettings = si->getPlayersSettings(id);
			if(!playerSettings)
				continue;

			std::string messageText = boost::str(boost::format("%s (cid %d) is connected") % playerSettings->name % connId);
			gh->playerMessages->broadcastMessage(playerSettings->color, messageText);

			startAiPack.players.push_back(playerSettings->color);
		}

		if(!startAiPack.players.empty())
			gh->sendAndApply(startAiPack);
	}
}

void CVCMIServer::setPlayerConnectedId(PlayerSettings & pset, ui8 player) const
{
	if(vstd::contains(playerNames, player))
		pset.name = playerNames.find(player)->second.name;
	else
		pset.name = LIBRARY->generaltexth->allTexts[468]; //Computer

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
		si->mode = EStartMode::LOAD_GAME;
		if(si->campState)
			campaignMap = si->campState->currentScenario().value();

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
	else if(si->mode == EStartMode::NEW_GAME || si->mode == EStartMode::CAMPAIGN)
	{
		if(mi->campaign)
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

			if(pset.hero != HeroTypeID::RANDOM && pinfo.hasCustomMainHero())
			{
				pset.hero = pinfo.mainCustomHeroId;
				pset.heroNameTextId = pinfo.mainCustomHeroNameTextId;
				pset.heroPortrait = pinfo.mainCustomHeroPortrait;
			}
		}

		if(mi->isRandomMap && mapGenOpts)
			si->mapGenOptions = std::shared_ptr<CMapGenOptions>(mapGenOpts);
		else
			si->mapGenOptions.reset();
	}

	if (lobbyProcessor)
	{
		std::string roomDescription;

		if (si->mapGenOptions)
		{
			if (si->mapGenOptions->getMapTemplate())
				roomDescription = si->mapGenOptions->getMapTemplate()->getName();
			// else - no template selected.
			// TODO: handle this somehow?
		}
		else
			roomDescription = mi->getNameTranslated();

		lobbyProcessor->sendChangeRoomDescription(roomDescription);
	}

	si->mapname = mi->fileURI;
}

void CVCMIServer::updateAndPropagateLobbyState()
{
	// Update player settings for RMG
	// TODO: find appropriate location for this code
	if(si->mapGenOptions && si->mode == EStartMode::NEW_GAME)
	{
		for(const auto & psetPair : si->playerInfos)
		{
			const auto & pset = psetPair.second;
			si->mapGenOptions->setStartingTownForPlayer(pset.color, pset.castle);
			si->mapGenOptions->setStartingHeroForPlayer(pset.color, pset.hero);
			if(pset.isControlledByHuman())
			{
				si->mapGenOptions->setPlayerTypeForStandardPlayer(pset.color, EPlayerType::HUMAN);
			}
			else
			{
				si->mapGenOptions->setPlayerTypeForStandardPlayer(pset.color, EPlayerType::AI);
			}
		}
	}

	LobbyUpdateState lus;
	lus.state = *static_cast<LobbyState*>(this);
	announcePack(lus);
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
		clickedNameID = *(clicked.connectedPlayerIDs.begin()); //if not AI - set appropriate ID

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

void CVCMIServer::setPlayerName(PlayerColor color, std::string name)
{
	if(color == PlayerColor::CANNOT_DETERMINE)
		return;

	PlayerSettings & player = si->playerInfos.at(color);

	if(!player.isControlledByHuman())
		return;

	if(player.connectedPlayerIDs.empty())
		return;

	int nameID = *(player.connectedPlayerIDs.begin()); //if not AI - set appropriate ID

	playerNames[nameID].name = name;
	setPlayerConnectedId(player, nameID);
}

void CVCMIServer::setPlayerHandicap(PlayerColor color, Handicap handicap)
{
	if(color == PlayerColor::CANNOT_DETERMINE)
		return;

	si->playerInfos[color].handicap = handicap;

	int humanPlayer = 0;
	for (const auto & pi : si->playerInfos)
		if(pi.second.isControlledByHuman())
			humanPlayer++;

	if(humanPlayer < 2) // Singleplayer
		return;

	MetaString str;
	str.appendTextID("vcmi.lobby.handicap");
	str.appendRawString(" ");
	str.appendName(color);
	str.appendRawString(":");

	if(handicap.startBonus.empty() && handicap.percentIncome == 100 && handicap.percentGrowth == 100)
	{
		str.appendRawString(" ");
		str.appendTextID("core.genrltxt.523");
		announceTxt(str);
		return;
	}

	for(auto & res : EGameResID::ALL_RESOURCES())
		if(handicap.startBonus[res] != 0)
		{
			str.appendRawString(" ");
			str.appendName(res);
			str.appendRawString(":");
			str.appendRawString(std::to_string(handicap.startBonus[res]));
		}
	if(handicap.percentIncome != 100)
	{
		str.appendRawString(" ");
		str.appendTextID("core.jktext.32");
		str.appendRawString(":");
		str.appendRawString(std::to_string(handicap.percentIncome) + "%");
	}
	if(handicap.percentGrowth != 100)
	{
		str.appendRawString(" ");
		str.appendTextID("core.genrltxt.194");
		str.appendRawString(":");
		str.appendRawString(std::to_string(handicap.percentGrowth) + "%");
	}
	announceTxt(str);
}

void CVCMIServer::optionNextCastle(PlayerColor player, int dir)
{
	PlayerSettings & s = si->playerInfos[player];
	FactionID & cur = s.castle;
	auto & allowed = getPlayerInfo(player).allowedFactions;
	const bool allowRandomTown = getPlayerInfo(player).isFactionRandom;

	if(cur == FactionID::NONE) //no change
		return;

	if(cur == FactionID::RANDOM) //first/last available
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
				cur = FactionID::RANDOM;
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
			auto iter = allowed.find(cur);
			std::advance(iter, dir);
			cur = *iter;
		}
	}

	if(s.hero.isValid() && !getPlayerInfo(player).hasCustomMainHero()) // remove hero unless it set to fixed one in map editor
	{
		s.hero = HeroTypeID::RANDOM;
	}
	if(!cur.isValid() && s.bonus == PlayerStartingBonus::RESOURCE)
		s.bonus = PlayerStartingBonus::RANDOM;
}

void CVCMIServer::optionSetCastle(PlayerColor player, FactionID id)
{
	PlayerSettings & s = si->playerInfos[player];
	FactionID & cur = s.castle;
	auto & allowed = getPlayerInfo(player).allowedFactions;

	if(cur == FactionID::NONE) //no change
		return;

	if(allowed.find(id) == allowed.end() && id != FactionID::RANDOM) // valid id
		return;

	cur = static_cast<FactionID>(id);

	if(s.hero.isValid() && !getPlayerInfo(player).hasCustomMainHero()) // remove hero unless it set to fixed one in map editor
	{
		s.hero = HeroTypeID::RANDOM;
	}
	if(!cur.isValid() && s.bonus == PlayerStartingBonus::RESOURCE)
		s.bonus = PlayerStartingBonus::RANDOM;
}

void CVCMIServer::setCampaignMap(CampaignScenarioID mapId)
{
	campaignMap = mapId;
	si->difficulty = si->campState->scenario(mapId).difficulty;
	campaignBonus = -1;
	updateStartInfoOnMapChange(si->campState->getMapInfo(mapId));
}

void CVCMIServer::setCampaignBonus(int bonusId)
{
	campaignBonus = bonusId;

	const CampaignScenario & scenario = si->campState->scenario(campaignMap);
	const std::vector<CampaignBonus> & bonDescs = scenario.travelOptions.bonusesToChoose;
	if(bonDescs[bonusId].type == CampaignBonusType::HERO || bonDescs[bonusId].type == CampaignBonusType::HEROES_FROM_PREVIOUS_SCENARIO)
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
	if(!s.castle.isValid() || s.hero == HeroTypeID::NONE)
		return;

	if(s.hero == HeroTypeID::RANDOM) // first/last available
	{
		if (dir > 0)
			s.hero = nextAllowedHero(player, HeroTypeID(-1), dir);
		else
			s.hero = nextAllowedHero(player, HeroTypeID(LIBRARY->heroh->size()), dir);
	}
	else
	{
		s.hero = nextAllowedHero(player, s.hero, dir);
	}
}

void CVCMIServer::optionSetHero(PlayerColor player, HeroTypeID id)
{
	PlayerSettings & s = si->playerInfos[player];
	if(!s.castle.isValid() || s.hero == HeroTypeID::NONE)
		return;

	if(id == HeroTypeID::RANDOM)
	{
		s.hero = HeroTypeID::RANDOM;
	}
	if(canUseThisHero(player, id))
		s.hero = static_cast<HeroTypeID>(id);
}

HeroTypeID CVCMIServer::nextAllowedHero(PlayerColor player, HeroTypeID initial, int direction)
{
	HeroTypeID first(initial.getNum() + direction);

	if(direction > 0)
	{
		for (auto i = first; i.getNum() < LIBRARY->heroh->size(); ++i)
			if(canUseThisHero(player, i))
				return i;
	}
	else
	{
		for (auto i = first; i.getNum() >= 0; --i)
			if(canUseThisHero(player, i))
				return i;
	}
	return HeroTypeID::RANDOM;
}

void CVCMIServer::optionNextBonus(PlayerColor player, int dir)
{
	PlayerSettings & s = si->playerInfos[player];
	PlayerStartingBonus & ret = s.bonus = static_cast<PlayerStartingBonus>(static_cast<int>(s.bonus) + dir);

	if(s.hero == HeroTypeID::NONE &&
		!getPlayerInfo(player).heroesNames.size() &&
		ret == PlayerStartingBonus::ARTIFACT) //no hero - can't be artifact
	{
		if(dir < 0)
			ret = PlayerStartingBonus::RANDOM;
		else
			ret = PlayerStartingBonus::GOLD;
	}

	if(ret > PlayerStartingBonus::RESOURCE)
		ret = PlayerStartingBonus::RANDOM;
	if(ret < PlayerStartingBonus::RANDOM)
		ret = PlayerStartingBonus::RESOURCE;

	if(s.castle == FactionID::RANDOM && ret == PlayerStartingBonus::RESOURCE) //random castle - can't be resource
	{
		if(dir < 0)
			ret = PlayerStartingBonus::GOLD;
		else
			ret = PlayerStartingBonus::RANDOM;
	}
}

void CVCMIServer::optionSetBonus(PlayerColor player, PlayerStartingBonus id)
{
	PlayerSettings & s = si->playerInfos[player];

	if(s.hero == HeroTypeID::NONE &&
		!getPlayerInfo(player).heroesNames.size() &&
		id == PlayerStartingBonus::ARTIFACT) //no hero - can't be artifact
			return;

	if(id > PlayerStartingBonus::RESOURCE)
		return;
	if(id < PlayerStartingBonus::RANDOM)
		return;

	if(s.castle == FactionID::RANDOM && id == PlayerStartingBonus::RESOURCE) //random castle - can't be resource
		return;

	s.bonus = id;
}

bool CVCMIServer::canUseThisHero(PlayerColor player, HeroTypeID ID)
{
	if (!ID.hasValue())
		return false;

	if (ID.getNum() >= LIBRARY->heroh->size())
		return false;

	if (si->playerInfos[player].castle != ID.toHeroType()->heroClass->faction)
		return false;

	if (vstd::contains(getUsedHeroes(), ID))
		return false;

	if (!mi->mapHeader->allowedHeroes.count(ID))
		return false;

	for (const auto & disposedHero : mi->mapHeader->disposedHeroes)
		if (disposedHero.heroId == ID && !disposedHero.players.count(player))
			return false;

	return true;
}

std::vector<HeroTypeID> CVCMIServer::getUsedHeroes()
{
	std::vector<HeroTypeID> heroIds;
	for(auto & p : si->playerInfos)
	{
		const auto & heroes = getPlayerInfo(p.first).heroesNames;
		for(auto & hero : heroes)
			if(hero.heroId.hasValue())
				heroIds.push_back(hero.heroId);

		if(p.second.hero != HeroTypeID::RANDOM)
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

void CVCMIServer::multiplayerWelcomeMessage()
{
	int humanPlayer = 0;
	for (const auto & pi : si->playerInfos)
		if(pi.second.isControlledByHuman())
			humanPlayer++;

	if(humanPlayer < 2) // Singleplayer
		return;

	gh->playerMessages->broadcastSystemMessage(MetaString::createFromTextID("vcmi.broadcast.command"));

	for (const auto & pi : si->playerInfos)
		if(!pi.second.handicap.startBonus.empty() || pi.second.handicap.percentIncome != 100 || pi.second.handicap.percentGrowth != 100)
		{
			MetaString str;
			str.appendTextID("vcmi.lobby.handicap");
			str.appendRawString(" ");
			str.appendName(pi.first);
			str.appendRawString(":");
			for(auto & res : EGameResID::ALL_RESOURCES())
				if(pi.second.handicap.startBonus[res] != 0)
				{
					str.appendRawString(" ");
					str.appendName(res);
					str.appendRawString(":");
					str.appendRawString(std::to_string(pi.second.handicap.startBonus[res]));
				}
			if(pi.second.handicap.percentIncome != 100)
			{
				str.appendRawString(" ");
				str.appendTextID("core.jktext.32");
				str.appendRawString(":");
				str.appendRawString(std::to_string(pi.second.handicap.percentIncome) + "%");
			}
			if(pi.second.handicap.percentGrowth != 100)
			{
				str.appendRawString(" ");
				str.appendTextID("core.genrltxt.194");
				str.appendRawString(":");
				str.appendRawString(std::to_string(pi.second.handicap.percentGrowth) + "%");
			}
			gh->playerMessages->broadcastSystemMessage(str);
		}

	std::vector<std::string> optionIds;
	if(si->extraOptionsInfo.cheatsAllowed)
		optionIds.emplace_back("vcmi.optionsTab.cheatAllowed.hover");
	if(si->extraOptionsInfo.unlimitedReplay)
		optionIds.emplace_back("vcmi.optionsTab.unlimitedReplay.hover");

	if(!optionIds.size()) // No settings to publish
		return;

	MetaString str;
	str.appendTextID("vcmi.optionsTab.extraOptions.hover");
	str.appendRawString(": ");
	for(int i = 0; i < optionIds.size(); i++)
	{
		str.appendTextID(optionIds[i]);
		if(i < optionIds.size() - 1)
			str.appendRawString(", ");
	}

	gh->playerMessages->broadcastSystemMessage(str);
}

INetworkHandler & CVCMIServer::getNetworkHandler()
{
	return *networkHandler;
}

INetworkServer & CVCMIServer::getNetworkServer()
{
	return *networkServer;
}
