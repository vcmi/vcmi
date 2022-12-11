/*
 * RandomMapTab.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"

#include "RandomMapTab.h"
#include "CSelectionBase.h"

#include "../CGameInfo.h"
#include "../CServerHandler.h"
#include "../gui/CAnimation.h"
#include "../gui/CGuiHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"

#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../../lib/CModHandler.h"

RandomMapTab::RandomMapTab():
	InterfaceBuilder()
{
	recActions = 0;
	mapGenOptions = std::make_shared<CMapGenOptions>();
	
	const JsonNode config(ResourceID("config/windows.json"));
	addCallback("toggleMapSize", [&](int btnId)
	{
		auto mapSizeVal = getPossibleMapSizes();
		mapGenOptions->setWidth(mapSizeVal[btnId]);
		mapGenOptions->setHeight(mapSizeVal[btnId]);
		updateMapInfoByHost();
	});
	addCallback("toggleTwoLevels", [&](bool on)
	{
		mapGenOptions->setHasTwoLevels(on);
		updateMapInfoByHost();
	});
	
	addCallback("setPlayersCount", [&](int btnId)
	{
		mapGenOptions->setPlayerCount(btnId);
	
		if(auto w = dynamic_pointer_cast<CToggleGroup>(widget("groupMaxTeams")))
			deactivateButtonsFrom(w.get(), btnId);

		// deactive some CompOnlyPlayers buttons to prevent total number of players exceeds PlayerColor::PLAYER_LIMIT_I
		if(auto w = dynamic_pointer_cast<CToggleGroup>(widget("groupCompOnlyPlayers")))
			deactivateButtonsFrom(w.get(), PlayerColor::PLAYER_LIMIT_I - btnId + 1);

		validatePlayersCnt(btnId);
		updateMapInfoByHost();
	});
	
	addCallback("setTeamsCount", [&](int btnId)
	{
		mapGenOptions->setTeamCount(btnId);
		updateMapInfoByHost();
	});
	
	addCallback("setCompOnlyPlayers", [&](int btnId)
	{
		mapGenOptions->setCompOnlyPlayerCount(btnId);

		// deactive some MaxPlayers buttons to prevent total number of players exceeds PlayerColor::PLAYER_LIMIT_I
		if(auto w = dynamic_pointer_cast<CToggleGroup>(widget("groupMaxPlayers")))
			deactivateButtonsFrom(w.get(), PlayerColor::PLAYER_LIMIT_I - btnId + 1);
		
		if(auto w = dynamic_pointer_cast<CToggleGroup>(widget("groupCompOnlyTeams")))
			deactivateButtonsFrom(w.get(), (btnId == 0 ? 1 : btnId));
		validateCompOnlyPlayersCnt(btnId);
		updateMapInfoByHost();
	});
	
	addCallback("setCompOnlyTeams", [&](int btnId)
	{
		mapGenOptions->setCompOnlyTeamCount(btnId);
		updateMapInfoByHost();
	});
	
	addCallback("setWaterContent", [&](int btnId)
	{
		mapGenOptions->setWaterContent(static_cast<EWaterContent::EWaterContent>(btnId));
		updateMapInfoByHost();
	});
	
	addCallback("setMonsterStrenght", [&](int btnId)
	{
		if(btnId < 0)
			mapGenOptions->setMonsterStrength(EMonsterStrength::RANDOM);
		else
			mapGenOptions->setMonsterStrength(static_cast<EMonsterStrength::EMonsterStrength>(btnId)); //value 2 to 4
		updateMapInfoByHost();
	});
	
	init(config["randomMapTab"]);
	
	updateMapInfoByHost();
}

void RandomMapTab::updateMapInfoByHost()
{
	if(CSH->isGuest())
		return;

	// Generate header info
	mapInfo = std::make_shared<CMapInfo>();
	mapInfo->isRandomMap = true;
	mapInfo->mapHeader = make_unique<CMapHeader>();
	mapInfo->mapHeader->version = EMapFormat::SOD;
	mapInfo->mapHeader->name = CGI->generaltexth->allTexts[740];
	mapInfo->mapHeader->description = CGI->generaltexth->allTexts[741];
	mapInfo->mapHeader->difficulty = 1; // Normal
	mapInfo->mapHeader->height = mapGenOptions->getHeight();
	mapInfo->mapHeader->width = mapGenOptions->getWidth();
	mapInfo->mapHeader->twoLevel = mapGenOptions->getHasTwoLevels();

	// Generate player information
	mapInfo->mapHeader->players.clear();
	int playersToGen = PlayerColor::PLAYER_LIMIT_I;
	if(mapGenOptions->getPlayerCount() != CMapGenOptions::RANDOM_SIZE)
	{
		if(mapGenOptions->getCompOnlyPlayerCount() != CMapGenOptions::RANDOM_SIZE)
			playersToGen = mapGenOptions->getPlayerCount() + mapGenOptions->getCompOnlyPlayerCount();
		else
			playersToGen = mapGenOptions->getPlayerCount();
	}


	mapInfo->mapHeader->howManyTeams = playersToGen;

	for(int i = 0; i < playersToGen; ++i)
	{
		PlayerInfo player;
		player.isFactionRandom = true;
		player.canComputerPlay = true;
		if(mapGenOptions->getCompOnlyPlayerCount() != CMapGenOptions::RANDOM_SIZE && i >= mapGenOptions->getPlayerCount())
		{
			player.canHumanPlay = false;
		}
		else
		{
			player.canHumanPlay = true;
		}
		player.team = TeamID(i);
		player.hasMainTown = true;
		player.generateHeroAtMainTown = true;
		mapInfo->mapHeader->players.push_back(player);
	}

	mapInfoChanged(mapInfo, mapGenOptions);
}

void RandomMapTab::setMapGenOptions(std::shared_ptr<CMapGenOptions> opts)
{
	if(auto w = dynamic_pointer_cast<CToggleGroup>(widget("groupMapSize")))
		w->setSelected(vstd::find_pos(getPossibleMapSizes(), opts->getWidth()));
	if(auto w = dynamic_pointer_cast<CToggleButton>(widget("buttonTwoLevels")))
		w->setSelected(opts->getHasTwoLevels());
	if(auto w = dynamic_pointer_cast<CToggleGroup>(widget("groupMaxPlayers")))
		w->setSelected(opts->getPlayerCount());
	if(auto w = dynamic_pointer_cast<CToggleGroup>(widget("groupMaxTeams")))
		w->setSelected(opts->getTeamCount());
	if(auto w = dynamic_pointer_cast<CToggleGroup>(widget("groupCompOnlyPlayers")))
		w->setSelected(opts->getCompOnlyPlayerCount());
	if(auto w = dynamic_pointer_cast<CToggleGroup>(widget("groupCompOnlyTeams")))
		w->setSelected(opts->getCompOnlyTeamCount());
	if(auto w = dynamic_pointer_cast<CToggleGroup>(widget("groupWaterContent")))
		w->setSelected(opts->getWaterContent());
	if(auto w = dynamic_pointer_cast<CToggleGroup>(widget("groupgroupMonsterStrengthMaxTeams")))
		w->setSelected(opts->getMonsterStrength());
}

void RandomMapTab::deactivateButtonsFrom(CToggleGroup * group, int startId)
{
	logGlobal->debug("Blocking buttons from %d", startId);
	for(auto toggle : group->buttons)
	{
		if(auto button = std::dynamic_pointer_cast<CToggleButton>(toggle.second))
		{
			if(startId == CMapGenOptions::RANDOM_SIZE || toggle.first < startId)
			{
				button->block(false);
			}
			else
			{
				button->block(true);
			}
		}
	}
}

void RandomMapTab::validatePlayersCnt(int playersCnt)
{
	if(playersCnt == CMapGenOptions::RANDOM_SIZE)
	{
		return;
	}

	if(mapGenOptions->getTeamCount() >= playersCnt)
	{
		mapGenOptions->setTeamCount(playersCnt - 1);
		if(auto w = dynamic_pointer_cast<CToggleGroup>(widget("groupMaxTeams")))
			w->setSelected(mapGenOptions->getTeamCount());
	}
	// total players should not exceed PlayerColor::PLAYER_LIMIT_I (8 in homm3)
	if(mapGenOptions->getCompOnlyPlayerCount() + playersCnt > PlayerColor::PLAYER_LIMIT_I)
	{
		mapGenOptions->setCompOnlyPlayerCount(PlayerColor::PLAYER_LIMIT_I - playersCnt);
		if(auto w = dynamic_pointer_cast<CToggleGroup>(widget("groupCompOnlyPlayers")))
			w->setSelected(mapGenOptions->getCompOnlyPlayerCount());
	}

	validateCompOnlyPlayersCnt(mapGenOptions->getCompOnlyPlayerCount());
}

void RandomMapTab::validateCompOnlyPlayersCnt(int compOnlyPlayersCnt)
{
	if(compOnlyPlayersCnt == CMapGenOptions::RANDOM_SIZE)
	{
		return;
	}

	if(mapGenOptions->getCompOnlyTeamCount() >= compOnlyPlayersCnt)
	{
		int compOnlyTeamCount = compOnlyPlayersCnt == 0 ? 0 : compOnlyPlayersCnt - 1;
		mapGenOptions->setCompOnlyTeamCount(compOnlyTeamCount);
		updateMapInfoByHost();
		if(auto w = dynamic_pointer_cast<CToggleGroup>(widget("groupCompOnlyTeams")))
			w->setSelected(compOnlyTeamCount);
	}
}

std::vector<int> RandomMapTab::getPossibleMapSizes()
{
	return {CMapHeader::MAP_SIZE_SMALL, CMapHeader::MAP_SIZE_MIDDLE, CMapHeader::MAP_SIZE_LARGE, CMapHeader::MAP_SIZE_XLARGE};
}
