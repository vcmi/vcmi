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
#include "../gui/CGuiHandler.h"
#include "../gui/MouseButton.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"

#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../../lib/CModHandler.h"
#include "../../lib/rmg/CRmgTemplateStorage.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/RoadHandler.h"

RandomMapTab::RandomMapTab():
	InterfaceObjectConfigurable()
{
	recActions = 0;
	mapGenOptions = std::make_shared<CMapGenOptions>();
	
	const JsonNode config(ResourceID("config/widgets/randomMapTab.json"));
	addCallback("toggleMapSize", [&](int btnId)
	{
		auto mapSizeVal = getPossibleMapSizes();
		mapGenOptions->setWidth(mapSizeVal[btnId]);
		mapGenOptions->setHeight(mapSizeVal[btnId]);
		if(mapGenOptions->getMapTemplate())
			if(!mapGenOptions->getMapTemplate()->matchesSize(int3{mapGenOptions->getWidth(), mapGenOptions->getHeight(), 1 + mapGenOptions->getHasTwoLevels()}))
				setTemplate(nullptr);
		updateMapInfoByHost();
	});
	addCallback("toggleTwoLevels", [&](bool on)
	{
		mapGenOptions->setHasTwoLevels(on);
		if(mapGenOptions->getMapTemplate())
			if(!mapGenOptions->getMapTemplate()->matchesSize(int3{mapGenOptions->getWidth(), mapGenOptions->getHeight(), 1 + mapGenOptions->getHasTwoLevels()}))
				setTemplate(nullptr);
		updateMapInfoByHost();
	});
	
	addCallback("setPlayersCount", [&](int btnId)
	{
		mapGenOptions->setPlayerCount(btnId);
		setMapGenOptions(mapGenOptions);
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
		setMapGenOptions(mapGenOptions);
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
		GH.windows().createAndPushWindow<TemplatesDropBox>(*this, int3{mapGenOptions->getWidth(), mapGenOptions->getHeight(), 1 + mapGenOptions->getHasTwoLevels()});
	});
	
	addCallback("teamAlignments", [&](int)
	{
		GH.windows().createAndPushWindow<TeamAlignmentsWidget>(*this);
	});
	
	for(auto road : VLC->roadTypeHandler->objects)
	{
		std::string cbRoadType = "selectRoad_" + road->getJsonKey();
		addCallback(cbRoadType, [&, road](bool on)
		{
			mapGenOptions->setRoadEnabled(road->getJsonKey(), on);
			updateMapInfoByHost();
		});
	}
	
	
	build(config);
	
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
	mapInfo->mapHeader->version = EMapFormat::VCMI;
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

	std::set<TeamID> occupiedTeams;
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
		auto team = mapGenOptions->getPlayersSettings().at(PlayerColor(i)).getTeam();
		player.team = team;
		occupiedTeams.insert(team);
		player.hasMainTown = true;
		player.generateHeroAtMainTown = true;
		mapInfo->mapHeader->players.push_back(player);
	}
	for(auto & player : mapInfo->mapHeader->players)
	{
		for(int i = 0; player.team == TeamID::NO_TEAM; ++i)
		{
			TeamID team(i);
			if(!occupiedTeams.count(team))
			{
				player.team = team;
				occupiedTeams.insert(team);
			}
		}
	}

	mapInfoChanged(mapInfo, mapGenOptions);
}

void RandomMapTab::setMapGenOptions(std::shared_ptr<CMapGenOptions> opts)
{
	mapGenOptions = opts;
	
	//prepare allowed options
	for(int i = 0; i <= PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		playerCountAllowed.insert(i);
		compCountAllowed.insert(i);
		playerTeamsAllowed.insert(i);
		compTeamsAllowed.insert(i);
	}
	auto * tmpl = mapGenOptions->getMapTemplate();
	if(tmpl)
	{
		playerCountAllowed = tmpl->getPlayers().getNumbers();
		compCountAllowed = tmpl->getCpuPlayers().getNumbers();
	}
	if(mapGenOptions->getPlayerCount() != CMapGenOptions::RANDOM_SIZE)
	{
		vstd::erase_if(compCountAllowed,
		[opts](int el){
			return PlayerColor::PLAYER_LIMIT_I - opts->getPlayerCount() < el;
		});
		vstd::erase_if(playerTeamsAllowed,
		[opts](int el){
			return opts->getPlayerCount() <= el;
		});
		
		if(!playerTeamsAllowed.count(opts->getTeamCount()))
		   opts->setTeamCount(CMapGenOptions::RANDOM_SIZE);
	}
	if(mapGenOptions->getCompOnlyPlayerCount() != CMapGenOptions::RANDOM_SIZE)
	{
		vstd::erase_if(playerCountAllowed,
		[opts](int el){
			return PlayerColor::PLAYER_LIMIT_I - opts->getCompOnlyPlayerCount() < el;
		});
		vstd::erase_if(compTeamsAllowed,
		[opts](int el){
			return opts->getCompOnlyPlayerCount() <= el;
		});
		
		if(!compTeamsAllowed.count(opts->getCompOnlyTeamCount()))
			opts->setCompOnlyTeamCount(CMapGenOptions::RANDOM_SIZE);
	}
	
	if(auto w = widget<CToggleGroup>("groupMapSize"))
	{
		for(auto toggle : w->buttons)
		{
			if(auto button = std::dynamic_pointer_cast<CToggleButton>(toggle.second))
			{
				const auto & mapSizes = getPossibleMapSizes();
				int3 size( mapSizes[toggle.first], mapSizes[toggle.first], 1 + mapGenOptions->getHasTwoLevels());

				bool sizeAllowed = !mapGenOptions->getMapTemplate() || mapGenOptions->getMapTemplate()->matchesSize(size);
				button->block(!sizeAllowed);
			}
		}
		w->setSelected(vstd::find_pos(getPossibleMapSizes(), opts->getWidth()));
	}
	if(auto w = widget<CToggleButton>("buttonTwoLevels"))
	{
		int3 size( opts->getWidth(), opts->getWidth(), 2);

		bool undergoundAllowed = !mapGenOptions->getMapTemplate() || mapGenOptions->getMapTemplate()->matchesSize(size);

		w->setSelected(opts->getHasTwoLevels());
		w->block(!undergoundAllowed);
	}
	if(auto w = widget<CToggleGroup>("groupMaxPlayers"))
	{
		w->setSelected(opts->getPlayerCount());
		deactivateButtonsFrom(*w, playerCountAllowed);
	}
	if(auto w = widget<CToggleGroup>("groupMaxTeams"))
	{
		w->setSelected(opts->getTeamCount());
		deactivateButtonsFrom(*w, playerTeamsAllowed);
	}
	if(auto w = widget<CToggleGroup>("groupCompOnlyPlayers"))
	{
		w->setSelected(opts->getCompOnlyPlayerCount());
		deactivateButtonsFrom(*w, compCountAllowed);
	}
	if(auto w = widget<CToggleGroup>("groupCompOnlyTeams"))
	{
		w->setSelected(opts->getCompOnlyTeamCount());
		deactivateButtonsFrom(*w, compTeamsAllowed);
	}
	if(auto w = widget<CToggleGroup>("groupWaterContent"))
	{
		w->setSelected(opts->getWaterContent());
		if(opts->getMapTemplate())
		{
			std::set<int> allowedWater(opts->getMapTemplate()->getWaterContentAllowed().begin(), opts->getMapTemplate()->getWaterContentAllowed().end());
			deactivateButtonsFrom(*w, allowedWater);
		}
		else
			deactivateButtonsFrom(*w, {-1});
	}
	if(auto w = widget<CToggleGroup>("groupMonsterStrength"))
		w->setSelected(opts->getMonsterStrength());
	if(auto w = widget<CButton>("templateButton"))
	{
		if(tmpl)
			w->addTextOverlay(tmpl->getName(), EFonts::FONT_SMALL);
		else
			w->addTextOverlay(readText(variables["defaultTemplate"]), EFonts::FONT_SMALL);
	}
	for(auto r : VLC->roadTypeHandler->objects)
	{
		if(auto w = widget<CToggleButton>(r->getJsonKey()))
		{
			w->setSelected(opts->isRoadEnabled(r->getJsonKey()));
		}
	}
}

void RandomMapTab::setTemplate(const CRmgTemplate * tmpl)
{
	mapGenOptions->setMapTemplate(tmpl);
	setMapGenOptions(mapGenOptions);
	if(auto w = widget<CButton>("templateButton"))
	{
		if(tmpl)
			w->addTextOverlay(tmpl->getName(), EFonts::FONT_SMALL);
		else
			w->addTextOverlay(readText(variables["defaultTemplate"]), EFonts::FONT_SMALL);
	}
	updateMapInfoByHost();
}

void RandomMapTab::deactivateButtonsFrom(CToggleGroup & group, const std::set<int> & allowed)
{
	logGlobal->debug("Blocking buttons");
	for(auto toggle : group.buttons)
	{
		if(auto button = std::dynamic_pointer_cast<CToggleButton>(toggle.second))
		{
			if(allowed.count(CMapGenOptions::RANDOM_SIZE)
			   || allowed.count(toggle.first)
			   || toggle.first == CMapGenOptions::RANDOM_SIZE)
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

std::vector<int> RandomMapTab::getPossibleMapSizes()
{
	return {CMapHeader::MAP_SIZE_SMALL, CMapHeader::MAP_SIZE_MIDDLE, CMapHeader::MAP_SIZE_LARGE, CMapHeader::MAP_SIZE_XLARGE, CMapHeader::MAP_SIZE_HUGE, CMapHeader::MAP_SIZE_XHUGE, CMapHeader::MAP_SIZE_GIANT};
}

TemplatesDropBox::ListItem::ListItem(const JsonNode & config, TemplatesDropBox & _dropBox, Point position)
	: InterfaceObjectConfigurable(LCLICK | HOVER, position),
	dropBox(_dropBox)
{
	OBJ_CONSTRUCTION;
	
	build(config);
	
	if(auto w = widget<CPicture>("hoverImage"))
	{
		pos.w = w->pos.w;
		pos.h = w->pos.h;
	}
	type |= REDRAW_PARENT;
}

void TemplatesDropBox::ListItem::updateItem(int idx, const CRmgTemplate * _item)
{
	if(auto w = widget<CLabel>("labelName"))
	{
		item = _item;
		if(item)
		{
			w->setText(item->getName());
		}
		else
		{
			if(idx)
				w->setText("");
			else
				w->setText(readText(dropBox.variables["defaultTemplate"]));
		}
	}
}

void TemplatesDropBox::ListItem::hover(bool on)
{
	auto h = widget<CPicture>("hoverImage");
	auto w = widget<CLabel>("labelName");
	if(h && w)
	{
		if(w->getText().empty())
			h->visible = false;
		else
			h->visible = on;
	}
	redraw();
}

void TemplatesDropBox::ListItem::clickLeft(tribool down, bool previousState)
{
	if(down && isHovered())
	{
		dropBox.setTemplate(item);
	}
	else 
	{
		dropBox.clickLeft(true, true);
	}
}


TemplatesDropBox::TemplatesDropBox(RandomMapTab & randomMapTab, int3 size):
	InterfaceObjectConfigurable(LCLICK | HOVER),
	randomMapTab(randomMapTab)
{
	REGISTER_BUILDER("templateListItem", &TemplatesDropBox::buildListItem);
	
	curItems = VLC->tplh->getTemplates();

	boost::range::sort(curItems, [](const CRmgTemplate * a, const CRmgTemplate * b){
		return a->getName() < b->getName();
	});

	curItems.insert(curItems.begin(), nullptr); //default template
	
	const JsonNode config(ResourceID("config/widgets/randomMapTemplateWidget.json"));
	
	addCallback("sliderMove", std::bind(&TemplatesDropBox::sliderMove, this, std::placeholders::_1));
	
	OBJ_CONSTRUCTION;
	pos = randomMapTab.pos;
	
	build(config);
	
	if(auto w = widget<CSlider>("slider"))
	{
		w->setAmount(curItems.size());
	}
	
	updateListItems();
}

std::shared_ptr<CIntObject> TemplatesDropBox::buildListItem(const JsonNode & config)
{
	auto position = readPosition(config["position"]);
	listItems.push_back(std::make_shared<ListItem>(config, *this, position));
	return listItems.back();
}

void TemplatesDropBox::sliderMove(int slidPos)
{
	auto w = widget<CSlider>("slider");
	if(!w)
		return; // ignore spurious call when slider is being created
	updateListItems();
	redraw();
}

void TemplatesDropBox::clickLeft(tribool down, bool previousState)
{
	if(down && !isActive())
	{
		auto w = widget<CSlider>("slider");

		// pop the interface only if the mouse is not clicking on the slider
		if (!w || !w->isMouseButtonPressed(MouseButton::LEFT))
		{
			assert(GH.windows().isTopWindow(this));
			GH.windows().popWindows(1);
		}
	}
}

void TemplatesDropBox::updateListItems()
{
	if(auto w = widget<CSlider>("slider"))
	{
		int elemIdx = w->getValue();
		for(auto item : listItems)
		{
			if(elemIdx < curItems.size())
			{
				item->updateItem(elemIdx, curItems[elemIdx]);
				elemIdx++;
			}
			else
			{
				item->updateItem(elemIdx);
			}
		}
	}
}

void TemplatesDropBox::setTemplate(const CRmgTemplate * tmpl)
{
	randomMapTab.setTemplate(tmpl);
	assert(GH.windows().isTopWindow(this));
	GH.windows().popWindows(1);
}

TeamAlignmentsWidget::TeamAlignmentsWidget(RandomMapTab & randomMapTab):
	InterfaceObjectConfigurable()
{
	const JsonNode config(ResourceID("config/widgets/randomMapTeamsWidget.json"));
	variables = config["variables"];
	
	int humanPlayers = randomMapTab.obtainMapGenOptions().getPlayerCount();
	int cpuPlayers = randomMapTab.obtainMapGenOptions().getCompOnlyPlayerCount();
	int totalPlayers = humanPlayers == CMapGenOptions::RANDOM_SIZE || cpuPlayers == CMapGenOptions::RANDOM_SIZE
	? PlayerColor::PLAYER_LIMIT_I : humanPlayers + cpuPlayers;
	assert(totalPlayers <= PlayerColor::PLAYER_LIMIT_I);
	auto settings = randomMapTab.obtainMapGenOptions().getPlayersSettings();
	variables["totalPlayers"].Integer() = totalPlayers;
	
	pos.w = variables["windowSize"]["x"].Integer() + totalPlayers * variables["cellMargin"]["x"].Integer();
	pos.h = variables["windowSize"]["y"].Integer() + totalPlayers * variables["cellMargin"]["y"].Integer();
	variables["backgroundRect"]["x"].Integer() = pos.x;
	variables["backgroundRect"]["y"].Integer() = pos.y;
	variables["backgroundRect"]["w"].Integer() = pos.w;
	variables["backgroundRect"]["h"].Integer() = pos.h;
	variables["okButtonPosition"]["x"].Integer() = variables["buttonsOffset"]["ok"]["x"].Integer();
	variables["okButtonPosition"]["y"].Integer() = variables["buttonsOffset"]["ok"]["y"].Integer() + totalPlayers * variables["cellMargin"]["y"].Integer();
	variables["cancelButtonPosition"]["x"].Integer() = variables["buttonsOffset"]["cancel"]["x"].Integer();
	variables["cancelButtonPosition"]["y"].Integer() = variables["buttonsOffset"]["cancel"]["y"].Integer() + totalPlayers * variables["cellMargin"]["y"].Integer();
	
	addCallback("ok", [&](int)
	{
		for(int plId = 0; plId < players.size(); ++plId)
		{
			randomMapTab.obtainMapGenOptions().setPlayerTeam(PlayerColor(plId), TeamID(players[plId]->getSelected()));
		}
		randomMapTab.updateMapInfoByHost();
		assert(GH.windows().isTopWindow(this));
		GH.windows().popWindows(1);
	});
	
	addCallback("cancel", [&](int)
	{
		assert(GH.windows().isTopWindow(this));
		GH.windows().popWindows(1);
	});
	
	build(config);
	
	center(pos);
	
	OBJ_CONSTRUCTION;
	
	for(int plId = 0; plId < totalPlayers; ++plId)
	{
		players.push_back(std::make_shared<CToggleGroup>([&, totalPlayers, plId](int sel)
		{
			variables["player_id"].Integer() = plId;
			OBJ_CONSTRUCTION_TARGETED(players[plId].get());
			for(int teamId = 0; teamId < totalPlayers; ++teamId)
			{
				auto button = std::dynamic_pointer_cast<CToggleButton>(players[plId]->buttons[teamId]);
				assert(button);
				if(sel == teamId)
				{
					button->addOverlay(buildWidget(variables["flagsAnimation"]));
				}
				else
				{
					button->addOverlay(nullptr);
				}
			}
		}));
		
		OBJ_CONSTRUCTION_TARGETED(players.back().get());
		for(int teamId = 0; teamId < totalPlayers; ++teamId)
		{
			variables["point"]["x"].Integer() = variables["cellOffset"]["x"].Integer() + plId * variables["cellMargin"]["x"].Integer();
			variables["point"]["y"].Integer() = variables["cellOffset"]["y"].Integer() + teamId * variables["cellMargin"]["y"].Integer();
			auto button = buildWidget(variables["button"]);
			players.back()->addToggle(teamId, std::dynamic_pointer_cast<CToggleBase>(button));
		}
		
		auto team = settings.at(PlayerColor(plId)).getTeam();
		if(team == TeamID::NO_TEAM)
			players.back()->setSelected(plId);
		else
			players.back()->setSelected(team.getNum());
	}
}
