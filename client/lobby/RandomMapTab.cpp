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
	InterfaceObjectConfigurable()
{
	recActions = 0;
	mapGenOptions = std::make_shared<CMapGenOptions>();
	
	const JsonNode config(ResourceID("config/windows/randomMapTab.json"));
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
	
		if(auto w = widget<CToggleGroup>("groupMaxTeams"))
			deactivateButtonsFrom(w.get(), btnId);

		// deactive some CompOnlyPlayers buttons to prevent total number of players exceeds PlayerColor::PLAYER_LIMIT_I
		if(auto w = widget<CToggleGroup>("groupCompOnlyPlayers"))
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
		if(auto w = widget<CToggleGroup>("groupMaxPlayers"))
			deactivateButtonsFrom(w.get(), PlayerColor::PLAYER_LIMIT_I - btnId + 1);
		
		if(auto w = widget<CToggleGroup>("groupCompOnlyTeams"))
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
	
	addCallback("setMonsterStrength", [&](int btnId)
	{
		if(btnId < 0)
			mapGenOptions->setMonsterStrength(EMonsterStrength::RANDOM);
		else
			mapGenOptions->setMonsterStrength(static_cast<EMonsterStrength::EMonsterStrength>(btnId)); //value 2 to 4
		updateMapInfoByHost();
	});
	
	//new callbacks available only from mod
	addCallback("templateSelection", [&](int)
	{
		GH.pushInt(std::shared_ptr<TemplatesDropBox>(new TemplatesDropBox(mapGenOptions)));
	});
	
	
	init(config);
	
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
	if(auto w = widget<CToggleGroup>("groupMapSize"))
		w->setSelected(vstd::find_pos(getPossibleMapSizes(), opts->getWidth()));
	if(auto w = widget<CToggleButton>("buttonTwoLevels"))
		w->setSelected(opts->getHasTwoLevels());
	if(auto w = widget<CToggleGroup>("groupMaxPlayers"))
		w->setSelected(opts->getPlayerCount());
	if(auto w = widget<CToggleGroup>("groupMaxTeams"))
		w->setSelected(opts->getTeamCount());
	if(auto w = widget<CToggleGroup>("groupCompOnlyPlayers"))
		w->setSelected(opts->getCompOnlyPlayerCount());
	if(auto w = widget<CToggleGroup>("groupCompOnlyTeams"))
		w->setSelected(opts->getCompOnlyTeamCount());
	if(auto w = widget<CToggleGroup>("groupWaterContent"))
		w->setSelected(opts->getWaterContent());
	if(auto w = widget<CToggleGroup>("groupMonsterStrength"))
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
		if(auto w = widget<CToggleGroup>("groupMaxTeams"))
			w->setSelected(mapGenOptions->getTeamCount());
	}
	// total players should not exceed PlayerColor::PLAYER_LIMIT_I (8 in homm3)
	if(mapGenOptions->getCompOnlyPlayerCount() + playersCnt > PlayerColor::PLAYER_LIMIT_I)
	{
		mapGenOptions->setCompOnlyPlayerCount(PlayerColor::PLAYER_LIMIT_I - playersCnt);
		if(auto w = widget<CToggleGroup>("groupCompOnlyPlayers"))
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
		if(auto w = widget<CToggleGroup>("groupCompOnlyTeams"))
			w->setSelected(compOnlyTeamCount);
	}
}

std::vector<int> RandomMapTab::getPossibleMapSizes()
{
	return {CMapHeader::MAP_SIZE_SMALL, CMapHeader::MAP_SIZE_MIDDLE, CMapHeader::MAP_SIZE_LARGE, CMapHeader::MAP_SIZE_XLARGE, CMapHeader::MAP_SIZE_HUGE, CMapHeader::MAP_SIZE_XHUGE, CMapHeader::MAP_SIZE_GIANT};
}

TemplatesDropBox::ListItem::ListItem(Point position, const std::string & text)
	: CIntObject(LCLICK | HOVER, position)
{
	OBJ_CONSTRUCTION;
	labelName = std::make_shared<CLabel>(0, 0, FONT_SMALL, EAlignment::TOPLEFT, Colors::WHITE, text);
	labelName->setAutoRedraw(false);
	
	hoverImage = std::make_shared<CPicture>("List10Sl", 0, 0);
	hoverImage->visible = false;
	
	pos.w = hoverImage->pos.w;
	pos.h = hoverImage->pos.h;
	type |= REDRAW_PARENT;
}

void TemplatesDropBox::ListItem::hover(bool on)
{
	hoverImage->visible = on;
	redraw();
}

TemplatesDropBox::TemplatesDropBox(std::shared_ptr<CMapGenOptions> options):
	CIntObject(LCLICK | HOVER),
	mapGenOptions(options)
{
	OBJ_CONSTRUCTION;
	background = std::make_shared<CPicture>("List10Bk", 158, 76);
	
	int positionsToShow = 10;
	
	for(int i = 0; i < positionsToShow; i++)
		listItems.push_back(std::make_shared<ListItem>(Point(158, 76 + i * 25), "test" + std::to_string(i)));
	
	slider = std::make_shared<CSlider>(Point(212 + 158, 76), 252, std::bind(&TemplatesDropBox::sliderMove, this, _1), positionsToShow, 20, 0, false, CSlider::BLUE);
	
	pos = background->pos;
}

void TemplatesDropBox::sliderMove(int slidPos)
{
	if(!slider)
		return; // ignore spurious call when slider is being created
	//updateListItems();
	redraw();
}

void TemplatesDropBox::hover(bool on)
{
	hovered = on;
}

void TemplatesDropBox::clickLeft(tribool down, bool previousState)
{
	if(!hovered)
	{
		assert(GH.topInt().get() == this);
		GH.popInt(GH.topInt());
	}
}
