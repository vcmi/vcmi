/*
 * EditorNewMapWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorNewMapWindow.h"

#include "EditorMap.h"
#include "EditorProgressWindow.h"
#include "EditorSettings.h"
#include "EditorTexts.h"

#include "../GameEngine.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CCheckBox.h"
#include "../widgets/CDropDown.h"
#include "../widgets/CFormView.h"
#include "../widgets/CNumberInput.h"
#include "../widgets/CTextButton.h"
#include "../windows/CPromptWindows.h"

#include "../../lib/CRandomGenerator.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/MapLayerHandler.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapEditManager.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/modding/ModScope.h"
#include "../../lib/rmg/CMapGenerator.h"
#include "../../lib/rmg/CRmgTemplate.h"
#include "../../lib/serializer/JsonDeserializer.h"
#include "../../lib/serializer/JsonSerializer.h"

#include <future>

namespace
{
const std::array<const char *, 7> SIZE_NAMES = {"S", "M", "L", "XL", "H", "XH", "G"};
const std::array<int, 7> SIZE_VALUES = {
	CMapHeader::MAP_SIZE_SMALL,
	CMapHeader::MAP_SIZE_MIDDLE,
	CMapHeader::MAP_SIZE_LARGE,
	CMapHeader::MAP_SIZE_XLARGE,
	CMapHeader::MAP_SIZE_HUGE,
	CMapHeader::MAP_SIZE_XHUGE,
	CMapHeader::MAP_SIZE_GIANT
};
constexpr int CUSTOM_SIZE_INDEX = 7;
constexpr int MAX_PLAYERS = PlayerColor::PLAYER_LIMIT_I;

/// Number of the humans or computers for the option at the index of the choice
int playersAt(int index)
{
	return index == 0 ? CMapGenOptions::RANDOM_SIZE : index;
}

/// Number of the computer players or teams for the option at the index of the choice
int computersAt(int index)
{
	return index == 0 ? CMapGenOptions::RANDOM_SIZE : index - 1;
}

std::vector<std::string> countOptions(int first, int last)
{
	std::vector<std::string> result{EditorTexts::get("newMap.random")};
	for(int i = first; i <= last; ++i)
		result.push_back(std::to_string(i));
	return result;
}

std::unique_ptr<CMap> generateEmptyMap(const CMapGenOptions & options)
{
	auto map = std::make_unique<CMap>(nullptr);
	map->version = EMapFormat::VCMI;
	map->creationDateTime = std::time(nullptr);
	map->width = options.getWidth();
	map->height = options.getHeight();
	map->mapLayers = options.getLevelMapLayers();

	map->initTerrain();
	map->getEditManager()->clearTerrain(&CRandomGenerator::getDefault());

	return map;
}

/// Dialog that sets the type of every level
class LayerDialog : public CDialogWindow
{
	std::shared_ptr<CFormView> form;
	std::vector<MapLayerId> layers;

public:
	LayerDialog(const std::vector<MapLayerId> & current)
		: CDialogWindow(Point(420, 340), EditorTexts::get("newMap.layersTitle"))
		, layers(current)
	{
		OBJECT_CONSTRUCTION_TARGETED(content());

		const Point size = contentSize();
		form = std::make_shared<CFormView>(Rect(0, 0, size.x, size.y), 140);

		std::vector<std::string> names;
		for(const auto & layer : LIBRARY->mapLayerHandler->objects)
			names.push_back(layer->getNameTranslated());

		for(size_t level = 0; level < layers.size(); ++level)
		{
			form->addChoice(EditorTexts::format("newMap.layersLevel", {std::to_string(level + 1)}), names, layers[level].getNum(), [this, level](int index)
			{
				layers[level] = MapLayerId(index);
			});
		}
		form->finish();
	}

	const std::vector<MapLayerId> & getLayers() const
	{
		return layers;
	}
};
}

EditorNewMapWindow::EditorNewMapWindow(IGameInfoCallback * callback, const std::function<void(std::unique_ptr<CMap>)> & onCreated)
	: CDialogWindow(Point(620, 640), EditorTexts::get("newMap.title"))
	, options(std::make_shared<CMapGenOptions>())
	, callback(callback)
	, onCreated(onCreated)
{
	build();
	loadSettings();
	updateAvailability();

	onAccept = [this]()
	{
		applyRandomOptions();

		if(randomMap && options->getPossibleTemplates().empty())
		{
			CMessageDialog::showMessage(EditorTexts::get("newMap.noTemplateTitle"), EditorTexts::get("newMap.noTemplateText"));
			return false;
		}

		saveSettings();
		return true;
	};

	afterAccept = [this]() { create(); };
}

void EditorNewMapWindow::build()
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	form = std::make_shared<CFormView>(Rect(0, 0, size.x, size.y), 190);

	form->addHeading(EditorTexts::get("newMap.mapSize"));

	std::vector<std::string> sizeNames;
	for(size_t i = 0; i < SIZE_NAMES.size(); ++i)
		sizeNames.push_back(std::string(SIZE_NAMES[i]) + " (" + std::to_string(SIZE_VALUES[i]) + "x" + std::to_string(SIZE_VALUES[i]) + ")");
	sizeNames.push_back(EditorTexts::get("newMap.customSize"));

	sizeChoice = form->addChoice(EditorTexts::get("newMap.size"), sizeNames, 1, [this](int index) { onSizeChosen(index); });
	widthInput = form->addNumber(EditorTexts::get("newMap.width"), CMapHeader::MAP_SIZE_MIDDLE, 1, 999, [this](int value)
	{
		options->setWidth(value);
		updateTemplateList();
	});
	heightInput = form->addNumber(EditorTexts::get("newMap.height"), CMapHeader::MAP_SIZE_MIDDLE, 1, 999, [this](int value)
	{
		options->setHeight(value);
		updateTemplateList();
	});
	levelsInput = form->addNumber(EditorTexts::get("newMap.levels"), 1, 1, 99, [this](int value) { onLevelsChanged(value); });
	form->addButton("", EditorTexts::get("newMap.layers"), [this]() { showLayerDialog(); });

	randomCheck = form->addCheck(EditorTexts::get("newMap.generateRandom"), false, [this](bool on)
	{
		randomMap = on;
		updateAvailability();
		updateTemplateList();
	});

	form->addHeading(EditorTexts::get("newMap.players"));
	humanChoice = form->addChoice(EditorTexts::get("newMap.humans"), countOptions(1, MAX_PLAYERS), 0, [this](int index) { onHumansChosen(index); });
	cpuChoice = form->addChoice(EditorTexts::get("newMap.computers"), countOptions(0, MAX_PLAYERS - 1), 0, [this](int index) { onComputersChosen(index); });
	humanTeamsChoice = form->addChoice(EditorTexts::get("newMap.humanTeams"), countOptions(0, MAX_PLAYERS - 1), 0, [this](int index) { onHumanTeamsChosen(index); });
	cpuTeamsChoice = form->addChoice(EditorTexts::get("newMap.computerTeams"), countOptions(0, MAX_PLAYERS - 1), 0, [this](int index) { onComputerTeamsChosen(index); });

	form->addHeading(EditorTexts::get("newMap.generation"));
	waterChoice = form->addChoice(EditorTexts::get("newMap.water"),
		{EditorTexts::get("newMap.random"), EditorTexts::get("newMap.waterNone"), EditorTexts::get("newMap.waterNormal"), EditorTexts::get("newMap.waterIslands")}, 0, [](int) {});
	monsterChoice = form->addChoice(EditorTexts::get("newMap.monsters"),
		{EditorTexts::get("newMap.random"), EditorTexts::get("newMap.monstersWeak"), EditorTexts::get("newMap.monstersNormal"), EditorTexts::get("newMap.monstersStrong")}, 0, [](int) {});
	dirtRoad = form->addCheck(EditorTexts::get("newMap.roadDirt"), true, [](bool) {});
	gravelRoad = form->addCheck(EditorTexts::get("newMap.roadGravel"), true, [](bool) {});
	cobblestoneRoad = form->addCheck(EditorTexts::get("newMap.roadCobblestone"), true, [](bool) {});
	templateChoice = form->addChoice(EditorTexts::get("newMap.template"), {EditorTexts::get("newMap.templateDefault")}, 0, [this](int index)
	{
		options->setMapTemplate(index == 0 ? nullptr : templates.at(index - 1));
	});
	seedCheck = form->addCheck(EditorTexts::get("newMap.customSeed"), false, [this](bool)
	{
		updateAvailability();
	});
	seedInput = form->addNumber(EditorTexts::get("newMap.seed"), 1, 1, 999999999, nullptr);

	form->finish();

	options->setWidth(CMapHeader::MAP_SIZE_MIDDLE);
	options->setHeight(CMapHeader::MAP_SIZE_MIDDLE);
	options->setLevels(1);
	options->resetLevelMapLayers();
}

void EditorNewMapWindow::loadSettings()
{
	const JsonNode saved = EditorSettings::getValue("newMap");

	if(saved["lastSettings"].isNull())
	{
		updateTemplateList();
		return;
	}

	JsonNode node = saved;
	node.setModScope(ModScope::scopeMap());
	JsonDeserializer handler(nullptr, node);
	handler.serializeStruct("lastSettings", *options);
	const CRmgTemplate * savedTemplate = options->getMapTemplate();

	randomMap = saved["generateRandom"].Bool();
	randomCheck->setChecked(randomMap);

	widthInput->setValue(options->getWidth());
	heightInput->setValue(options->getHeight());
	levelsInput->setValue(options->getLevels());

	sizeChoice->setSelected(CUSTOM_SIZE_INDEX);
	customSize = true;
	for(size_t i = 0; i < SIZE_VALUES.size(); ++i)
	{
		if(SIZE_VALUES[i] == options->getWidth() && SIZE_VALUES[i] == options->getHeight())
		{
			sizeChoice->setSelected(static_cast<int>(i));
			customSize = false;
			break;
		}
	}

	humanChoice->setSelected(options->getHumanOrCpuPlayerCount() == CMapGenOptions::RANDOM_SIZE ? 0 : options->getHumanOrCpuPlayerCount());
	cpuChoice->setSelected(options->getCompOnlyPlayerCount() == CMapGenOptions::RANDOM_SIZE ? 0 : options->getCompOnlyPlayerCount() + 1);
	humanTeamsChoice->setSelected(options->getTeamCount() == CMapGenOptions::RANDOM_SIZE ? 0 : options->getTeamCount() + 1);
	cpuTeamsChoice->setSelected(options->getCompOnlyTeamCount() == CMapGenOptions::RANDOM_SIZE ? 0 : options->getCompOnlyTeamCount() + 1);

	switch(options->getWaterContent())
	{
		case EWaterContent::RANDOM:
			waterChoice->setSelected(0);
			break;
		case EWaterContent::NONE:
			waterChoice->setSelected(1);
			break;
		case EWaterContent::NORMAL:
			waterChoice->setSelected(2);
			break;
		case EWaterContent::ISLANDS:
			waterChoice->setSelected(3);
			break;
	}

	switch(options->getMonsterStrength())
	{
		case EMonsterStrength::RANDOM:
			monsterChoice->setSelected(0);
			break;
		case EMonsterStrength::GLOBAL_WEAK:
			monsterChoice->setSelected(1);
			break;
		case EMonsterStrength::GLOBAL_NORMAL:
			monsterChoice->setSelected(2);
			break;
		case EMonsterStrength::GLOBAL_STRONG:
			monsterChoice->setSelected(3);
			break;
	}

	dirtRoad->setChecked(options->isRoadEnabled(Road::DIRT_ROAD));
	gravelRoad->setChecked(options->isRoadEnabled(Road::GRAVEL_ROAD));
	cobblestoneRoad->setChecked(options->isRoadEnabled(Road::COBBLESTONE_ROAD));

	updateTemplateList();
	options->setMapTemplate(savedTemplate);

	if(savedTemplate)
	{
		for(size_t i = 0; i < templates.size(); ++i)
			if(templates[i]->getName() == savedTemplate->getName())
				templateChoice->setSelected(static_cast<int>(i) + 1);
	}
}

void EditorNewMapWindow::saveSettings()
{
	JsonNode data;
	{
		JsonSerializer serializer(nullptr, data);
		serializer.serializeStruct("lastSettings", *options);
	}
	data.setModScope(ModScope::scopeMap());
	data["generateRandom"].Bool() = randomMap;

	EditorSettings::setValue("newMap", data);
}

void EditorNewMapWindow::updateAvailability()
{
	widthInput->block(!customSize);
	heightInput->block(!customSize);

	for(auto * choice : {humanChoice.get(), cpuChoice.get(), humanTeamsChoice.get(), cpuTeamsChoice.get(), waterChoice.get(), monsterChoice.get(), templateChoice.get()})
		choice->block(!randomMap);

	for(auto * check : {dirtRoad.get(), gravelRoad.get(), cobblestoneRoad.get(), seedCheck.get()})
		check->block(!randomMap);

	seedInput->block(!randomMap || !seedCheck->isChecked());
}

void EditorNewMapWindow::updateTemplateList()
{
	std::vector<std::string> names{EditorTexts::get("newMap.templateDefault")};
	templates.clear();

	if(randomMap)
	{
		options->setMapTemplate(nullptr);
		templates = options->getPossibleTemplates();

		for(const auto * templ : templates)
			names.push_back(templ->getName());
	}

	templateChoice->setOptions(names, 0);
}

void EditorNewMapWindow::applyRandomOptions()
{
	EWaterContent::EWaterContent water = EWaterContent::RANDOM;
	switch(waterChoice->getSelected())
	{
		case 1:
			water = EWaterContent::NONE;
			break;
		case 2:
			water = EWaterContent::NORMAL;
			break;
		case 3:
			water = EWaterContent::ISLANDS;
			break;
		default:
			break;
	}

	EMonsterStrength::EMonsterStrength monster = EMonsterStrength::RANDOM;
	switch(monsterChoice->getSelected())
	{
		case 1:
			monster = EMonsterStrength::GLOBAL_WEAK;
			break;
		case 2:
			monster = EMonsterStrength::GLOBAL_NORMAL;
			break;
		case 3:
			monster = EMonsterStrength::GLOBAL_STRONG;
			break;
		default:
			break;
	}

	options->setWaterContent(water);
	options->setMonsterStrength(monster);

	options->setRoadEnabled(Road::DIRT_ROAD, dirtRoad->isChecked());
	options->setRoadEnabled(Road::GRAVEL_ROAD, gravelRoad->isChecked());
	options->setRoadEnabled(Road::COBBLESTONE_ROAD, cobblestoneRoad->isChecked());

	if(customSize)
	{
		options->setWidth(widthInput->getValue());
		options->setHeight(heightInput->getValue());
	}
	else
	{
		const int sizeValue = SIZE_VALUES.at(sizeChoice->getSelected());
		options->setWidth(sizeValue);
		options->setHeight(sizeValue);
	}
}

void EditorNewMapWindow::showLayerDialog()
{
	auto dialog = std::make_shared<LayerDialog>(options->getLevelMapLayers());
	auto * raw = dialog.get();
	dialog->onAccept = [this, raw]()
	{
		options->setLevelMapLayers(raw->getLayers());
		return true;
	};
	ENGINE->windows().pushWindow(dialog);
}

void EditorNewMapWindow::onSizeChosen(int index)
{
	customSize = index == CUSTOM_SIZE_INDEX;

	if(!customSize)
	{
		options->setWidth(SIZE_VALUES.at(index));
		options->setHeight(SIZE_VALUES.at(index));
	}

	updateAvailability();
	updateTemplateList();
}

void EditorNewMapWindow::onLevelsChanged(int levels)
{
	if(levels > 2 && !layerWarningShown)
	{
		CMessageDialog::showMessage(EditorTexts::get("newMap.multilevelTitle"), EditorTexts::get("newMap.multilevelText"));
		layerWarningShown = true;
	}

	options->setLevels(levels);
	updateTemplateList();
	options->resetLevelMapLayers();
}

void EditorNewMapWindow::onHumansChosen(int index)
{
	int humans = std::min(playersAt(index), MAX_PLAYERS);

	int teams = options->getTeamCount();
	if(teams > humans - 1)
	{
		teams = humans > 0 ? humans - 1 : CMapGenOptions::RANDOM_SIZE;
		humanTeamsChoice->setSelected(teams + 1); //skip one element because first is random
		options->setTeamCount(teams);
	}

	int cpu = options->getCompOnlyPlayerCount();
	if(cpu > MAX_PLAYERS - humans)
	{
		cpu = MAX_PLAYERS - humans;
		cpuChoice->setSelected(cpu + 1); //skip one element because first is random
		options->setCompOnlyPlayerCount(cpu);
	}

	int cpuTeams = options->getCompOnlyTeamCount(); //comp only players - 1
	if(cpuTeams > cpu - 1)
	{
		cpuTeams = cpu > 0 ? cpu - 1 : CMapGenOptions::RANDOM_SIZE;
		cpuTeamsChoice->setSelected(cpuTeams + 1); //skip one element because first is random
		options->setCompOnlyTeamCount(cpuTeams);
	}

	options->setHumanOrCpuPlayerCount(humans);
	updateTemplateList();
}

void EditorNewMapWindow::onComputersChosen(int index)
{
	const int humans = options->getHumanOrCpuPlayerCount();
	int cpu = computersAt(index);

	if(cpu > MAX_PLAYERS - humans)
	{
		cpu = MAX_PLAYERS - humans;
		cpuChoice->setSelected(cpu + 1); //skip one element because first is random
	}

	int cpuTeams = options->getCompOnlyTeamCount(); //comp only players - 1
	if(cpuTeams > cpu - 1)
	{
		cpuTeams = cpu > 0 ? cpu - 1 : CMapGenOptions::RANDOM_SIZE;
		cpuTeamsChoice->setSelected(cpuTeams + 1); //skip one element because first is random
		options->setCompOnlyTeamCount(cpuTeams);
	}

	options->setCompOnlyPlayerCount(cpu);
	updateTemplateList();
}

void EditorNewMapWindow::onHumanTeamsChosen(int index)
{
	const int humans = options->getHumanOrCpuPlayerCount();
	int teams = computersAt(index);

	if(teams >= humans)
	{
		teams = humans > 0 ? humans - 1 : CMapGenOptions::RANDOM_SIZE;
		humanTeamsChoice->setSelected(teams + 1); //skip one element because first is random
	}

	options->setTeamCount(teams);
	updateTemplateList();
}

void EditorNewMapWindow::onComputerTeamsChosen(int index)
{
	const int cpu = options->getCompOnlyPlayerCount();
	int cpuTeams = computersAt(index);

	if(cpuTeams >= cpu)
	{
		cpuTeams = cpu > 0 ? cpu - 1 : CMapGenOptions::RANDOM_SIZE;
		cpuTeamsChoice->setSelected(cpuTeams + 1); //skip one element because first is random
	}

	options->setCompOnlyTeamCount(cpuTeams);
	updateTemplateList();
}

void EditorNewMapWindow::create()
{
	// the window is closed already, so everything needed later is captured by the callbacks
	auto generatedOptions = options;
	auto cb = callback;
	auto created = onCreated;

	if(!randomMap)
	{
		auto map = generateEmptyMap(*generatedOptions);
		map->mods = EditorMap::modAssessmentMap(*map);
		created(std::move(map));
		return;
	}

	int seed = static_cast<int>(std::time(nullptr));
	if(seedCheck->isChecked() && seedInput->getValue() != 0)
		seed = seedInput->getValue();

	auto generator = std::make_shared<CMapGenerator>(*generatedOptions, cb, seed);
	auto future = std::make_shared<std::future<std::unique_ptr<CMap>>>(std::async(std::launch::async, &CMapGenerator::generate, generator.get()));

	EditorProgressWindow::show(EditorTexts::get("newMap.generating"), *generator, [generator, generatedOptions, future, created]()
	{
		std::unique_ptr<CMap> map;

		try
		{
			map = future->get();
		}
		catch(const std::exception & e)
		{
			CMessageDialog::showMessage(EditorTexts::get("newMap.generatorFailure"), e.what());
			return;
		}

		map->mods = EditorMap::modAssessmentMap(*map);
		created(std::move(map));
	});
}

void EditorNewMapWindow::show(IGameInfoCallback * callback, const std::function<void(std::unique_ptr<CMap>)> & onCreated)
{
	ENGINE->windows().createAndPushWindow<EditorNewMapWindow>(callback, onCreated);
}
