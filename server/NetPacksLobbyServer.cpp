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
#include "LobbyNetPackVisitors.h"

#include "CVCMIServer.h"
#include "CGameHandler.h"

#include "../lib/NetPacksLobby.h"
#include "../lib/serializer/Connection.h"
#include "../lib/StartInfo.h"

// Campaigns
#include "../lib/campaign/CampaignState.h"

void ClientPermissionsCheckerNetPackVisitor::visitForLobby(CPackForLobby & pack)
{
	if(pack.isForServer())
	{
		result = srv.isClientHost(pack.c->connectionID);
	}
}

void ApplyOnServerAfterAnnounceNetPackVisitor::visitForLobby(CPackForLobby & pack)
{
	// Propogate options after every CLobbyPackToServer
	if(pack.isForServer())
	{
		srv.updateAndPropagateLobbyState();
	}
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyClientConnected(LobbyClientConnected & pack)
{
	if(srv.gh)
	{
		for(auto & connection : srv.hangingConnections)
		{
			if(connection->uuid == pack.uuid)
			{
				{
				result = true;
				return;
				}
			}
		}
	}
	
	if(srv.state == EServerState::LOBBY)
		{
		result = true;
		return;
		}
	
	//disconnect immediately and ignore this client
	srv.connections.erase(pack.c);
	if(pack.c && pack.c->isOpen())
	{
		pack.c->close();
		pack.c->connected = false;
	}
	{
	result = false;
	return;
	}
}

void ApplyOnServerNetPackVisitor::visitLobbyClientConnected(LobbyClientConnected & pack)
{
	if(srv.gh)
	{
		for(auto & connection : srv.hangingConnections)
		{
			if(connection->uuid == pack.uuid)
			{
				logNetwork->info("Reconnection player");
				pack.c->connectionID = connection->connectionID;
				for(auto & playerConnection : srv.gh->connections)
				{
					for(auto & existingConnection : playerConnection.second)
					{
						if(existingConnection == connection)
						{
							playerConnection.second.erase(existingConnection);
							playerConnection.second.insert(pack.c);
							break;
						}
					}
				}
				srv.hangingConnections.erase(connection);
				break;
			}
		}
	}
	
	srv.clientConnected(pack.c, pack.names, pack.uuid, pack.mode);
	// Server need to pass some data to newly connected client
	pack.clientId = pack.c->connectionID;
	pack.mode = srv.si->mode;
	pack.hostClientId = srv.hostClientId;

	result = true;
}

void ApplyOnServerAfterAnnounceNetPackVisitor::visitLobbyClientConnected(LobbyClientConnected & pack)
{
	// FIXME: we need to avoid senting something to client that not yet get answer for LobbyClientConnected
	// Until UUID set we only pass LobbyClientConnected to this client
	pack.c->uuid = pack.uuid;
	srv.updateAndPropagateLobbyState();
	if(srv.state == EServerState::GAMEPLAY)
	{
		//immediately start game
		std::unique_ptr<LobbyStartGame> startGameForReconnectedPlayer(new LobbyStartGame);
		startGameForReconnectedPlayer->initializedStartInfo = srv.si;
		startGameForReconnectedPlayer->initializedGameState = srv.gh->gameState();
		startGameForReconnectedPlayer->clientId = pack.c->connectionID;
		srv.addToAnnounceQueue(std::move(startGameForReconnectedPlayer));
	}
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyClientDisconnected(LobbyClientDisconnected & pack)
{
	if(pack.clientId != pack.c->connectionID)
	{
		result = false;
		return;
	}

	if(pack.shutdownServer)
	{
		if(!srv.cmdLineOptions.count("run-by-client"))
		{
			result = false;
			return;
		}

		if(pack.c->uuid != srv.cmdLineOptions["uuid"].as<std::string>())
		{
			result = false;
			return;
		}
	}

	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbyClientDisconnected(LobbyClientDisconnected & pack)
{
	srv.clientDisconnected(pack.c);
	pack.c->close();
	pack.c->connected = false;

	result = true;
}

void ApplyOnServerAfterAnnounceNetPackVisitor::visitLobbyClientDisconnected(LobbyClientDisconnected & pack)
{
	if(pack.c && pack.c->isOpen())
	{
		boost::unique_lock<boost::mutex> lock(*pack.c->mutexWrite);
		pack.c->close();
		pack.c->connected = false;
	}

	if(pack.shutdownServer)
	{
		logNetwork->info("Client requested shutdown, server will close itself...");
		srv.state = EServerState::SHUTDOWN;
		return;
	}
	else if(srv.connections.empty())
	{
		logNetwork->error("Last connection lost, server will close itself...");
		srv.state = EServerState::SHUTDOWN;
	}
	else if(pack.c == srv.hostClient)
	{
		auto ph = std::make_unique<LobbyChangeHost>();
		auto newHost = *RandomGeneratorUtil::nextItem(srv.connections, CRandomGenerator::getDefault());
		ph->newHostConnectionId = newHost->connectionID;
		srv.addToAnnounceQueue(std::move(ph));
	}
	srv.updateAndPropagateLobbyState();
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyChatMessage(LobbyChatMessage & pack)
{
	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbySetMap(LobbySetMap & pack)
{
	if(srv.state != EServerState::LOBBY)
	{
		result = false;
		return;
	}

	srv.updateStartInfoOnMapChange(pack.mapInfo, pack.mapGenOpts);

	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbySetCampaign(LobbySetCampaign & pack)
{
	srv.si->mapname = pack.ourCampaign->header.filename;
	srv.si->mode = StartInfo::CAMPAIGN;
	srv.si->campState = pack.ourCampaign;
	srv.si->turnTime = 0;
	bool isCurrentMapConquerable = pack.ourCampaign->currentMap && pack.ourCampaign->conquerable(*pack.ourCampaign->currentMap);
	for(int i = 0; i < pack.ourCampaign->scenarios.size(); i++)
	{
		auto scenarioID = static_cast<CampaignScenarioID>(i);

		if(pack.ourCampaign->conquerable(scenarioID))
		{
			if(!isCurrentMapConquerable || (isCurrentMapConquerable && scenarioID == *pack.ourCampaign->currentMap))
			{
				srv.setCampaignMap(scenarioID);
			}
		}
	}
	
	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbySetCampaignMap(LobbySetCampaignMap & pack)
{
	srv.setCampaignMap(pack.mapId);

	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbySetCampaignBonus(LobbySetCampaignBonus & pack)
{
	srv.setCampaignBonus(pack.bonusId);

	result = true;
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyGuiAction(LobbyGuiAction & pack)
{
	result = srv.isClientHost(pack.c->connectionID);
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyEndGame(LobbyEndGame & pack)
{
	result = srv.isClientHost(pack.c->connectionID);
}

void ApplyOnServerNetPackVisitor::visitLobbyEndGame(LobbyEndGame & pack)
{
	srv.prepareToRestart();

	result = true;
}

void ApplyOnServerAfterAnnounceNetPackVisitor::visitLobbyEndGame(LobbyEndGame & pack)
{
	boost::unique_lock<boost::mutex> stateLock(srv.stateMutex);
	for(auto & c : srv.connections)
	{
		c->enterLobbyConnectionMode();
		c->disableStackSendingByID();
	}
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyStartGame(LobbyStartGame & pack)
{
	result = srv.isClientHost(pack.c->connectionID);
}

void ApplyOnServerNetPackVisitor::visitLobbyStartGame(LobbyStartGame & pack)
{
	try
	{
		srv.verifyStateBeforeStart(true);
	}
	catch(...)
	{
		result = false;
		return;
	}
	// Server will prepare gamestate and we announce StartInfo to clients
	if(!srv.prepareToStartGame())
	{
		result = false;
		return;
	}
	
	pack.initializedStartInfo = std::make_shared<StartInfo>(*srv.gh->getStartInfo(true));
	pack.initializedGameState = srv.gh->gameState();

	result = true;
}

void ApplyOnServerAfterAnnounceNetPackVisitor::visitLobbyStartGame(LobbyStartGame & pack)
{
	if(pack.clientId == -1) //do not restart game for single client only
		srv.startGameImmidiately();
	else
	{
		for(auto & c : srv.connections)
		{
			if(c->connectionID == pack.clientId)
			{
				c->enterGameplayConnectionMode(srv.gh->gameState());
				srv.reconnectPlayer(pack.clientId);
			}
		}
	}
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyChangeHost(LobbyChangeHost & pack)
{
	result = srv.isClientHost(pack.c->connectionID);
}

void ApplyOnServerNetPackVisitor::visitLobbyChangeHost(LobbyChangeHost & pack)
{
	result = true;
}

void ApplyOnServerAfterAnnounceNetPackVisitor::visitLobbyChangeHost(LobbyChangeHost & pack)
{
	auto result = srv.passHost(pack.newHostConnectionId);
	
	if(!result)
	{
		logGlobal->error("passHost returned false. What does it mean?");
	}
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyChangePlayerOption(LobbyChangePlayerOption & pack)
{
	if(srv.isClientHost(pack.c->connectionID))
	{
		result = true;
		return;
	}

	if(vstd::contains(srv.getAllClientPlayers(pack.c->connectionID), pack.color))
	{
		result = true;
		return;
	}

	result = false;
}

void ApplyOnServerNetPackVisitor::visitLobbyChangePlayerOption(LobbyChangePlayerOption & pack)
{
	switch(pack.what)
	{
	case LobbyChangePlayerOption::TOWN:
		srv.optionNextCastle(pack.color, pack.direction);
		break;
	case LobbyChangePlayerOption::HERO:
		srv.optionNextHero(pack.color, pack.direction);
		break;
	case LobbyChangePlayerOption::BONUS:
		srv.optionNextBonus(pack.color, pack.direction);
		break;
	}
	
	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbySetPlayer(LobbySetPlayer & pack)
{
	srv.setPlayer(pack.clickedColor);
	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbySetTurnTime(LobbySetTurnTime & pack)
{
	srv.si->turnTime = pack.turnTime;
	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbySetDifficulty(LobbySetDifficulty & pack)
{
	srv.si->difficulty = std::clamp<uint8_t>(pack.difficulty, 0, 4);
	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbyForceSetPlayer(LobbyForceSetPlayer & pack)
{
	srv.si->playerInfos[pack.targetPlayerColor].connectedPlayerIDs.insert(pack.targetConnectedPlayer);
	result = true;
}
