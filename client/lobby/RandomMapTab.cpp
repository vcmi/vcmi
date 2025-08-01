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
#include "CLobbyScreen.h"
#include "SelectionTab.h"

#include "../CServerHandler.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../gui/MouseButton.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CComponent.h"
#include "../widgets/ComboBox.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../windows/GUIClasses.h"
#include "../windows/InfoWindows.h"

#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/mapping/CMapHeader.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/rmg/CMapGenOptions.h"
#include "../../lib/rmg/CRmgTemplateStorage.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/RoadHandler.h"

#include "../../lib/CConfigHandler.h"
#include "../../lib/serializer/JsonSerializer.h"
#include "../../lib/serializer/JsonDeserializer.h"

RandomMapTab::RandomMapTab():
	InterfaceObjectConfigurable()
{
	recActions = 0;
	mapGenOptions = std::make_shared<CMapGenOptions>();
	
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
		mapGenOptions->setHumanOrCpuPlayerCount(btnId);
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
	addCallback("teamAlignments", [&](int)
	{
		ENGINE->windows().createAndPushWindow<TeamAlignments>(*this);
	});
	
	for(const auto & road : LIBRARY->roadTypeHandler->objects)
	{
		std::string cbRoadType = "selectRoad_" + road->getJsonKey();
		addCallback(cbRoadType, [&, roadID = road->getId()](bool on)
		{
			mapGenOptions->setRoadEnabled(roadID, on);
			updateMapInfoByHost();
		});
	}
	
	const JsonNode config(JsonPath::builtin("config/widgets/randomMapTab.json"));
	build(config);
	
	if(auto w = widget<CButton>("buttonShowRandomMaps"))
	{
		w->addCallback([&]()
		{
			(static_cast<CLobbyScreen *>(parent))->toggleTab((static_cast<CLobbyScreen *>(parent))->tabSel);
			(static_cast<CLobbyScreen *>(parent))->tabSel->showRandom = true;
			(static_cast<CLobbyScreen *>(parent))->tabSel->filter(0, true);
		});
	}

	//set combo box callbacks
	if(auto w = widget<ComboBox>("templateList"))
	{
		w->onConstructItems = [](std::vector<const void *> & curItems){
			auto templates = LIBRARY->tplh->getTemplates();
		
			boost::range::sort(templates, [](const CRmgTemplate * a, const CRmgTemplate * b){
				return a->getName() < b->getName();
			});

			curItems.push_back(nullptr); //default template
			
			for(auto & t : templates)
				curItems.push_back(t);
		};
		
		w->onSetItem = [&](const void * item){
			this->setTemplate(reinterpret_cast<const CRmgTemplate *>(item));
		};
		
		w->getItemText = [this](int idx, const void * item){
			if(item)
				return reinterpret_cast<const CRmgTemplate *>(item)->getName();
			if(idx == 0)
				return readText(variables["randomTemplate"]);
			return std::string("");
		};
	}
	
	loadOptions();
}

void RandomMapTab::updateMapInfoByHost()
{
	if(GAME->server().isGuest())
		return;

	// Generate header info
	mapInfo = std::make_shared<CMapInfo>();
	mapInfo->isRandomMap = true;
	mapInfo->mapHeader = std::make_unique<CMapHeader>();
	mapInfo->mapHeader->version = EMapFormat::VCMI;
	mapInfo->mapHeader->name.appendLocalString(EMetaText::GENERAL_TXT, 740);
	mapInfo->mapHeader->description.appendLocalString(EMetaText::GENERAL_TXT, 741);

	if(mapGenOptions->getWaterContent() != EWaterContent::RANDOM)
		mapInfo->mapHeader->banWaterHeroes(mapGenOptions->getWaterContent() != EWaterContent::NONE);

	const auto * temp = mapGenOptions->getMapTemplate();
	if (temp)
	{
		auto randomTemplateDescription = temp->getDescription();
		if (!randomTemplateDescription.empty())
		{
			auto description = std::string("\n\n") + randomTemplateDescription;
			mapInfo->mapHeader->description.appendRawString(description);
		}

		for (const auto & hero : temp->getBannedHeroes())
			mapInfo->mapHeader->allowedHeroes.erase(hero);
	}

	mapInfo->mapHeader->difficulty = EMapDifficulty::NORMAL;
	mapInfo->mapHeader->height = mapGenOptions->getHeight();
	mapInfo->mapHeader->width = mapGenOptions->getWidth();
	mapInfo->mapHeader->twoLevel = mapGenOptions->getHasTwoLevels();

	// Generate player information
	int playersToGen = mapGenOptions->getMaxPlayersCount();

	mapInfo->mapHeader->howManyTeams = playersToGen;

	//TODO: Assign all human-controlled colors in first place

	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		mapInfo->mapHeader->players[i].canComputerPlay = false;
		mapInfo->mapHeader->players[i].canHumanPlay = false;
	}

	std::vector<PlayerColor> availableColors;
	for (ui8 color = 0; color < PlayerColor::PLAYER_LIMIT_I; color++)
	{
		availableColors.push_back(PlayerColor(color));
	}

	//First restore known players
	for (auto& player : mapGenOptions->getPlayersSettings())
	{
		PlayerInfo playerInfo;
		playerInfo.isFactionRandom = (player.second.getStartingTown() == FactionID::RANDOM);
		playerInfo.canComputerPlay = (player.second.getPlayerType() != EPlayerType::HUMAN);
		playerInfo.canHumanPlay = (player.second.getPlayerType() != EPlayerType::COMP_ONLY);

		auto team = player.second.getTeam();
		playerInfo.team = team;
		playerInfo.hasMainTown = true;
		playerInfo.generateHeroAtMainTown = true;
		mapInfo->mapHeader->players[player.first.getNum()] = playerInfo;
		vstd::erase(availableColors, player.first);
	}

	mapInfoChanged(mapInfo, mapGenOptions);
}

void RandomMapTab::setMapGenOptions(std::shared_ptr<CMapGenOptions> opts)
{
	mapGenOptions = opts;
	
	//Prepare allowed options - add all, then erase the ones above the limit
	for(int i = 0; i <= PlayerColor::PLAYER_LIMIT_I; ++i)
	{
		playerCountAllowed.insert(i);
		compCountAllowed.insert(i);
		if (i >= 2)
		{
			playerTeamsAllowed.insert(i);
		}
		if (i >= 1)
		{
			compTeamsAllowed.insert(i);
		}
	}
	std::set<int> humanCountAllowed;

	auto * tmpl = mapGenOptions->getMapTemplate();
	if(tmpl)
	{
		playerCountAllowed = tmpl->getPlayers().getNumbers();
		humanCountAllowed = tmpl->getHumanPlayers().getNumbers(); // Unused now?
	}
	
	si8 playerLimit = opts->getPlayerLimit();
	si8 humanOrCpuPlayerCount = opts->getHumanOrCpuPlayerCount();
	si8 compOnlyPlayersCount = opts->getCompOnlyPlayerCount();

	if(humanOrCpuPlayerCount != CMapGenOptions::RANDOM_SIZE)
	{
		vstd::erase_if(compCountAllowed, [playerLimit, humanOrCpuPlayerCount](int el)
		{
			return (playerLimit - humanOrCpuPlayerCount) < el;
		});
		vstd::erase_if(playerTeamsAllowed, [humanOrCpuPlayerCount](int el)
		{
			return humanOrCpuPlayerCount <= el;
		});
	}
	else // Random
	{
		vstd::erase_if(compCountAllowed, [playerLimit](int el)
		{
			return (playerLimit - 1) < el; // Must leave at least 1 human player
		});
		vstd::erase_if(playerTeamsAllowed, [playerLimit](int el)
		{
			return playerLimit <= el;
		});
	}
	if(!playerTeamsAllowed.count(opts->getTeamCount()))
	{
		opts->setTeamCount(CMapGenOptions::RANDOM_SIZE);
	}

	if(compOnlyPlayersCount != CMapGenOptions::RANDOM_SIZE)
	{
		// This setting doesn't impact total number of players
		vstd::erase_if(compTeamsAllowed, [compOnlyPlayersCount](int el)
		{
			return compOnlyPlayersCount<= el;
		});
		
		if(!compTeamsAllowed.count(opts->getCompOnlyTeamCount()))
		{
			opts->setCompOnlyTeamCount(CMapGenOptions::RANDOM_SIZE);
		}
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
		w->setSelected(opts->getHumanOrCpuPlayerCount());
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
			w->setTextOverlay(tmpl->getName(), EFonts::FONT_SMALL, Colors::WHITE);
		else
			w->setTextOverlay(readText(variables["randomTemplate"]), EFonts::FONT_SMALL, Colors::WHITE);
	}
	for(const auto & r : LIBRARY->roadTypeHandler->objects)
	{
		// Workaround for vcmi-extras bug
		std::string jsonKey = r->getJsonKey();
		std::string identifier = jsonKey.substr(jsonKey.find(':')+1);

		if(auto w = widget<CToggleButton>(identifier))
		{
			w->setSelected(opts->isRoadEnabled(r->getId()));
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
			w->setTextOverlay(tmpl->getName(), EFonts::FONT_SMALL, Colors::WHITE);
		else
			w->setTextOverlay(readText(variables["randomTemplate"]), EFonts::FONT_SMALL, Colors::WHITE);
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

void TeamAlignmentsWidget::checkTeamCount()
{
	//Do not allow to select one team only
	std::set<TeamID> teams;
	for (int plId = 0; plId < players.size(); ++plId)
	{
		teams.insert(TeamID(players[plId]->getSelected()));
	}
	if (teams.size() < 2)
	{
		//Do not let player close the window
		buttonOk->block(true);
	}
	else
	{
		buttonOk->block(false);
	}
}

TeamAlignments::TeamAlignments(RandomMapTab & randomMapTab)
	: CWindowObject(BORDERED)
{
	OBJECT_CONSTRUCTION;

	widget = std::make_shared<TeamAlignmentsWidget>(randomMapTab);
	pos = widget->pos;

	updateShadow();
	center();
}

TeamAlignmentsWidget::TeamAlignmentsWidget(RandomMapTab & randomMapTab):
	InterfaceObjectConfigurable()
{
	const JsonNode config(JsonPath::builtin("config/widgets/randomMapTeamsWidget.json"));
	variables = config["variables"];
	
	//int totalPlayers = randomMapTab.obtainMapGenOptions().getPlayerLimit();
	int totalPlayers = randomMapTab.obtainMapGenOptions().getMaxPlayersCount();
	assert(totalPlayers <= PlayerColor::PLAYER_LIMIT_I);
	auto playerSettings = randomMapTab.obtainMapGenOptions().getPlayersSettings();
	variables["totalPlayers"].Integer() = totalPlayers;
	
	pos.w = variables["windowSize"]["x"].Integer() + totalPlayers * variables["cellMargin"]["x"].Integer();
	auto widthExtend = std::max(pos.w, 220) - pos.w; // too small for buttons
	pos.w += widthExtend;
	pos.h = variables["windowSize"]["y"].Integer() + totalPlayers * variables["cellMargin"]["y"].Integer();
	variables["backgroundRect"]["x"].Integer() = 0;
	variables["backgroundRect"]["y"].Integer() = 0;
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

		for(auto & window : ENGINE->windows().findWindows<TeamAlignments>())
			ENGINE->windows().popWindow(window);
	});
	
	addCallback("cancel", [&](int)
	{
		for(auto & window : ENGINE->windows().findWindows<TeamAlignments>())
			ENGINE->windows().popWindow(window);
	});
	
	build(config);
	
	center(pos);
	
	OBJECT_CONSTRUCTION;
	
	// Window should have X * X columns, where X is max players allowed for current settings
	// For random player count, X is 8

	if (totalPlayers > playerSettings.size())
	{
		auto savedPlayers = randomMapTab.obtainMapGenOptions().getSavedPlayersMap();
		for (const auto & player : savedPlayers)
		{
			if (!vstd::contains(playerSettings, player.first))
			{
				playerSettings[player.first] = player.second;
			}
		}
	}

	std::vector<CMapGenOptions::CPlayerSettings> settingsVec;
	for (const auto & player : playerSettings)
	{
		settingsVec.push_back(player.second);
	}

	for(int plId = 0; plId < totalPlayers; ++plId)
	{
		players.push_back(std::make_shared<CToggleGroup>([&, totalPlayers, plId](int sel)
		{
			variables["player_id"].Integer() = plId;
			OBJECT_CONSTRUCTION_TARGETED(players[plId].get());
			for(int teamId = 0; teamId < totalPlayers; ++teamId)
			{
				auto button = std::dynamic_pointer_cast<CToggleButton>(players[plId]->buttons[teamId]);
				assert(button);
				if(sel == teamId)
				{
					button->setOverlay(buildWidget(variables["flagsAnimation"]));
				}
				else
				{
					button->setOverlay(nullptr);
				}
				button->addCallback([this](bool)
				{
					checkTeamCount();
				});
			}
		}));
		
		OBJECT_CONSTRUCTION_TARGETED(players.back().get());

		for(int teamId = 0; teamId < totalPlayers; ++teamId)
		{
			variables["point"]["x"].Integer() = variables["cellOffset"]["x"].Integer() + plId * variables["cellMargin"]["x"].Integer() + (widthExtend / 2);
			variables["point"]["y"].Integer() = variables["cellOffset"]["y"].Integer() + teamId * variables["cellMargin"]["y"].Integer();
			auto button = buildWidget(variables["button"]);
			players.back()->addToggle(teamId, std::dynamic_pointer_cast<CToggleBase>(button));
		}
		
		// plId is not necessarily player color, just an index
		auto team = settingsVec.at(plId).getTeam();
		if(team == TeamID::NO_TEAM)
		{
			logGlobal->warn("Player %d (id %d) has uninitialized team", settingsVec.at(plId).getColor(), plId);
			players.back()->setSelected(plId);
		}
		else
			players.back()->setSelected(team.getNum());
	}

	buttonOk = widget<CButton>("buttonOK");
	buttonCancel = widget<CButton>("buttonCancel");
}

void RandomMapTab::saveOptions(const CMapGenOptions & options)
{
	JsonNode data;
	JsonSerializer ser(nullptr, data);

	ser.serializeStruct("lastSettings", const_cast<CMapGenOptions & >(options));

	// FIXME: Do not nest fields
	Settings rmgSettings = persistentStorage.write["rmg"];
	rmgSettings["rmg"] = data;
}

void RandomMapTab::loadOptions()
{
	JsonNode rmgSettings = persistentStorage["rmg"]["rmg"];

	if (!rmgSettings.Struct().empty())
	{
		rmgSettings.setModScope(ModScope::scopeGame());
		mapGenOptions.reset(new CMapGenOptions());
		JsonDeserializer handler(nullptr, rmgSettings);
		handler.serializeStruct("lastSettings", *mapGenOptions);

		// Will check template and set other options as well
		setTemplate(mapGenOptions->getMapTemplate());
		if(auto w = widget<ComboBox>("templateList"))
		{
			w->setItem(mapGenOptions->getMapTemplate());
		}
	} else
	{
		// Default settings
		mapGenOptions->setRoadEnabled(RoadId(Road::DIRT_ROAD), true);
		mapGenOptions->setRoadEnabled(RoadId(Road::GRAVEL_ROAD), true);
		mapGenOptions->setRoadEnabled(RoadId(Road::COBBLESTONE_ROAD), true);
	}
	updateMapInfoByHost();

	// TODO: Save & load difficulty?
}
