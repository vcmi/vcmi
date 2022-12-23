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

RandomMapTab::RandomMapTab()
{
	recActions = 0;
	mapGenOptions = std::make_shared<CMapGenOptions>();
	OBJ_CONSTRUCTION;
	background = std::make_shared<CPicture>("RANMAPBK", 0, 6);

	labelHeadlineBig = std::make_shared<CLabel>(222, 36, FONT_BIG, ETextAlignment::CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[738]);
	labelHeadlineSmall = std::make_shared<CLabel>(222, 56, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[739]);

	labelMapSize = std::make_shared<CLabel>(104, 97, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, CGI->generaltexth->allTexts[752]);
	groupMapSize = std::make_shared<CToggleGroup>(0);
	groupMapSize->pos.y += 81;
	groupMapSize->pos.x += 158;
	const std::vector<std::string> mapSizeBtns = {"RANSIZS", "RANSIZM", "RANSIZL", "RANSIZX"};
	addButtonsToGroup(groupMapSize.get(), mapSizeBtns, 0, 3, 47, 198);
	groupMapSize->setSelected(1);
	groupMapSize->addCallback([&](int btnId)
	{
		auto mapSizeVal = getPossibleMapSizes();
		mapGenOptions->setWidth(mapSizeVal[btnId]);
		mapGenOptions->setHeight(mapSizeVal[btnId]);
		updateMapInfoByHost();
	});

	buttonTwoLevels = std::make_shared<CToggleButton>(Point(346, 81), "RANUNDR", CGI->generaltexth->zelp[202]);
	buttonTwoLevels->setSelected(true);
	buttonTwoLevels->addCallback([&](bool on)
	{
		mapGenOptions->setHasTwoLevels(on);
		updateMapInfoByHost();
	});

	labelGroupForOptions = std::make_shared<CLabelGroup>(FONT_SMALL, ETextAlignment::TOPLEFT, Colors::WHITE);
	// Create number defs list
	std::vector<std::string> numberDefs;
	for(int i = 0; i <= 8; ++i)
	{
		numberDefs.push_back("RANNUM" + boost::lexical_cast<std::string>(i));
	}

	const int NUMBERS_WIDTH = 32;
	const int BTNS_GROUP_LEFT_MARGIN = 67;
	labelGroupForOptions->add(68, 133, CGI->generaltexth->allTexts[753]);
	groupMaxPlayers = std::make_shared<CToggleGroup>(0);
	groupMaxPlayers->pos.y += 153;
	groupMaxPlayers->pos.x += BTNS_GROUP_LEFT_MARGIN;
	addButtonsWithRandToGroup(groupMaxPlayers.get(), numberDefs, 1, 8, NUMBERS_WIDTH, 204, 212);
	groupMaxPlayers->addCallback([&](int btnId)
	{
		mapGenOptions->setPlayerCount(btnId);
		deactivateButtonsFrom(groupMaxTeams.get(), btnId);

		// deactive some CompOnlyPlayers buttons to prevent total number of players exceeds PlayerColor::PLAYER_LIMIT_I
		deactivateButtonsFrom(groupCompOnlyPlayers.get(), PlayerColor::PLAYER_LIMIT_I - btnId + 1);

		validatePlayersCnt(btnId);
		updateMapInfoByHost();
	});

	labelGroupForOptions->add(68, 199, CGI->generaltexth->allTexts[754]);
	groupMaxTeams = std::make_shared<CToggleGroup>(0);
	groupMaxTeams->pos.y += 219;
	groupMaxTeams->pos.x += BTNS_GROUP_LEFT_MARGIN;
	addButtonsWithRandToGroup(groupMaxTeams.get(), numberDefs, 0, 7, NUMBERS_WIDTH, 214, 222);
	groupMaxTeams->addCallback([&](int btnId)
	{
		mapGenOptions->setTeamCount(btnId);
		updateMapInfoByHost();
	});

	labelGroupForOptions->add(68, 265, CGI->generaltexth->allTexts[755]);
	groupCompOnlyPlayers = std::make_shared<CToggleGroup>(0);
	groupCompOnlyPlayers->pos.y += 285;
	groupCompOnlyPlayers->pos.x += BTNS_GROUP_LEFT_MARGIN;
	addButtonsWithRandToGroup(groupCompOnlyPlayers.get(), numberDefs, 0, 7, NUMBERS_WIDTH, 224, 232);
	groupCompOnlyPlayers->addCallback([&](int btnId)
	{
		mapGenOptions->setCompOnlyPlayerCount(btnId);
		
		// deactive some MaxPlayers buttons to prevent total number of players exceeds PlayerColor::PLAYER_LIMIT_I
		deactivateButtonsFrom(groupMaxPlayers.get(), PlayerColor::PLAYER_LIMIT_I - btnId + 1);
		
		deactivateButtonsFrom(groupCompOnlyTeams.get(), (btnId == 0 ? 1 : btnId));
		validateCompOnlyPlayersCnt(btnId);
		updateMapInfoByHost();
	});

	labelGroupForOptions->add(68, 331, CGI->generaltexth->allTexts[756]);
	groupCompOnlyTeams = std::make_shared<CToggleGroup>(0);
	groupCompOnlyTeams->pos.y += 351;
	groupCompOnlyTeams->pos.x += BTNS_GROUP_LEFT_MARGIN;
	addButtonsWithRandToGroup(groupCompOnlyTeams.get(), numberDefs, 0, 6, NUMBERS_WIDTH, 234, 241);
	deactivateButtonsFrom(groupCompOnlyTeams.get(), 1);
	groupCompOnlyTeams->addCallback([&](int btnId)
	{
		mapGenOptions->setCompOnlyTeamCount(btnId);
		updateMapInfoByHost();
	});

	labelGroupForOptions->add(68, 398, CGI->generaltexth->allTexts[757]);
	const int WIDE_BTN_WIDTH = 85;
	groupWaterContent = std::make_shared<CToggleGroup>(0);
	groupWaterContent->pos.y += 419;
	groupWaterContent->pos.x += BTNS_GROUP_LEFT_MARGIN;
	const std::vector<std::string> waterContentBtns = {"RANNONE", "RANNORM", "RANISLD"};
	addButtonsWithRandToGroup(groupWaterContent.get(), waterContentBtns, 0, 2, WIDE_BTN_WIDTH, 243, 246);
	groupWaterContent->addCallback([&](int btnId)
	{
		mapGenOptions->setWaterContent(static_cast<EWaterContent::EWaterContent>(btnId));
		updateMapInfoByHost();
	});

	labelGroupForOptions->add(68, 465, CGI->generaltexth->allTexts[758]);
	groupMonsterStrength = std::make_shared<CToggleGroup>(0);
	groupMonsterStrength->pos.y += 485;
	groupMonsterStrength->pos.x += BTNS_GROUP_LEFT_MARGIN;
	const std::vector<std::string> monsterStrengthBtns = {"RANWEAK", "RANNORM", "RANSTRG"};
	addButtonsWithRandToGroup(groupMonsterStrength.get(), monsterStrengthBtns, 2, 4, WIDE_BTN_WIDTH, 248, 251, EMonsterStrength::RANDOM, false);
	groupMonsterStrength->addCallback([&](int btnId)
	{
		if(btnId < 0)
			mapGenOptions->setMonsterStrength(EMonsterStrength::RANDOM);
		else
			mapGenOptions->setMonsterStrength(static_cast<EMonsterStrength::EMonsterStrength>(btnId)); //value 2 to 4
		updateMapInfoByHost();
	});

	buttonShowRandomMaps = std::make_shared<CButton>(Point(54, 535), "RANSHOW", CGI->generaltexth->zelp[252]);

	updateMapInfoByHost();
}

void RandomMapTab::updateMapInfoByHost()
{
	if(CSH->isGuest())
		return;

	// Generate header info
	mapInfo = std::make_shared<CMapInfo>();
	mapInfo->isRandomMap = true;
	mapInfo->mapHeader = std::make_unique<CMapHeader>();
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
	groupMapSize->setSelected(vstd::find_pos(getPossibleMapSizes(), opts->getWidth()));
	buttonTwoLevels->setSelected(opts->getHasTwoLevels());
	groupMaxPlayers->setSelected(opts->getPlayerCount());
	groupMaxTeams->setSelected(opts->getTeamCount());
	groupCompOnlyPlayers->setSelected(opts->getCompOnlyPlayerCount());
	groupCompOnlyTeams->setSelected(opts->getCompOnlyTeamCount());
	groupWaterContent->setSelected(opts->getWaterContent());
	groupMonsterStrength->setSelected(opts->getMonsterStrength());
}

void RandomMapTab::addButtonsWithRandToGroup(CToggleGroup * group, const std::vector<std::string> & defs, int nStart, int nEnd, int btnWidth, int helpStartIndex, int helpRandIndex, int randIndex, bool animIdfromBtnId) const
{
	addButtonsToGroup(group, defs, nStart, nEnd, btnWidth, helpStartIndex, animIdfromBtnId);

	// Buttons are relative to button group, TODO better solution?
	SObjectConstruction obj__i(group);
	const std::string RANDOM_DEF = "RANRAND";
	group->addToggle(randIndex, std::make_shared<CToggleButton>(Point(256, 0), RANDOM_DEF, CGI->generaltexth->zelp[helpRandIndex]));
	group->setSelected(randIndex);
}

void RandomMapTab::addButtonsToGroup(CToggleGroup * group, const std::vector<std::string> & defs, int nStart, int nEnd, int btnWidth, int helpStartIndex, bool animIdfromBtnId) const
{
	// Buttons are relative to button group, TODO better solution?
	SObjectConstruction obj__i(group);
	int cnt = nEnd - nStart + 1;
	for(int i = 0; i < cnt; ++i)
	{
		auto button = std::make_shared<CToggleButton>(Point(i * btnWidth, 0), animIdfromBtnId ? defs[i + nStart] : defs[i], CGI->generaltexth->zelp[helpStartIndex + i]);
		// For blocked state we should use pressed image actually
		button->setImageOrder(0, 1, 1, 3);
		group->addToggle(i + nStart, button);
	}
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
		groupMaxTeams->setSelected(mapGenOptions->getTeamCount());
	}
	// total players should not exceed PlayerColor::PLAYER_LIMIT_I (8 in homm3)
	if(mapGenOptions->getCompOnlyPlayerCount() + playersCnt > PlayerColor::PLAYER_LIMIT_I)
	{
		mapGenOptions->setCompOnlyPlayerCount(PlayerColor::PLAYER_LIMIT_I - playersCnt);
		groupCompOnlyPlayers->setSelected(mapGenOptions->getCompOnlyPlayerCount());
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
		groupCompOnlyTeams->setSelected(compOnlyTeamCount);
	}
}

std::vector<int> RandomMapTab::getPossibleMapSizes()
{
	return {CMapHeader::MAP_SIZE_SMALL, CMapHeader::MAP_SIZE_MIDDLE, CMapHeader::MAP_SIZE_LARGE, CMapHeader::MAP_SIZE_XLARGE};
}
