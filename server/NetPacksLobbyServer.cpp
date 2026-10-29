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
#include "lobby/LastSavedGames.h"
#include "lobby/ScenarioListBuilder.h"

#include "../lib/StartInfo.h"
#include "../lib/VCMIDirs.h"
#include "../lib/GameLibrary.h"
#include "../lib/modding/CModHandler.h"
#include "../lib/modding/ModDescription.h"

#include "../lib/CRandomGenerator.h"
#include "../lib/campaign/CampaignState.h"
#include "../lib/entities/faction/CTownHandler.h"
#include "../lib/filesystem/Filesystem.h"
#include "../lib/gameState/CGameState.h"
#include "../lib/mapping/CMapInfo.h"
#include "../lib/mapping/CMapHeader.h"
#include "../lib/serializer/GameConnection.h"

/// Returns true if canonical path points to an entry inside directory, and not to directory itself
static bool isInsideDirectory(const boost::filesystem::path & canonicalPath, const boost::filesystem::path & directory)
{
	boost::system::error_code ec;
	const auto canonicalDirectory = boost::filesystem::canonical(directory, ec);
	if(ec)
		return false;

	const auto [directoryEnd, pathEnd] = std::mismatch(canonicalDirectory.begin(), canonicalDirectory.end(), canonicalPath.begin(), canonicalPath.end());
	return directoryEnd == canonicalDirectory.end() && pathEnd != canonicalPath.end();
}

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

void ClientPermissionsCheckerNetPackVisitor::visitLobbyQueryState(LobbyQueryState & pack)
{
	// Anyone can query lobby state without being a registered player
	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbyQueryState(LobbyQueryState & pack)
{
	LobbyModsCheck lms;
	lms.vcmiVersion = VCMI_VERSION_STRING;

	for(const auto & modId : LIBRARY->modh->getActiveMods())
		lms.mods[modId] = LIBRARY->modh->getModInfo(modId).getVerificationInfo();

	for(const auto & [playerId, player] : srv.playerNames)
	{
		lms.participantNames.push_back(player.name);
		if(player.connection == srv.hostClientId)
			lms.hostAccountDisplayName = player.name;
	}

	connection->sendPack(lms);
	result = false;
}

void ApplyOnServerAfterAnnounceNetPackVisitor::visitLobbyQueryState(LobbyQueryState & pack)
{
	// Do nothing - query response is sent directly, no broadcast needed
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyQuickLoadGame(LobbyQuickLoadGame & pack)
{
	// only host can load quicksave
	result = srv.isClientHost(connection->connectionID);
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyClientConnected(LobbyClientConnected & pack)
{
	result = srv.getState() == EServerState::LOBBY;
}

void ApplyOnServerNetPackVisitor::visitLobbyClientConnected(LobbyClientConnected & pack)
{
	if(pack.version < ESerializationVersion::MINIMAL_LOBBY)
	{
		// client of an older version can not read packs of the current lobby protocol
		logNetwork->error("Rejecting client with incompatible serialization version %d", static_cast<int>(pack.version));
		connection->getConnection()->close();
		vstd::erase(srv.activeConnections, connection);
		result = false;
		return;
	}

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
	// other clients already have map info, only the new client needs it
	LobbyUpdateMapInfo lumi;
	lumi.mapInfo = srv.mi;
	for(const auto & connection : srv.activeConnections)
		if(connection->connectionID == pack.clientId)
			connection->sendPack(lumi);

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

void ApplyOnServerNetPackVisitor::visitLobbyQuickLoadGame(LobbyQuickLoadGame & pack)
{
	*srv.si = *srv.gh->gs->getStartInfo();
	srv.prepareToRestart();
	// modify StartInfo to load the quicksave
	srv.si->mode = EStartMode::LOAD_GAME;
	srv.si->mapname = pack.saveFilePath;
	result = true;
}

void ApplyOnServerAfterAnnounceNetPackVisitor::visitLobbyQuickLoadGame(LobbyQuickLoadGame & pack)
{
	for(const auto & connection : srv.activeConnections)
		connection->enterLobbyConnectionMode();
}

void ApplyOnServerNetPackVisitor::visitLobbySetMap(LobbySetMap & pack)
{
	if(srv.getState() != EServerState::LOBBY)
	{
		result = false;
		return;
	}

	srv.updateStartInfoOnMapChange(pack.mapInfo, pack.mapGenOpts);

	// pack contains whole map header and clients receive it in LobbyUpdateMapInfo, so it is not announced
	srv.updateAndPropagateLobbyState();
	result = false;
}

void ApplyOnServerNetPackVisitor::visitLobbySetCampaign(LobbySetCampaign & pack)
{
	srv.si->mapname = pack.ourCampaign->getFilename();
	srv.si->mode = EStartMode::CAMPAIGN;
	srv.si->campState = pack.ourCampaign;
	srv.si->turnTimerInfo = TurnTimerInfo{};

	bool isCurrentMapConquerable = pack.ourCampaign->currentScenario() && pack.ourCampaign->isAvailable(*pack.ourCampaign->currentScenario());

	auto scenarios = pack.ourCampaign->allScenarios();
	for(auto scenarioID : std::views::reverse(scenarios)) // reverse -> on multiple scenario selection set lowest id at the end
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
	*srv.si = *srv.gh->gs->getInitialStartInfo();
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
	// reject option changes for a player not present in the current map (invalid color, or campaign/random-map lobby without a header)
	if(!srv.mi || !srv.mi->mapHeader || pack.color.getNum() >= srv.mi->mapHeader->players.size())
	{
		srv.announceMessage("Cannot change options for player " + pack.color.toString() + " - not present in the current map");
		return;
	}

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

	// list shown after deletion is the last one requested
	if(!srv.lastScenarioListRequest)
	{
		logNetwork->error("Rejecting deletion of files before any list was requested");
		result = false;
	}
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbyRequestScenarioList(LobbyRequestScenarioList & pack)
{
	result = srv.isClientHost(connection->connectionID);

	const bool validLoadMode = pack.loadMode == ELoadMode::SINGLE || pack.loadMode == ELoadMode::MULTI || pack.loadMode == ELoadMode::CAMPAIGN || pack.loadMode == ELoadMode::TUTORIAL;
	if(pack.type != ScenarioListEntry::EType::MAP && (pack.type != ScenarioListEntry::EType::SAVE || !validLoadMode))
	{
		logNetwork->error("Rejecting scenario list request with invalid type %d or load mode %d", static_cast<int>(pack.type), static_cast<int>(pack.loadMode));
		result = false;
	}
}

void ClientPermissionsCheckerNetPackVisitor::visitLobbySetBattleOnlyModeStartInfo(LobbySetBattleOnlyModeStartInfo & pack)
{
	result = true;
}

void ApplyOnServerNetPackVisitor::visitLobbyRequestScenarioList(LobbyRequestScenarioList & pack)
{
	const ScenarioListBuilder builder(pack.type, pack.loadMode);
	LobbyScenarioList reply;
	if(pack.selectLastSave && pack.type == ScenarioListEntry::EType::SAVE)
		reply.list = builder.buildForSave(LastSavedGames::get(pack.loadMode), reply.selectedFile);
	else
		reply.list = builder.build(pack.folder);
	connection->sendPack(reply);

	srv.lastScenarioListRequest = pack;
	srv.lastScenarioListRequest->folder = reply.list.folder;
	srv.lastScenarioListRequest->selectLastSave = false;
	result = false;
}

static void deleteFile(LobbyDelete::EType type, const std::string & fileName)
{
	if(type == LobbyDelete::EType::SAVEGAME || type == LobbyDelete::EType::RANDOMMAP)
	{
		auto res = ResourcePath(fileName, type == LobbyDelete::EType::SAVEGAME ? EResType::SAVEGAME : EResType::MAP);
		auto name = CResourceHandler::get()->getResourceName(res);
		if (!name)
		{
			logGlobal->error("Failed to find resource with name '%s'", res.getOriginalName());
			return;
		}

		boost::system::error_code ec;
		auto file = boost::filesystem::canonical(*name, ec);
		const auto directory = type == LobbyDelete::EType::SAVEGAME ? VCMIDirs::get().userSavePath() : VCMIDirs::get().userDataPath() / "Maps" / "RandomMaps";

		if (ec)
		{
			logGlobal->error("Failed to delete file '%s'. Reason: %s", res.getOriginalName(), ec.message());
		}
		else if (!isInsideDirectory(file, directory))
		{
			logGlobal->error("Refusing to delete file '%s' outside of directory '%s'", res.getOriginalName(), directory.string());
		}
		else
		{
			boost::filesystem::remove(file);
			if(boost::filesystem::is_empty(file.parent_path()))
				boost::filesystem::remove(file.parent_path());
		}
	}
	else if(type == LobbyDelete::EType::SAVEGAME_FOLDER)
	{
		auto res = ResourcePath("Saves/" + fileName, EResType::DIRECTORY);
		auto name = CResourceHandler::get()->getResourceName(res);
		if (!name)
		{
			logGlobal->error("Failed to find folder with name '%s'", res.getOriginalName());
			return;
		}

		boost::system::error_code ec;
		auto folder = boost::filesystem::canonical(*name, ec);
		if (ec)
		{
			logGlobal->error("Failed to delete folder '%s'. Reason: %s", res.getOriginalName(), ec.message());
		}
		else if (!isInsideDirectory(folder, VCMIDirs::get().userSavePath()))
		{
			logGlobal->error("Refusing to delete folder '%s' outside of directory '%s'", res.getOriginalName(), VCMIDirs::get().userSavePath().string());
		}
		else
		{
			boost::filesystem::remove_all(folder);
		}
	}
}

void ApplyOnServerNetPackVisitor::visitLobbyDelete(LobbyDelete & pack)
{
	for(const auto & name : pack.names)
		deleteFile(pack.type, name);

	LobbyScenarioList reply;
	reply.list = ScenarioListBuilder(srv.lastScenarioListRequest->type, srv.lastScenarioListRequest->loadMode).build(srv.lastScenarioListRequest->folder);
	connection->sendPack(reply);
	result = false;
}
