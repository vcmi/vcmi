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

RandomMapTab::RandomMapTab()
{
	mapGenOptions = std::make_shared<CMapGenOptions>();
	OBJ_CONSTRUCTION;
	bg = new CPicture("RANMAPBK", 0, 6);

	// Map Size
	mapSizeBtnGroup = new CToggleGroup(0);
	mapSizeBtnGroup->pos.y += 81;
	mapSizeBtnGroup->pos.x += 158;
	const std::vector<std::string> mapSizeBtns = {"RANSIZS", "RANSIZM", "RANSIZL", "RANSIZX"};
	addButtonsToGroup(mapSizeBtnGroup, mapSizeBtns, 0, 3, 47, 198);
	mapSizeBtnGroup->setSelected(1);
	mapSizeBtnGroup->addCallback([&](int btnId)
	{
		auto mapSizeVal = getPossibleMapSizes();
		mapGenOptions->setWidth(mapSizeVal[btnId]);
		mapGenOptions->setHeight(mapSizeVal[btnId]);
		updateMapInfoByHost();
	});

	// Two levels
	twoLevelsBtn = new CToggleButton(Point(346, 81), "RANUNDR", CGI->generaltexth->zelp[202]);
	//twoLevelsBtn->select(true); for now, deactivated
	twoLevelsBtn->addCallback([&](bool on)
	{
		mapGenOptions->setHasTwoLevels(on);
		updateMapInfoByHost();
	});

	// Create number defs list
	std::vector<std::string> numberDefs;
	for(int i = 0; i <= 8; ++i)
	{
		numberDefs.push_back("RANNUM" + boost::lexical_cast<std::string>(i));
	}

	const int NUMBERS_WIDTH = 32;
	const int BTNS_GROUP_LEFT_MARGIN = 67;
	// Amount of players
	playersCntGroup = new CToggleGroup(0);
	playersCntGroup->pos.y += 153;
	playersCntGroup->pos.x += BTNS_GROUP_LEFT_MARGIN;
	addButtonsWithRandToGroup(playersCntGroup, numberDefs, 1, 8, NUMBERS_WIDTH, 204, 212);
	playersCntGroup->addCallback([&](int btnId)
	{
		mapGenOptions->setPlayerCount(btnId);
		deactivateButtonsFrom(teamsCntGroup, btnId);
		deactivateButtonsFrom(compOnlyPlayersCntGroup, btnId);
		validatePlayersCnt(btnId);
		updateMapInfoByHost();
	});

	// Amount of teams
	teamsCntGroup = new CToggleGroup(0);
	teamsCntGroup->pos.y += 219;
	teamsCntGroup->pos.x += BTNS_GROUP_LEFT_MARGIN;
	addButtonsWithRandToGroup(teamsCntGroup, numberDefs, 0, 7, NUMBERS_WIDTH, 214, 222);
	teamsCntGroup->addCallback([&](int btnId)
	{
		mapGenOptions->setTeamCount(btnId);
		updateMapInfoByHost();
	});

	// Computer only players
	compOnlyPlayersCntGroup = new CToggleGroup(0);
	compOnlyPlayersCntGroup->pos.y += 285;
	compOnlyPlayersCntGroup->pos.x += BTNS_GROUP_LEFT_MARGIN;
	addButtonsWithRandToGroup(compOnlyPlayersCntGroup, numberDefs, 0, 7, NUMBERS_WIDTH, 224, 232);
	compOnlyPlayersCntGroup->addCallback([&](int btnId)
	{
		mapGenOptions->setCompOnlyPlayerCount(btnId);
		deactivateButtonsFrom(compOnlyTeamsCntGroup, (btnId == 0 ? 1 : btnId));
		validateCompOnlyPlayersCnt(btnId);
		updateMapInfoByHost();
	});

	// Computer only teams
	compOnlyTeamsCntGroup = new CToggleGroup(0);
	compOnlyTeamsCntGroup->pos.y += 351;
	compOnlyTeamsCntGroup->pos.x += BTNS_GROUP_LEFT_MARGIN;
	addButtonsWithRandToGroup(compOnlyTeamsCntGroup, numberDefs, 0, 6, NUMBERS_WIDTH, 234, 241);
	deactivateButtonsFrom(compOnlyTeamsCntGroup, 1);
	compOnlyTeamsCntGroup->addCallback([&](int btnId)
	{
		mapGenOptions->setCompOnlyTeamCount(btnId);
		updateMapInfoByHost();
	});

	const int WIDE_BTN_WIDTH = 85;
	// Water content
	waterContentGroup = new CToggleGroup(0);
	waterContentGroup->pos.y += 419;
	waterContentGroup->pos.x += BTNS_GROUP_LEFT_MARGIN;
	const std::vector<std::string> waterContentBtns = {"RANNONE", "RANNORM", "RANISLD"};
	addButtonsWithRandToGroup(waterContentGroup, waterContentBtns, 0, 2, WIDE_BTN_WIDTH, 243, 246);
	waterContentGroup->addCallback([&](int btnId)
	{
		mapGenOptions->setWaterContent(static_cast<EWaterContent::EWaterContent>(btnId));
		updateMapInfoByHost();
	});

	// Monster strength
	monsterStrengthGroup = new CToggleGroup(0);
	monsterStrengthGroup->pos.y += 485;
	monsterStrengthGroup->pos.x += BTNS_GROUP_LEFT_MARGIN;
	const std::vector<std::string> monsterStrengthBtns = {"RANWEAK", "RANNORM", "RANSTRG"};
	addButtonsWithRandToGroup(monsterStrengthGroup, monsterStrengthBtns, 0, 2, WIDE_BTN_WIDTH, 248, 251);
	monsterStrengthGroup->addCallback([&](int btnId)
	{
		if(btnId < 0)
			mapGenOptions->setMonsterStrength(EMonsterStrength::RANDOM);
		else
			mapGenOptions->setMonsterStrength(static_cast<EMonsterStrength::EMonsterStrength>(btnId + EMonsterStrength::GLOBAL_WEAK)); //value 2 to 4
		updateMapInfoByHost();
	});

	// Show random maps btn
	showRandMaps = new CButton(Point(54, 535), "RANSHOW", CGI->generaltexth->zelp[252]);

	// Initialize map info object
	updateMapInfoByHost();
}

void RandomMapTab::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	// Headline
	printAtMiddleLoc(CGI->generaltexth->allTexts[738], 222, 36, FONT_BIG, Colors::YELLOW, to);
	printAtMiddleLoc(CGI->generaltexth->allTexts[739], 222, 56, FONT_SMALL, Colors::WHITE, to);

	// Map size
	printAtMiddleLoc(CGI->generaltexth->allTexts[752], 104, 97, FONT_SMALL, Colors::WHITE, to);

	// Players cnt
	printAtLoc(CGI->generaltexth->allTexts[753], 68, 133, FONT_SMALL, Colors::WHITE, to);

	// Teams cnt
	printAtLoc(CGI->generaltexth->allTexts[754], 68, 199, FONT_SMALL, Colors::WHITE, to);

	// Computer only players cnt
	printAtLoc(CGI->generaltexth->allTexts[755], 68, 265, FONT_SMALL, Colors::WHITE, to);

	// Computer only teams cnt
	printAtLoc(CGI->generaltexth->allTexts[756], 68, 331, FONT_SMALL, Colors::WHITE, to);

	// Water content
	printAtLoc(CGI->generaltexth->allTexts[757], 68, 398, FONT_SMALL, Colors::WHITE, to);

	// Monster strength
	printAtLoc(CGI->generaltexth->allTexts[758], 68, 465, FONT_SMALL, Colors::WHITE, to);
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
		playersToGen = mapGenOptions->getPlayerCount();
	mapInfo->mapHeader->howManyTeams = playersToGen;

	for(int i = 0; i < playersToGen; ++i)
	{
		PlayerInfo player;
		player.isFactionRandom = true;
		player.canComputerPlay = true;
		if(mapGenOptions->getCompOnlyPlayerCount() != CMapGenOptions::RANDOM_SIZE && i >= mapGenOptions->getHumanOnlyPlayerCount())
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
	mapSizeBtnGroup->setSelected(vstd::find_pos(getPossibleMapSizes(), opts->getWidth()));
	twoLevelsBtn->setSelected(opts->getHasTwoLevels());
	playersCntGroup->setSelected(opts->getPlayerCount());
	teamsCntGroup->setSelected(opts->getTeamCount());
	compOnlyPlayersCntGroup->setSelected(opts->getCompOnlyPlayerCount());
	compOnlyTeamsCntGroup->setSelected(opts->getCompOnlyTeamCount());
	waterContentGroup->setSelected(opts->getWaterContent());
	monsterStrengthGroup->setSelected(opts->getMonsterStrength());
}

void RandomMapTab::addButtonsWithRandToGroup(CToggleGroup * group, const std::vector<std::string> & defs, int nStart, int nEnd, int btnWidth, int helpStartIndex, int helpRandIndex) const
{
	addButtonsToGroup(group, defs, nStart, nEnd, btnWidth, helpStartIndex);

	// Buttons are relative to button group, TODO better solution?
	SObjectConstruction obj__i(group);
	const std::string RANDOM_DEF = "RANRAND";
	group->addToggle(CMapGenOptions::RANDOM_SIZE, new CToggleButton(Point(256, 0), RANDOM_DEF, CGI->generaltexth->zelp[helpRandIndex]));
	group->setSelected(CMapGenOptions::RANDOM_SIZE);
}

void RandomMapTab::addButtonsToGroup(CToggleGroup * group, const std::vector<std::string> & defs, int nStart, int nEnd, int btnWidth, int helpStartIndex) const
{
	// Buttons are relative to button group, TODO better solution?
	SObjectConstruction obj__i(group);
	int cnt = nEnd - nStart + 1;
	for(int i = 0; i < cnt; ++i)
	{
		auto button = new CToggleButton(Point(i * btnWidth, 0), defs[i + nStart], CGI->generaltexth->zelp[helpStartIndex + i]);
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
		if(auto button = dynamic_cast<CToggleButton *>(toggle.second))
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
		teamsCntGroup->setSelected(mapGenOptions->getTeamCount());
	}
	if(mapGenOptions->getCompOnlyPlayerCount() >= playersCnt)
	{
		mapGenOptions->setCompOnlyPlayerCount(playersCnt - 1);
		compOnlyPlayersCntGroup->setSelected(mapGenOptions->getCompOnlyPlayerCount());
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
		compOnlyTeamsCntGroup->setSelected(compOnlyTeamCount);
	}
}

std::vector<int> RandomMapTab::getPossibleMapSizes()
{
	return {CMapHeader::MAP_SIZE_SMALL, CMapHeader::MAP_SIZE_MIDDLE, CMapHeader::MAP_SIZE_LARGE, CMapHeader::MAP_SIZE_XLARGE};
}
