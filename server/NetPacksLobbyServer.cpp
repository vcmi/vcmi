/*
 * NetPacksLobbyServer.cpp, part of VCMI engine
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

#include "../lib/NetPacksLobby.h"
#include "../lib/serializer/Connection.h"
#include "../lib/StartInfo.h"

// Campaigns
#include "../lib/mapping/CCampaignHandler.h"
#include "../lib/mapping/CMapService.h"
#include "../lib/mapping/CMapInfo.h"

bool CLobbyPackToServer::checkClientPermissions(CVCMIServer * srv) const
{
	return srv->isClientHost(c->connectionID);
}

void CLobbyPackToServer::applyOnServerAfterAnnounce(CVCMIServer * srv)
{
	// Propogate options after every CLobbyPackToServer
	srv->updateAndPropagateLobbyState();
}

bool LobbyClientConnected::checkClientPermissions(CVCMIServer * srv) const
{
	return true;
}

bool LobbyClientConnected::applyOnServer(CVCMIServer * srv)
{
	srv->clientConnected(c, names, uuid, mode);
	// Server need to pass some data to newly connected client
	clientId = c->connectionID;
	mode = srv->si->mode;
	hostClientId = srv->hostClientId;
	return true;
}

void LobbyClientConnected::applyOnServerAfterAnnounce(CVCMIServer * srv)
{
	// FIXME: we need to avoid senting something to client that not yet get answer for LobbyClientConnected
	// Until UUID set we only pass LobbyClientConnected to this client
	c->uuid = uuid;
	srv->updateAndPropagateLobbyState();
}

bool LobbyClientDisconnected::checkClientPermissions(CVCMIServer * srv) const
{
	if(clientId != c->connectionID)
		return false;

	if(shutdownServer)
	{
		if(!srv->cmdLineOptions.count("run-by-client"))
			return false;

		if(c->uuid != srv->cmdLineOptions["uuid"].as<std::string>())
			return false;
	}

	return true;
}

bool LobbyClientDisconnected::applyOnServer(CVCMIServer * srv)
{
	srv->clientDisconnected(c);
	return true;
}

void LobbyClientDisconnected::applyOnServerAfterAnnounce(CVCMIServer * srv)
{
	if(c->isOpen())
	{
		boost::unique_lock<boost::mutex> lock(*c->mutexWrite);
		c->close();
		c->connected = false;
	}

	if(shutdownServer)
	{
		logNetwork->info("Client requested shutdown, server will close itself...");
		srv->state = EServerState::SHUTDOWN;
		return;
	}
	else if(srv->connections.empty())
	{
		logNetwork->error("Last connection lost, server will close itself...");
		srv->state = EServerState::SHUTDOWN;
	}
	else if(c == srv->hostClient)
	{
		auto ph = new LobbyChangeHost();
		auto newHost = *RandomGeneratorUtil::nextItem(srv->connections, CRandomGenerator::getDefault());
		ph->newHostConnectionId = newHost->connectionID;
		srv->addToAnnounceQueue(ph);
	}
	srv->updateAndPropagateLobbyState();
}

bool LobbyChatMessage::checkClientPermissions(CVCMIServer * srv) const
{
	return true;
}

bool LobbySetMap::applyOnServer(CVCMIServer * srv)
{
	if (srv->state != EServerState::LOBBY)
		return false;
	srv->updateStartInfoOnMapChange(mapInfo, mapGenOpts);
	return true;
}

bool LobbySetCampaign::applyOnServer(CVCMIServer * srv)
{
	srv->si->mapname = ourCampaign->camp->header.filename;
	srv->si->mode = StartInfo::CAMPAIGN;
	srv->si->campState = ourCampaign;
	srv->si->turnTime = 0;
	bool isCurrentMapConquerable = ourCampaign->currentMap && ourCampaign->camp->conquerable(*ourCampaign->currentMap);
	for(int i = 0; i < ourCampaign->camp->scenarios.size(); i++)
	{
		if(ourCampaign->camp->conquerable(i))
		{
			if(!isCurrentMapConquerable || (isCurrentMapConquerable && i == *ourCampaign->currentMap))
			{
				srv->setCampaignMap(i);
			}
		}
	}
	return true;
}

bool LobbySetCampaignMap::applyOnServer(CVCMIServer * srv)
{
	srv->setCampaignMap(mapId);
	return true;
}

bool LobbySetCampaignBonus::applyOnServer(CVCMIServer * srv)
{
	srv->setCampaignBonus(bonusId);
	return true;
}

bool LobbyGuiAction::checkClientPermissions(CVCMIServer * srv) const
{
	return srv->isClientHost(c->connectionID);
}

bool LobbyStartGame::checkClientPermissions(CVCMIServer * srv) const
{
	return srv->isClientHost(c->connectionID);
}

bool LobbyStartGame::applyOnServer(CVCMIServer * srv)
{
	try
	{
		srv->verifyStateBeforeStart(true);
	}
	catch(...)
	{
		return false;
	}
	// Server will prepare gamestate and we announce StartInfo to clients
	srv->state = EServerState::GAMEPLAY_STARTING;
	srv->prepareToStartGame();
	initializedStartInfo = std::make_shared<StartInfo>(*srv->gh->getStartInfo(true));
	return true;
}

void LobbyStartGame::applyOnServerAfterAnnounce(CVCMIServer * srv)
{
	srv->startGameImmidiately();
}

bool LobbyChangeHost::checkClientPermissions(CVCMIServer * srv) const
{
	return srv->isClientHost(c->connectionID);
}

bool LobbyChangeHost::applyOnServer(CVCMIServer * srv)
{
	return true;
}

bool LobbyChangeHost::applyOnServerAfterAnnounce(CVCMIServer * srv)
{
	return srv->passHost(newHostConnectionId);
}

bool LobbyChangePlayerOption::checkClientPermissions(CVCMIServer * srv) const
{
	if(srv->isClientHost(c->connectionID))
		return true;

	if(vstd::contains(srv->getAllClientPlayers(c->connectionID), color))
		return true;

	return false;
}

bool LobbyChangePlayerOption::applyOnServer(CVCMIServer * srv)
{
	switch(what)
	{
	case TOWN:
		srv->optionNextCastle(color, direction);
		break;
	case HERO:
		srv->optionNextHero(color, direction);
		break;
	case BONUS:
		srv->optionNextBonus(color, direction);
		break;
	}
	return true;
}

bool LobbySetPlayer::applyOnServer(CVCMIServer * srv)
{
	srv->setPlayer(clickedColor);
	return true;
}

bool LobbySetTurnTime::applyOnServer(CVCMIServer * srv)
{
	srv->si->turnTime = turnTime;
	return true;
}

bool LobbySetDifficulty::applyOnServer(CVCMIServer * srv)
{
	srv->si->difficulty = vstd::abetween(difficulty, 0, 4);
	return true;
}

bool LobbyForceSetPlayer::applyOnServer(CVCMIServer * srv)
{
	srv->si->playerInfos[targetPlayerColor].connectedPlayerIDs.insert(targetConnectedPlayer);
	return true;
}
