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
#include "../gui/Shortcut.h"
#include "../widgets/CComponent.h"
#include "../widgets/ComboBox.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/Slider.h"
#include "../widgets/TextControls.h"
#include "../widgets/GraphicalPrimitiveCanvas.h"
#include "../widgets/CTextInput.h"
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
	InterfaceObjectConfigurable(),
	templateIndex(0)
{
	recActions = 0;
	mapGenOptions = std::make_shared<CMapGenOptions>();
	
	addCallback("toggleMapSize", [this](int btnId)
	{
		onToggleMapSize(btnId);
	});
	addCallback("toggleTwoLevels", [&](bool on)
	{
		mapGenOptions->setLevels(on ? 2 : 1);
		if(mapGenOptions->getMapTemplate())
			if(!mapGenOptions->getMapTemplate()->matchesSize(int3{mapGenOptions->getWidth(), mapGenOptions->getHeight(), mapGenOptions->getLevels()}))
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
		auto getTemplates = [](){
			auto templates = LIBRARY->tplh->getTemplates();
			boost::range::sort(templates, [](const CRmgTemplate * a, const CRmgTemplate * b){
				return a->getName() < b->getName();
			});
			return templates;
		};

		w->onConstructItems = [getTemplates](std::vector<const void *> & curItems){
			curItems.push_back(nullptr); //default template
			
			for(auto & t : getTemplates())
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

		w->addCallback([this, getTemplates]() // no real dropdown... - instead open dialog
		{
			std::vector<std::string> texts;
			std::vector<std::string> popupTexts;
			texts.push_back(readText(variables["randomTemplate"]));
			popupTexts.push_back("");

			auto selectedTemplate = mapGenOptions->getMapTemplate();
			const auto& templates = getTemplates();
			for(int i = 0; i < templates.size(); i++)
			{
				if(selectedTemplate)
				{
					if(templates[i]->getId() == selectedTemplate->getId())
						templateIndex = i + 1;
				}
				else
					templateIndex = 0;

				texts.push_back(templates[i]->getName());
				popupTexts.push_back("{" + templates[i]->getName() + "}" + (templates[i]->getDescription().empty() ? "" : "\n\n") + templates[i]->getDescription());
			}

			ENGINE->windows().popWindows(1);
			auto window = std::make_shared<CObjectListWindow>(texts, nullptr, LIBRARY->generaltexth->translate("vcmi.lobby.templatesSelect.hover"), LIBRARY->generaltexth->translate("vcmi.lobby.templatesSelect.help"), [this](int index){
				widget<ComboBox>("templateList")->setItem(index);
			}, templateIndex, std::vector<std::shared_ptr<IImage>>(), true, true);
			window->onPopup = [popupTexts](int index){
				if(!popupTexts[index].empty())
					CRClickPopup::createAndPush(popupTexts[index]);
			};
			ENGINE->windows().pushWindow(window);
		});
	}
	
	loadOptions();
}

void RandomMapTab::onToggleMapSize(int btnId)
{
	if(btnId == -1)
		return;

	auto mapSizeVal = getStandardMapSizes();

	auto setTemplateForSize = [this](){
		if(mapGenOptions->getMapTemplate())
			if(!mapGenOptions->getMapTemplate()->matchesSize(int3{mapGenOptions->getWidth(), mapGenOptions->getHeight(), mapGenOptions->getLevels()}))
				setTemplate(nullptr);
		updateMapInfoByHost();
	};

	if(btnId == mapSizeVal.size() - 1)
	{
		ENGINE->windows().createAndPushWindow<SetSizeWindow>(int3(mapGenOptions->getWidth(), mapGenOptions->getHeight(), mapGenOptions->getLevels()), mapGenOptions->getMapTemplate(), [this, setTemplateForSize](int3 ret){
			if(ret.z > 2)
			{
				std::shared_ptr<CInfoWindow> temp = CInfoWindow::create(LIBRARY->generaltexth->translate("vcmi.lobby.customRmgSize.experimental"), PlayerColor(0), {}); //TODO: multilevel support
				ENGINE->windows().pushWindow(temp);
			}
			mapGenOptions->setWidth(ret.x);
			mapGenOptions->setHeight(ret.y);
			mapGenOptions->setLevels(ret.z);
			setTemplateForSize();
		});
		return;
	}

	mapGenOptions->setWidth(mapSizeVal[btnId]);
	mapGenOptions->setHeight(mapSizeVal[btnId]);
	setTemplateForSize();
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

		for (const auto & hero : temp->getEnabledHeroes())
			mapInfo->mapHeader->allowedHeroes.insert(hero);
	}

	mapInfo->mapHeader->difficulty = EMapDifficulty::NORMAL;
	mapInfo->mapHeader->height = mapGenOptions->getHeight();
	mapInfo->mapHeader->width = mapGenOptions->getWidth();
	mapInfo->mapHeader->mapLevels = mapGenOptions->getLevels();

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
		const auto & mapSizes = getStandardMapSizes();
		for(auto toggle : w->buttons)
		{
			if(auto button = std::dynamic_pointer_cast<CToggleButton>(toggle.second))
			{
				int3 size( mapSizes[toggle.first], mapSizes[toggle.first], mapGenOptions->getLevels());

				bool sizeAllowed = !mapGenOptions->getMapTemplate() || mapGenOptions->getMapTemplate()->matchesSize(size);
				button->block(!sizeAllowed && !(toggle.first == mapSizes.size() - 1));
			}
		}
		auto position = vstd::find_pos(getStandardMapSizes(), opts->getWidth());
		w->setSelected(position == mapSizes.size() - 1 || opts->getWidth() != opts->getHeight() ? -1 : position);
	}
	if(auto w = widget<CToggleButton>("buttonTwoLevels"))
	{
		int possibleLevelCount = 2;
		if(mapGenOptions->getMapTemplate())
		{
			auto sizes = mapGenOptions->getMapTemplate()->getMapSizes();
			possibleLevelCount = sizes.second.z - sizes.first.z + 1;
		}
		w->setSelectedSilent(opts->getLevels() == 2);
		w->block(possibleLevelCount < 2);
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

	if(auto w = widget<ComboBox>("templateList"))
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

std::vector<int> RandomMapTab::getStandardMapSizes()
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

SetSizeWindow::SetSizeWindow(int3 initSize, const CRmgTemplate * mapTemplate, std::function<void(int3)> cb)
	: CWindowObject(BORDERED)
{
	OBJECT_CONSTRUCTION;

	pos.w = 300;
	pos.h = 180;

	updateShadow();
	center();

	background = std::make_shared<FilledTexturePlayerColored>(Rect(0, 0, pos.w, pos.h));
	background->setPlayerColor(PlayerColor(1));
	buttonCancel = std::make_shared<CButton>(Point(160, 140), AnimationPath::builtin("MuBcanc"), CButton::tooltip(), [this, cb](){ close();}, EShortcut::GLOBAL_CANCEL);
	buttonOk = std::make_shared<CButton>(Point(70, 140), AnimationPath::builtin("MuBchck"), CButton::tooltip(), [this, cb](){
		close();
		if(cb)
			cb(int3(std::max(1, std::stoi(numInputs[0]->getText())), std::max(1, std::stoi(numInputs[1]->getText())), std::max(1, std::stoi(numInputs[2]->getText()))));
	}, EShortcut::GLOBAL_ACCEPT);

	int3 minSize = mapTemplate ? mapTemplate->getMapSizes().first : int3{36,36,1};
	int3 maxSize = mapTemplate ? mapTemplate->getMapSizes().second : int3{999,999,9};

	MetaString minSizeString;
	MetaString maxSizeString;
	minSizeString.appendTextID("vcmi.randomMapTab.template.minimalSize");
	maxSizeString.appendTextID("vcmi.randomMapTab.template.maximalSize");

	minSizeString.replaceNumber(minSize.x);
	minSizeString.replaceNumber(minSize.y);
	minSizeString.replaceNumber(minSize.z);
	maxSizeString.replaceNumber(maxSize.x);
	maxSizeString.replaceNumber(maxSize.y);
	maxSizeString.replaceNumber(maxSize.z);

	if (mapTemplate)
	{
		MetaString templateTitle;
		templateTitle.appendRawString("%s: %s");
		templateTitle.replaceTextID("vcmi.randomMapTab.widgets.templateLabel");
		templateTitle.replaceRawString(mapTemplate->getName());

		titles.push_back(std::make_shared<CLabel>(10, 40, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, templateTitle.toString()));
	}

	sizeLabels.push_back(std::make_shared<CLabel>(10, 60, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, minSizeString.toString()));
	sizeLabels.push_back(std::make_shared<CLabel>(10, 80, FONT_SMALL, ETextAlignment::CENTERLEFT, Colors::WHITE, maxSizeString.toString()));

	const auto checkTemplateSize = [this, mapTemplate](const std::string &){
		int3 mapSize {
			std::stoi(numInputs[0]->getText()),
			std::stoi(numInputs[1]->getText()),
			std::stoi(numInputs[2]->getText())
		};

		bool isSizeLegal = mapTemplate ? mapTemplate->matchesSize(mapSize) : (mapSize.x * mapSize.y >= 36*36);

		buttonOk->block(!isSizeLegal);

		auto labelColors = isSizeLegal ? Colors::WHITE : Colors::RED;

		for (const auto & label : sizeLabels)
			label->setColor(labelColors);
	};

	titles.push_back(std::make_shared<CLabel>(150, 15, FONT_BIG, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->translate("vcmi.lobby.customRmgSize.title")));

	for(int i = 0; i < 3; i++)
	{
		Rect r(10 + i * 100, 110, 80, 20);
		rectangles.push_back(std::make_shared<TransparentFilledRectangle>(Rect(r.topLeft() - Point(0,18), r.dimensions()), ColorRGBA(0, 0, 0, 64), ColorRGBA(64, 64, 64, 64), 1));
		rectangles.push_back(std::make_shared<TransparentFilledRectangle>(r, ColorRGBA(0, 0, 0, 128), ColorRGBA(64, 64, 64, 64), 1));
		titles.push_back(std::make_shared<CLabel>(50 + i * 100, 100, FONT_SMALL, ETextAlignment::CENTER, Colors::WHITE, LIBRARY->generaltexth->translate("vcmi.lobby.customRmgSize." + std::to_string(i))));
		numInputs.push_back(std::make_shared<CTextInput>(r, EFonts::FONT_SMALL, ETextAlignment::CENTER, false));
		numInputs.back()->setFilterNumber(0, i < 2 ? 999 : 9);
		numInputs.back()->setCallback(checkTemplateSize);
	}
	numInputs[0]->setText(std::to_string(initSize.x));
	numInputs[1]->setText(std::to_string(initSize.y));
	numInputs[2]->setText(std::to_string(initSize.z));
}
