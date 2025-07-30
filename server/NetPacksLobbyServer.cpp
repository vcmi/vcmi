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

#include "../lib/StartInfo.h"
#include "../lib/GameLibrary.h"

#include "../lib/CRandomGenerator.h"
#include "../lib/campaign/CampaignState.h"
#include "../lib/entities/faction/CTownHandler.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/mapping/CMapInfo.h"
#include "../lib/serializer/GameConnection.h"

void ClientPermissionsCheckerNetPackVisitor::visitForLobby(CPackForLobby & pack)
{
	if(pack.isForServer())
	{
		result = srv.isClientHost(connection->connectionID);
	}
}

void ApplyOnServerAfterAnnounceNetPackVisitor::visitForLobby(CPackForLobby & pack)
{
	// Propagate options after every CLobbyPackToServer
	if(pack.isForServer())
	{
		srv.updateAndPropagateLobbyState();
	}
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyClientConnected(LobbyClientConnected & pack)
{
	result = srv.getState() == EServerState::LOBBY;
}

void ApplyOnServerNetPackVisitor::visitLobbyClientConnected(LobbyClientConnected & pack)
{
	auto compatibleVersion = std::min(pack.version, ESerializationVersion::CURRENT);
	connection->setSerializationVersion(compatibleVersion);

	srv.clientConnected(connection, pack.names, pack.uuid, pack.mode);

	// Server need to pass some data to newly connected client
	pack.clientId = connection->connectionID;
	pack.mode = srv.si->mode;
	pack.hostClientId = srv.hostClientId;
	pack.version = compatibleVersion;

	result = true;
}

void ApplyOnServerAfterAnnounceNetPackVisitor::visitLobbyClientConnected(LobbyClientConnected & pack)
{
	srv.updateAndPropagateLobbyState();
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyClientDisconnected(LobbyClientDisconnected & pack)
{
	if(pack.clientId != connection->connectionID)
	{
		result = false;
		return;
	}

	if(pack.shutdownServer)
	{
		if(!srv.wasStartedByClient())
		{
			result = false;
			return;
		}

		if(connection->connectionID != srv.hostClientId)
		{
			result = false;
			return;
		}
	}

	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbyClientDisconnected(LobbyClientDisconnected & pack)
{
	connection->getConnection()->close();
	srv.clientDisconnected(connection);
	result = true;
}

void ApplyOnServerAfterAnnounceNetPackVisitor::visitLobbyClientDisconnected(LobbyClientDisconnected & pack)
{
	srv.updateAndPropagateLobbyState();
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyChatMessage(LobbyChatMessage & pack)
{
	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbySetMap(LobbySetMap & pack)
{
	if(srv.getState() != EServerState::LOBBY)
	{
		result = false;
		return;
	}

	srv.updateStartInfoOnMapChange(pack.mapInfo, pack.mapGenOpts);

	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbySetCampaign(LobbySetCampaign & pack)
{
	srv.si->mapname = pack.ourCampaign->getFilename();
	srv.si->mode = EStartMode::CAMPAIGN;
	srv.si->campState = pack.ourCampaign;
	srv.si->turnTimerInfo = TurnTimerInfo{};

	bool isCurrentMapConquerable = pack.ourCampaign->currentScenario() && pack.ourCampaign->isAvailable(*pack.ourCampaign->currentScenario());

	auto scenarios = pack.ourCampaign->allScenarios();
	for(auto scenarioID : boost::adaptors::reverse(scenarios)) // reverse -> on multiple scenario selection set lowest id at the end
	{
		if(pack.ourCampaign->isAvailable(scenarioID))
		{
			if(!isCurrentMapConquerable || (isCurrentMapConquerable && scenarioID == *pack.ourCampaign->currentScenario()))
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
	result = srv.isClientHost(connection->connectionID);
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyRestartGame(LobbyRestartGame & pack)
{
	result = srv.isClientHost(connection->connectionID);
}

void ApplyOnServerNetPackVisitor::visitLobbyRestartGame(LobbyRestartGame & pack)
{
	srv.prepareToRestart();

	result = true;
}

void ApplyOnServerAfterAnnounceNetPackVisitor::visitLobbyRestartGame(LobbyRestartGame & pack)
{
	for(const auto & connection : srv.activeConnections)
		connection->enterLobbyConnectionMode();
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyPrepareStartGame(LobbyPrepareStartGame & pack)
{
	result = srv.isClientHost(connection->connectionID);
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyStartGame(LobbyStartGame & pack)
{
	result = srv.isClientHost(connection->connectionID);
}

void ApplyOnServerNetPackVisitor::visitLobbyStartGame(LobbyStartGame & pack)
{
	try
	{
		srv.verifyStateBeforeStart(true);
	}
	catch(const std::exception &)
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
	
	pack.initializedStartInfo = std::make_shared<StartInfo>(*srv.gh->gameState().getInitialStartInfo());
	pack.initializedGameState = srv.gh->gs;
	result = true;
}

void ApplyOnServerAfterAnnounceNetPackVisitor::visitLobbyStartGame(LobbyStartGame & pack)
{
	srv.startGameImmediately();
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyChangeHost(LobbyChangeHost & pack)
{
	result = srv.isClientHost(connection->connectionID);
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
	if(srv.isClientHost(connection->connectionID))
	{
		result = true;
		return;
	}

	if(vstd::contains(srv.getAllClientPlayers(connection->connectionID), pack.color))
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
	case LobbyChangePlayerOption::TOWN_ID:
		srv.optionSetCastle(pack.color, FactionID(pack.value));
		break;
	case LobbyChangePlayerOption::TOWN:
		srv.optionNextCastle(pack.color, pack.value);
		break;
	case LobbyChangePlayerOption::HERO_ID:
		srv.optionSetHero(pack.color, HeroTypeID(pack.value));
		break;
	case LobbyChangePlayerOption::HERO:
		srv.optionNextHero(pack.color, pack.value);
		break;
	case LobbyChangePlayerOption::BONUS_ID:
		srv.optionSetBonus(pack.color, static_cast<PlayerStartingBonus>(pack.value));
		break;
	case LobbyChangePlayerOption::BONUS:
		srv.optionNextBonus(pack.color, pack.value);
		break;
	}
	
	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbySetPlayer(LobbySetPlayer & pack)
{
	srv.setPlayer(pack.clickedColor);
	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbySetPlayerName(LobbySetPlayerName & pack)
{
	srv.setPlayerName(pack.color, pack.name);
	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbySetPlayerHandicap(LobbySetPlayerHandicap & pack)
{
	srv.setPlayerHandicap(pack.color, pack.handicap);
	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbySetSimturns(LobbySetSimturns & pack)
{
	srv.si->simturnsInfo = pack.simturnsInfo;
	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbySetTurnTime(LobbySetTurnTime & pack)
{
	srv.si->turnTimerInfo = pack.turnTimerInfo;
	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbySetExtraOptions(LobbySetExtraOptions & pack)
{
	srv.si->extraOptionsInfo = pack.extraOptionsInfo;
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

void ClientPermissionsCheckerNetPackVisitor::visitLobbyPvPAction(LobbyPvPAction & pack)
{
	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbyPvPAction(LobbyPvPAction & pack)
{
	std::vector<FactionID> allowedTowns;

	for (auto const & factionID : LIBRARY->townh->getDefaultAllowed())
		if(std::find(pack.bannedTowns.begin(), pack.bannedTowns.end(), factionID) == pack.bannedTowns.end())
			allowedTowns.push_back(factionID);

	std::vector<FactionID> randomFaction1;
	std::sample(allowedTowns.begin(), allowedTowns.end(), std::back_inserter(randomFaction1), 1, std::mt19937{std::random_device{}()});
	std::vector<FactionID> randomFaction2;
	std::sample(allowedTowns.begin(), allowedTowns.end(), std::back_inserter(randomFaction2), 1, std::mt19937{std::random_device{}()});

	MetaString txt;

	switch(pack.action) {
		case LobbyPvPAction::COIN:
			txt.appendTextID("vcmi.lobby.pvp.coin.hover");
			txt.appendRawString(" - " + std::to_string(CRandomGenerator::getDefault().nextInt(1)));
			srv.announceTxt(txt);
			break;
		case LobbyPvPAction::RANDOM_TOWN:
			if(!allowedTowns.size())
				break;
			txt.appendTextID("core.overview.3");
			txt.appendRawString(" - ");
			txt.appendTextID(LIBRARY->townh->getById(randomFaction1[0])->getNameTextID());
			srv.announceTxt(txt);
			break;
		case LobbyPvPAction::RANDOM_TOWN_VS:
			if(!allowedTowns.size())
				break;
			txt.appendTextID("core.overview.3");
			txt.appendRawString(" - ");
			txt.appendTextID(LIBRARY->townh->getById(randomFaction1[0])->getNameTextID());
			txt.appendRawString(" ");
			txt.appendTextID("vcmi.lobby.pvp.versus");
			txt.appendRawString(" ");
			txt.appendTextID(LIBRARY->townh->getById(randomFaction2[0])->getNameTextID());
			srv.announceTxt(txt);
			break;
	}
	result = true;
}


void ClientPermissionsCheckerNetPackVisitor::visitLobbyDelete(LobbyDelete & pack)
{
	result = srv.isClientHost(connection->connectionID);
}

void ApplyOnServerNetPackVisitor::visitLobbyDelete(LobbyDelete & pack)
{
	if(pack.type == LobbyDelete::EType::SAVEGAME || pack.type == LobbyDelete::EType::RANDOMMAP)
	{
		auto res = ResourcePath(pack.name, pack.type == LobbyDelete::EType::SAVEGAME ? EResType::SAVEGAME : EResType::MAP);
		auto name = CResourceHandler::get()->getResourceName(res);
		if (!name)
		{
			logGlobal->error("Failed to find resource with name '%s'", res.getOriginalName());
			return;
		}

		boost::system::error_code ec;
		auto file = boost::filesystem::canonical(*name, ec);

		if (ec)
		{
			logGlobal->error("Failed to delete file '%s'. Reason: %s", res.getOriginalName(), ec.message());
		}
		else
		{
			boost::filesystem::remove(file);
			if(boost::filesystem::is_empty(file.parent_path()))
				boost::filesystem::remove(file.parent_path());
		}
	}
	else if(pack.type == LobbyDelete::EType::SAVEGAME_FOLDER)
	{
		auto res = ResourcePath("Saves/" + pack.name, EResType::DIRECTORY);
		auto name = CResourceHandler::get()->getResourceName(res);
		if (!name)
		{
			logGlobal->error("Failed to find folder with name '%s'", res.getOriginalName());
			return;
		}

		boost::system::error_code ec;
		auto folder = boost::filesystem::canonical(*name);
		if (ec)
		{
			logGlobal->error("Failed to delete folder '%s'. Reason: %s", res.getOriginalName(), ec.message());
		}
		else
		{
			boost::filesystem::remove_all(folder);
		}
	}

	LobbyUpdateState lus;
	lus.state = srv;
	lus.refreshList = true;
	srv.announcePack(lus);
}
