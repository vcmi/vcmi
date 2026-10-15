/*
 * EditorScenarioDialog.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorScenarioDialog.h"

#include "EditorStartingBonusDialog.h"

#include "../EditorCampaignWindow.h"
#include "../EditorTexts.h"

#include "../../GameEngine.h"
#include "../../gui/WindowHandler.h"
#include "../../widgets/CCheckBox.h"
#include "../../widgets/CDropDown.h"
#include "../../widgets/CFormView.h"
#include "../../widgets/CListEditor.h"
#include "../../widgets/CMultiLineEdit.h"
#include "../../widgets/CSetPicker.h"
#include "../../widgets/CTabBar.h"
#include "../../widgets/CTextButton.h"
#include "../../widgets/CTextField.h"
#include "../../widgets/UiTheme.h"
#include "../../windows/CChoiceDialog.h"
#include "../../windows/CFileDialog.h"
#include "../../windows/CPromptWindows.h"

#include "../../../lib/CCreatureHandler.h"
#include "../../../lib/GameLibrary.h"
#include "../../../lib/campaign/CampaignState.h"
#include "../../../lib/entities/artifact/CArtHandler.h"
#include "../../../lib/entities/hero/CHeroHandler.h"
#include "../../../lib/mapping/CMap.h"
#include "../../../lib/texts/TextOperations.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem/fstream.hpp>

namespace
{
constexpr int LABEL_WIDTH = 190;
constexpr int TEXT_HEIGHT = 90;
constexpr int LIST_HEIGHT = 140;
constexpr int DIFFICULTIES = 5;
constexpr int DIFFICULTY_TEXT_OFFSET = 142;
constexpr int TAB_COUNT = 4;

/// Window that edits one row of the list of the starting heroes or of the source scenarios
class RowDialog : public CDialogWindow
{
	std::vector<int> optionIds;
	std::vector<int> playerIds;
	std::shared_ptr<CDropDown> optionChoice;
	std::shared_ptr<CDropDown> playerChoice;

public:
	RowDialog(const std::string & title, const std::string & optionLabel, const std::vector<int> & optionIds, const std::vector<std::string> & optionNames, int option,
		const std::vector<int> & playerIds, const std::vector<std::string> & playerNames, int player, const std::function<void(int, int)> & onSaved)
		: CDialogWindow(Point(520, 170), title)
		, optionIds(optionIds)
		, playerIds(playerIds)
	{
		OBJECT_CONSTRUCTION_TARGETED(content());

		const Point size = contentSize();
		const auto indexOf = [](const std::vector<int> & ids, int id)
		{
			const auto found = std::ranges::find(ids, id);
			return found == ids.end() ? 0 : static_cast<int>(found - ids.begin());
		};

		optionChoice = std::make_shared<CDropDown>(Rect(0, 0, size.x, UiTheme::CONTROL_HEIGHT), optionNames, indexOf(optionIds, option), nullptr, optionLabel);
		playerChoice = std::make_shared<CDropDown>(Rect(0, UiTheme::CONTROL_HEIGHT + UiTheme::SPACING, size.x, UiTheme::CONTROL_HEIGHT), playerNames, indexOf(playerIds, player), nullptr, EditorTexts::get("scenario.playerPosition"));

		onAccept = [this, onSaved]()
		{
			if(this->optionIds.empty() || this->playerIds.empty())
				return false;

			onSaved(this->optionIds.at(optionChoice->getSelected()), this->playerIds.at(playerChoice->getSelected()));
			return true;
		};
	}
};

template<class Container>
std::set<int> toSet(const Container & ids)
{
	std::set<int> result;
	for(const auto & id : ids)
		result.insert(id.getNum());
	return result;
}

std::vector<CChoiceDialog::Item> itemsOf(const std::vector<std::string> & names)
{
	return CChoiceDialog::makeItems(names);
}
}

EditorScenarioDialog::EditorScenarioDialog(const std::shared_ptr<CampaignState> & campaign, CampaignScenarioID scenario, EditorCallback * callback, const std::function<void()> & onChanged)
	: CDialogWindow(Point(820, 660), EditorTexts::get("scenario.title"))
	, campaign(campaign)
	, scenario(scenario)
	, callback(callback)
	, changedCallback(onChanged)
{
	OBJECT_CONSTRUCTION_TARGETED(content());

	const Point size = contentSize();
	const int tabHeight = UiTheme::CONTROL_HEIGHT + 4;

	std::map<int, std::string> heroSelection;
	for(const auto & hero : LIBRARY->heroh->objects)
		heroSelection.emplace(hero->getId().getNum(), hero->getNameTranslated());
	heroSelection.emplace(HeroTypeID::CAMP_STRONGEST, EditorTexts::get("bonus.strongest"));
	heroSelection.emplace(HeroTypeID::CAMP_GENERATED, EditorTexts::get("bonus.generated"));
	heroSelection.emplace(HeroTypeID::CAMP_RANDOM, EditorTexts::get("bonus.random"));
	for(const auto & [id, name] : heroSelection)
	{
		heroIds.push_back(id);
		heroNames.push_back(name);
	}

	tabs = std::make_shared<CTabBar>(Rect(0, 0, size.x, tabHeight),
		std::vector<std::string>{EditorTexts::get("scenario.general"), EditorTexts::get("scenario.texts"), EditorTexts::get("scenario.crossover"), EditorTexts::get("scenario.start")},
		0, [this](int index) { selectPage(index); });

	for(auto & page : pages)
		page = std::make_shared<CFormView>(Rect(0, tabHeight + UiTheme::SPACING, size.x, size.y - tabHeight - UiTheme::SPACING), LABEL_WIDTH);

	loadDraft();
	loadMap();
	buildPages();
	selectPage(0);

	onAccept = [this]() { return commit(); };
	afterAccept = onChanged;
}

EditorScenarioDialog::~EditorScenarioDialog() = default;

void EditorScenarioDialog::loadDraft()
{
	const CampaignScenario & source = campaign->scenarios.at(scenario);

	draft = Draft();
	draft.regionText = EditorTexts::translate(source.regionText);
	draft.regionColor = source.regionColor;
	draft.difficulty = source.difficulty;
	for(const auto & precondition : source.preconditionRegions)
		draft.prerequisites.insert(precondition.getNum());

	const auto loadPrologEpilog = [](const CampaignScenarioPrologEpilog & from, bool & enabled, std::array<std::string, 5> & fields)
	{
		enabled = from.hasPrologEpilog;
		fields = {from.prologVideo.first.getName(), from.prologVideo.second.getName(), from.prologMusic.getName(), from.prologVoice.getName(), EditorTexts::translate(from.prologText)};
	};
	loadPrologEpilog(source.prolog, draft.prologEnabled, draft.prolog);
	loadPrologEpilog(source.epilog, draft.epilogEnabled, draft.epilog);

	const auto & travel = source.travelOptions;
	draft.keepExperience = travel.whatHeroKeeps.experience;
	draft.keepPrimary = travel.whatHeroKeeps.primarySkills;
	draft.keepSecondary = travel.whatHeroKeeps.secondarySkills;
	draft.keepSpells = travel.whatHeroKeeps.spells;
	draft.keepArtifacts = travel.whatHeroKeeps.artifacts;
	draft.keptArtifacts = toSet(travel.artifactsKeptByHero);
	draft.keptCreatures = toSet(travel.monstersKeptByHero);

	draft.startOption = travel.startOptions == CampaignStartOptions::NONE ? CampaignStartOptions::START_BONUS : travel.startOptions;
	draft.playerPosition = travel.playerColor.getNum();

	for(const auto & bonus : travel.bonusesToChoose)
	{
		if(draft.startOption == CampaignStartOptions::START_BONUS)
			draft.bonuses.push_back(bonus);
		else if(draft.startOption == CampaignStartOptions::HERO_CROSSOVER)
			draft.rows.emplace_back(bonus.getValue<CampaignBonusHeroesFromScenario>().scenario.getNum(), bonus.getValue<CampaignBonusHeroesFromScenario>().startingPlayer.getNum());
		else
			draft.rows.emplace_back(bonus.getValue<CampaignBonusStartingHero>().hero.getNum(), bonus.getValue<CampaignBonusStartingHero>().startingPlayer.getNum());
	}
}

void EditorScenarioDialog::loadMap()
{
	mapTexts.reset();
	map = EditorCampaignWindow::tryToOpenMap(campaign, scenario, callback);

	playerPositions.clear();
	if(map)
	{
		mapTexts.emplace(map->texts);

		for(size_t i = 0; i < map->players.size(); ++i)
			if(map->players[i].canHumanPlay)
				playerPositions.push_back(PlayerColor(i));
	}

	if(!playerPositions.empty() && !vstd::contains(playerPositions, PlayerColor(draft.playerPosition)))
		draft.playerPosition = playerPositions.front().getNum();
}

std::string EditorScenarioDialog::scenarioTitle(int index) const
{
	return campaign->getRegions().regions.at(index).infix + " - " + campaign->scenarios.at(CampaignScenarioID(index)).mapName;
}

std::string EditorScenarioDialog::playerPositionName(const PlayerColor & player) const
{
	return EditorTexts::format("scenario.playerEntry", {std::to_string(player.getNum() + 1), EditorTexts::playerName(player)});
}

void EditorScenarioDialog::selectPage(int index)
{
	for(int i = 0; i < TAB_COUNT; ++i)
		pages[i]->setEnabled(i == index);

	tabs->setSelected(index);
	redraw();
}

void EditorScenarioDialog::buildPages()
{
	for(auto & page : pages)
		page->clear();

	buildGeneralPage();
	buildTextsPage();
	buildCrossoverPage();
	buildStartPage();

	for(auto & page : pages)
		page->finish();
}

void EditorScenarioDialog::buildGeneralPage()
{
	auto & page = *pages[0];
	const int controlWidth = page.contentWidth() - LABEL_WIDTH;

	page.addInfo(EditorTexts::get("scenario.region"), campaign->getRegions().regions.at(scenario.getNum()).infix);
	page.addInfo(EditorTexts::get("scenario.mapFile"), campaign->scenarios.at(scenario).mapName);
	page.addInfo(EditorTexts::get("scenario.mapName"), map ? EditorTexts::translate(map->name) : EditorTexts::get("scenario.noMap"));

	page.addButton(EditorTexts::get("scenario.import"), EditorTexts::get("scenario.importButton"), [this]() { importMap(); });
	if(map)
	{
		page.addButton(EditorTexts::get("scenario.export"), EditorTexts::get("scenario.exportButton"), [this]() { exportMap(); });
		page.addButton(EditorTexts::get("scenario.remove"), EditorTexts::get("scenario.removeButton"), [this]() { removeMap(); });
	}

	if(!map)
		return;

	auto regionText = std::make_shared<CMultiLineEdit>(Rect(0, 0, controlWidth, TEXT_HEIGHT), draft.regionText, [this](const std::string & text) { draft.regionText = text; });
	page.addWidget(EditorTexts::get("scenario.regionText"), regionText);

	std::vector<std::string> colors;
	for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
		colors.push_back(EditorTexts::playerName(PlayerColor(i)));
	page.addChoice(EditorTexts::get("scenario.regionColor"), colors, draft.regionColor, [this](int index) { draft.regionColor = index; });

	std::vector<std::string> difficulties;
	for(int i = 0; i < DIFFICULTIES; ++i)
		difficulties.push_back(EditorTexts::translate(TextIdentifier("core.arraytxt", DIFFICULTY_TEXT_OFFSET + i)));
	page.addChoice(EditorTexts::get("scenario.difficulty"), difficulties, draft.difficulty, [this](int index) { draft.difficulty = index; });

	std::vector<std::string> scenarios;
	for(int i = 0; i < static_cast<int>(campaign->scenarios.size()); ++i)
		scenarios.push_back(scenarioTitle(i));
	auto prerequisites = std::make_shared<CSetPicker>(Rect(0, 0, controlWidth, UiTheme::CONTROL_HEIGHT), EditorTexts::get("scenario.prerequisites"), itemsOf(scenarios), draft.prerequisites,
		[this](const std::set<int> & chosen) { draft.prerequisites = chosen; });
	page.addWidget(EditorTexts::get("scenario.prerequisites"), prerequisites);
}

void EditorScenarioDialog::buildTextsPage()
{
	auto & page = *pages[1];

	if(!map)
	{
		page.addParagraph(EditorTexts::get("scenario.noMap"));
		return;
	}

	const int controlWidth = page.contentWidth() - LABEL_WIDTH;
	const auto addSection = [&](const std::string & heading, bool & enabled, std::array<std::string, 5> & fields)
	{
		page.addHeading(heading);
		page.addCheck(EditorTexts::get("scenario.enabled"), enabled, [&enabled](bool on) { enabled = on; });
		page.addText(EditorTexts::get("scenario.video"), fields[0], [&fields](const std::string & text) { fields[0] = text; });
		page.addText(EditorTexts::get("scenario.videoLoop"), fields[1], [&fields](const std::string & text) { fields[1] = text; });
		page.addText(EditorTexts::get("scenario.music"), fields[2], [&fields](const std::string & text) { fields[2] = text; });
		page.addText(EditorTexts::get("scenario.voice"), fields[3], [&fields](const std::string & text) { fields[3] = text; });

		auto text = std::make_shared<CMultiLineEdit>(Rect(0, 0, controlWidth, TEXT_HEIGHT), fields[4], [&fields](const std::string & value) { fields[4] = value; });
		page.addWidget(EditorTexts::get("scenario.text"), text);
	};

	addSection(EditorTexts::get("scenario.prologue"), draft.prologEnabled, draft.prolog);
	addSection(EditorTexts::get("scenario.epilogue"), draft.epilogEnabled, draft.epilog);
}

void EditorScenarioDialog::buildCrossoverPage()
{
	auto & page = *pages[2];

	if(!map)
	{
		page.addParagraph(EditorTexts::get("scenario.noMap"));
		return;
	}

	const int controlWidth = page.contentWidth() - LABEL_WIDTH;

	page.addCheck(EditorTexts::get("scenario.keepExperience"), draft.keepExperience, [this](bool on) { draft.keepExperience = on; });
	page.addCheck(EditorTexts::get("scenario.keepPrimary"), draft.keepPrimary, [this](bool on) { draft.keepPrimary = on; });
	page.addCheck(EditorTexts::get("scenario.keepSecondary"), draft.keepSecondary, [this](bool on) { draft.keepSecondary = on; });
	page.addCheck(EditorTexts::get("scenario.keepSpells"), draft.keepSpells, [this](bool on) { draft.keepSpells = on; });
	page.addCheck(EditorTexts::get("scenario.keepArtifacts"), draft.keepArtifacts, [this](bool on) { draft.keepArtifacts = on; });

	std::vector<std::string> artifacts;
	for(const auto & artifact : LIBRARY->arth->objects)
		artifacts.push_back(artifact->getNameTranslated());
	auto keptArtifacts = std::make_shared<CSetPicker>(Rect(0, 0, controlWidth, UiTheme::CONTROL_HEIGHT), EditorTexts::get("scenario.keptArtifacts"), itemsOf(artifacts), draft.keptArtifacts,
		[this](const std::set<int> & chosen) { draft.keptArtifacts = chosen; });
	page.addWidget(EditorTexts::get("scenario.keptArtifacts"), keptArtifacts);

	std::vector<std::string> creatures;
	for(const auto & creature : LIBRARY->creh->objects)
		creatures.push_back(creature->getNameSingularTranslated());
	auto keptCreatures = std::make_shared<CSetPicker>(Rect(0, 0, controlWidth, UiTheme::CONTROL_HEIGHT), EditorTexts::get("scenario.keptCreatures"), itemsOf(creatures), draft.keptCreatures,
		[this](const std::set<int> & chosen) { draft.keptCreatures = chosen; });
	page.addWidget(EditorTexts::get("scenario.keptCreatures"), keptCreatures);
}

void EditorScenarioDialog::buildStartPage()
{
	auto & page = *pages[3];

	if(!map)
	{
		page.addParagraph(EditorTexts::get("scenario.noMap"));
		return;
	}

	const int controlWidth = page.contentWidth() - LABEL_WIDTH;

	bool heroGeneration = false;
	for(const auto & player : map->players)
		heroGeneration = heroGeneration || player.generateHeroAtMainTown;

	auto options = std::make_shared<std::vector<CampaignStartOptions>>();
	std::vector<std::string> optionNames;
	options->push_back(CampaignStartOptions::START_BONUS);
	optionNames.push_back(EditorTexts::get("scenario.startBonus"));
	if(scenario.getNum() > 0)
	{
		options->push_back(CampaignStartOptions::HERO_CROSSOVER);
		optionNames.push_back(EditorTexts::get("scenario.startCrossover"));
	}
	if(heroGeneration)
	{
		options->push_back(CampaignStartOptions::HERO_OPTIONS);
		optionNames.push_back(EditorTexts::get("scenario.startHero"));
	}

	const int selectedOption = static_cast<int>(vstd::find_pos(*options, draft.startOption));
	page.addChoice(EditorTexts::get("scenario.startOption"), optionNames, selectedOption < static_cast<int>(options->size()) ? selectedOption : 0, [this, options](int index)
	{
		draft.startOption = options->at(index);
		draft.bonuses.clear();
		draft.rows.clear();
		refreshStartList(-1);
	});

	std::vector<std::string> positions;
	int selectedPosition = 0;
	for(size_t i = 0; i < playerPositions.size(); ++i)
	{
		positions.push_back(playerPositionName(playerPositions[i]));
		if(playerPositions[i].getNum() == draft.playerPosition)
			selectedPosition = static_cast<int>(i);
	}
	page.addChoice(EditorTexts::get("scenario.playerPosition"), positions, selectedPosition, [this](int index) { draft.playerPosition = playerPositions.at(index).getNum(); });

	startList = std::make_shared<CListEditor>(Rect(0, 0, controlWidth, LIST_HEIGHT), true);
	startList->onAdd = [this]()
	{
		if(static_cast<int>(draft.bonuses.size() + draft.rows.size()) >= MAX_START_OPTIONS)
			return;

		if(draft.startOption == CampaignStartOptions::START_BONUS)
		{
			draft.bonuses.push_back(CampaignBonusSpell{HeroTypeID(), SpellID()});
			editStartBonus(static_cast<int>(draft.bonuses.size()) - 1);
		}
		else
		{
			draft.rows.emplace_back(draft.startOption == CampaignStartOptions::HERO_OPTIONS ? heroIds.front() : 0, playerPositions.empty() ? 0 : playerPositions.front().getNum());
			editStartRow(static_cast<int>(draft.rows.size()) - 1);
		}
	};
	startList->onEdit = [this](int index)
	{
		if(draft.startOption == CampaignStartOptions::START_BONUS)
			editStartBonus(index);
		else
			editStartRow(index);
	};
	startList->onRemove = [this](int index)
	{
		if(draft.startOption == CampaignStartOptions::START_BONUS)
			draft.bonuses.erase(draft.bonuses.begin() + index);
		else
			draft.rows.erase(draft.rows.begin() + index);

		refreshStartList(-1);
	};
	page.addWidget(EditorTexts::get("scenario.startList"), startList);
	refreshStartList(-1);
}

void EditorScenarioDialog::refreshStartList(int selected)
{
	std::vector<std::string> names;

	if(draft.startOption == CampaignStartOptions::START_BONUS)
	{
		for(const auto & bonus : draft.bonuses)
			names.push_back(EditorStartingBonusDialog::describe(bonus, *map));
	}
	else
	{
		for(const auto & [option, player] : draft.rows)
		{
			const std::string optionName = draft.startOption == CampaignStartOptions::HERO_OPTIONS ? heroNames.at(std::ranges::find(heroIds, option) - heroIds.begin()) : scenarioTitle(option);
			names.push_back(optionName + " / " + playerPositionName(PlayerColor(player)));
		}
	}

	startList->setEntries(names, selected);
}

void EditorScenarioDialog::editStartBonus(int index)
{
	if(playerPositions.empty())
	{
		draft.bonuses.erase(draft.bonuses.begin() + index);
		CMessageDialog::showMessage(EditorTexts::get("scenario.title"), EditorTexts::get("scenario.noPlayerPosition"));
		return;
	}

	ENGINE->windows().createAndPushWindow<EditorStartingBonusDialog>(PlayerColor(draft.playerPosition), *map, draft.bonuses.at(index), [this, index](const CampaignBonus & bonus)
	{
		draft.bonuses.at(index) = bonus;
		refreshStartList(index);
	});
}

void EditorScenarioDialog::editStartRow(int index)
{
	std::vector<int> optionIds;
	std::vector<std::string> optionNames;
	if(draft.startOption == CampaignStartOptions::HERO_OPTIONS)
	{
		optionIds = heroIds;
		optionNames = heroNames;
	}
	else
	{
		for(int i = 0; i < scenario.getNum(); ++i)
		{
			optionIds.push_back(i);
			optionNames.push_back(scenarioTitle(i));
		}
	}

	std::vector<int> playerIds;
	std::vector<std::string> playerNames;
	for(const auto & player : playerPositions)
	{
		playerIds.push_back(player.getNum());
		playerNames.push_back(playerPositionName(player));
	}

	const auto & row = draft.rows.at(index);
	ENGINE->windows().createAndPushWindow<RowDialog>(EditorTexts::get("scenario.startRow"), EditorTexts::get(draft.startOption == CampaignStartOptions::HERO_OPTIONS ? "scenario.hero" : "scenario.sourceScenario"),
		optionIds, optionNames, row.first, playerIds, playerNames, row.second, [this, index](int option, int player)
		{
			draft.rows.at(index) = {option, player};
			refreshStartList(index);
		});
}

void EditorScenarioDialog::importMap()
{
	CFileDialog::openFile(EditorTexts::get("file.openMap"), {"vmap", "h3m"}, [this](const boost::filesystem::path & file)
	{
		std::vector<uint8_t> data;
		try
		{
			boost::filesystem::ifstream stream(file, std::ios::binary);
			if(!stream)
				throw std::runtime_error(EditorTexts::get("error.openFile"));

			data.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
		}
		catch(const std::exception & e)
		{
			CMessageDialog::showMessage(EditorTexts::get("error.title"), e.what());
			return;
		}

		campaign->mapPieces[scenario] = data;
		campaign->scenarios.at(scenario).mapName = TextOperations::filesystemPathToUtf8(file.filename());

		if(!EditorCampaignWindow::tryToOpenMap(campaign, scenario, callback))
		{
			campaign->mapPieces.erase(scenario);
			campaign->scenarios.at(scenario) = CampaignScenario();
			return;
		}

		loadMap();
		buildPages();
		if(changedCallback)
			changedCallback();
	});
}

void EditorScenarioDialog::exportMap()
{
	const std::string mapName = campaign->scenarios.at(scenario).mapName;
	const boost::filesystem::path name(mapName);
	const std::string extension = boost::algorithm::to_lower_copy(name.extension().string()) == ".vmap" ? "vmap" : "h3m";

	CFileDialog::saveFile(EditorTexts::get("file.saveMap"), extension, TextOperations::filesystemPathToUtf8(name.stem()), [this](const boost::filesystem::path & file)
	{
		try
		{
			const auto & data = campaign->mapPieces.at(scenario);
			boost::filesystem::ofstream stream(file, std::ios::binary);
			if(!stream)
				throw std::runtime_error(EditorTexts::get("error.openFile"));

			stream.write(reinterpret_cast<const char *>(data.data()), static_cast<std::streamsize>(data.size()));
		}
		catch(const std::exception & e)
		{
			CMessageDialog::showMessage(EditorTexts::get("error.title"), e.what());
		}
	});
}

void EditorScenarioDialog::removeMap()
{
	campaign->mapPieces.erase(scenario);
	campaign->scenarios.at(scenario) = CampaignScenario();

	loadDraft();
	loadMap();
	buildPages();
	if(changedCallback)
		changedCallback();
}

bool EditorScenarioDialog::commit()
{
	CampaignScenario & target = campaign->scenarios.at(scenario);

	// the map related settings are meaningful only while the scenario has a map
	if(!map)
		return true;

	target.regionText = MetaString::createFromRawString(draft.regionText);
	target.regionColor = static_cast<ui8>(draft.regionColor);
	target.difficulty = static_cast<ui8>(draft.difficulty);

	target.preconditionRegions.clear();
	for(int id : draft.prerequisites)
		target.preconditionRegions.emplace(id);

	const auto savePrologEpilog = [](CampaignScenarioPrologEpilog & to, bool enabled, const std::array<std::string, 5> & fields)
	{
		to.hasPrologEpilog = enabled;
		to.prologVideo.first = VideoPath::builtin(fields[0]);
		to.prologVideo.second = VideoPath::builtin(fields[1]);
		to.prologMusic = AudioPath::builtin(fields[2]);
		to.prologVoice = AudioPath::builtin(fields[3]);
		to.prologText = MetaString::createFromRawString(fields[4]);
	};
	savePrologEpilog(target.prolog, draft.prologEnabled, draft.prolog);
	savePrologEpilog(target.epilog, draft.epilogEnabled, draft.epilog);

	auto & travel = target.travelOptions;
	travel.whatHeroKeeps.experience = draft.keepExperience;
	travel.whatHeroKeeps.primarySkills = draft.keepPrimary;
	travel.whatHeroKeeps.secondarySkills = draft.keepSecondary;
	travel.whatHeroKeeps.spells = draft.keepSpells;
	travel.whatHeroKeeps.artifacts = draft.keepArtifacts;

	travel.artifactsKeptByHero.clear();
	for(int id : draft.keptArtifacts)
		travel.artifactsKeptByHero.emplace(id);
	travel.monstersKeptByHero.clear();
	for(int id : draft.keptCreatures)
		travel.monstersKeptByHero.emplace(id);

	travel.startOptions = draft.startOption;
	travel.bonusesToChoose.clear();
	if(draft.startOption == CampaignStartOptions::START_BONUS)
	{
		travel.playerColor = PlayerColor(draft.playerPosition);
		travel.bonusesToChoose = draft.bonuses;
	}
	else
	{
		for(const auto & [option, player] : draft.rows)
		{
			if(draft.startOption == CampaignStartOptions::HERO_CROSSOVER)
				travel.bonusesToChoose.push_back(CampaignBonusHeroesFromScenario{PlayerColor(player), CampaignScenarioID(option)});
			else
				travel.bonusesToChoose.push_back(CampaignBonusStartingHero{PlayerColor(player), HeroTypeID(option)});
		}
	}

	return true;
}
