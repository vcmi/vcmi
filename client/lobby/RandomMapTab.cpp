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
#include "../../lib/rmg/CRmgTemplateStorage.h"

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
		GH.pushIntT<TemplatesDropBox>(*this, int3{mapGenOptions->getWidth(), mapGenOptions->getHeight(), 1 + mapGenOptions->getHasTwoLevels()});
	});
	
	addCallback("teamAlignments", [&](int)
	{
		//TODO: support team alignments
		//GH.pushIntT<TeamAlignmentsWidget>(*this);
	});
	
	for(auto road : VLC->terrainTypeHandler->roads())
	{
		std::string cbRoadType = "selectRoad_" + road.fileName;
		addCallback(cbRoadType, [&](bool on)
		{
			//TODO: support road types
		});
	}
	
	
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
		w->setSelected(vstd::find_pos(getPossibleMapSizes(), opts->getWidth()));
	if(auto w = widget<CToggleButton>("buttonTwoLevels"))
		w->setSelected(opts->getHasTwoLevels());
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
			w->addTextOverlay("default", EFonts::FONT_SMALL);
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
	
	init(config);
	
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
				w->setText("default");
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
		{
			hovered = false;
			h->visible = false;
		}
		else
		{
			h->visible = on;
		}
	}
	redraw();
}

void TemplatesDropBox::ListItem::clickLeft(tribool down, bool previousState)
{
	if(down && hovered)
	{
		dropBox.setTemplate(item);
	}
}


TemplatesDropBox::TemplatesDropBox(RandomMapTab & randomMapTab, int3 size):
	InterfaceObjectConfigurable(LCLICK | HOVER),
	randomMapTab(randomMapTab)
{
	curItems = VLC->tplh->getTemplates();
	vstd::erase_if(curItems, [size](const CRmgTemplate * t){return !t->matchesSize(size);});
	curItems.insert(curItems.begin(), nullptr); //default template
	
	const JsonNode config(ResourceID("config/windows/randomMapTemplateWidget.json"));
	
	addCallback("sliderMove", std::bind(&TemplatesDropBox::sliderMove, this, std::placeholders::_1));
	
	OBJ_CONSTRUCTION;
	pos = randomMapTab.pos.topLeft();
	pos.w = randomMapTab.pos.w;
	pos.h = randomMapTab.pos.h;
	
	init(config);
	
	if(auto w = widget<CSlider>("slider"))
	{
		w->setAmount(curItems.size());
	}
	
	updateListItems();
}

std::shared_ptr<CIntObject> TemplatesDropBox::buildCustomWidget(const JsonNode & config)
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

void TemplatesDropBox::hover(bool on)
{
	hovered = on;
}

void TemplatesDropBox::clickLeft(tribool down, bool previousState)
{
	if(down && !hovered)
	{
		assert(GH.topInt().get() == this);
		GH.popInt(GH.topInt());
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
	assert(GH.topInt().get() == this);
	GH.popInt(GH.topInt());
}
