/*
 * EditorCampaignWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorCampaignWindow.h"

#include "EditorFiles.h"
#include "EditorTexts.h"
#include "campaign/EditorCampaignPropertiesDialog.h"
#include "campaign/EditorScenarioDialog.h"

#include "../GameEngine.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CMenuBar.h"
#include "../widgets/CPanel.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"
#include "../widgets/UiTheme.h"
#include "../windows/CChoiceDialog.h"
#include "../windows/CFileDialog.h"
#include "../windows/CPopupMenu.h"
#include "../windows/CPromptWindows.h"

#include "../../Version.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/callback/EditorCallback.h"
#include "../../lib/campaign/CampaignHandler.h"
#include "../../lib/campaign/CampaignRegionsHandler.h"
#include "../../lib/campaign/CampaignState.h"
#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/json/JsonUtils.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/modding/ModIncompatibility.h"
#include "../../lib/texts/CompositeTranslator.h"
#include "../../lib/texts/TextOperations.h"

#include <boost/algorithm/string/erase.hpp>

namespace
{
/// Width of the part of the background that the campaign map uses unless the full background is shown
constexpr int MAP_WIDTH = 456;
constexpr int MAP_HEIGHT = 600;
constexpr int FULL_WIDTH = 800;

CPopupMenu::Entry makeEntry(const std::string & text, bool enabled, const std::function<void()> & callback, bool checked = false)
{
	CPopupMenu::Entry entry;
	entry.text = text;
	entry.enabled = enabled;
	entry.checked = checked;
	entry.callback = callback;
	return entry;
}
}

/// Region of the campaign map that belongs to a scenario
class EditorCampaignRegion : public CIntObject
{
	CampaignScenarioID scenario;
	std::shared_ptr<CPicture> picture;

public:
	std::function<void(CampaignScenarioID)> onSelect;
	std::function<void(CampaignScenarioID)> onOpen;
	std::function<void(CampaignScenarioID, const Point &)> onMenu;
	std::function<void(CampaignScenarioID, bool)> onHover;

	EditorCampaignRegion(CampaignScenarioID scenario, const Point & position, const ImagePath & image)
		: CIntObject(LCLICK | DOUBLECLICK | SHOW_POPUP | HOVER)
		, scenario(scenario)
	{
		pos += position;

		OBJECT_CONSTRUCTION;
		picture = std::make_shared<CPicture>(image);
		pos.w = picture->pos.w;
		pos.h = picture->pos.h;
	}

	void clickPressed(const Point & cursorPosition) override
	{
		if(onSelect)
			onSelect(scenario);
	}

	void clickDouble(const Point & cursorPosition) override
	{
		if(onOpen)
			onOpen(scenario);
	}

	void showPopupWindow(const Point & cursorPosition) override
	{
		if(onMenu)
			onMenu(scenario, cursorPosition);
	}

	void hover(bool on) override
	{
		if(onHover)
			onHover(scenario, on);
	}
};

/// Background of the campaign map with the regions of the scenarios
class EditorCampaignView : public CIntObject
{
	std::shared_ptr<CPicture> backgroundPicture;
	std::vector<std::shared_ptr<EditorCampaignRegion>> regions;
	/// The callbacks of a region rebuild the view, so the regions removed by it live until the next rebuild
	std::vector<std::shared_ptr<EditorCampaignRegion>> removedRegions;

public:
	std::function<void(CampaignScenarioID)> onSelect;
	std::function<void(CampaignScenarioID)> onOpen;
	std::function<void(CampaignScenarioID, const Point &)> onMenu;
	std::function<void(CampaignScenarioID, bool)> onHover;

	EditorCampaignView(const Point & position)
	{
		pos += position;
		pos.w = MAP_WIDTH;
		pos.h = MAP_HEIGHT;
	}

	void setCampaign(const std::shared_ptr<CampaignState> & campaign, CampaignScenarioID selected, bool fullBackground)
	{
		OBJECT_CONSTRUCTION;

		for(auto & region : regions)
		{
			region->deactivate();
			removeChild(region.get());
		}
		removedRegions = std::move(regions);
		regions.clear();
		backgroundPicture.reset();

		if(!campaign)
			return;

		const CampaignRegions & campaignRegions = campaign->getRegions();
		const int width = fullBackground ? FULL_WIDTH : MAP_WIDTH;
		backgroundPicture = std::make_shared<CPicture>(campaignRegions.getBackgroundName(), Rect(0, 0, width, MAP_HEIGHT), 0, 0);
		pos.w = width;
		pos.h = MAP_HEIGHT;

		for(const auto & id : campaign->allScenarios())
		{
			const CampaignScenario & scenario = campaign->scenario(id);
			ImagePath image = campaignRegions.getAvailableName(id, scenario.regionColor);
			if(id == selected)
				image = campaignRegions.getSelectedName(id, scenario.regionColor);
			else if(scenario.mapName.empty())
				image = campaignRegions.getConqueredName(id, scenario.regionColor);

			auto region = std::make_shared<EditorCampaignRegion>(id, campaignRegions.getPosition(id), image);
			region->onSelect = [this](CampaignScenarioID scenarioId) { if(onSelect) onSelect(scenarioId); };
			region->onOpen = [this](CampaignScenarioID scenarioId) { if(onOpen) onOpen(scenarioId); };
			region->onMenu = [this](CampaignScenarioID scenarioId, const Point & cursor) { if(onMenu) onMenu(scenarioId, cursor); };
			region->onHover = [this](CampaignScenarioID scenarioId, bool on) { if(onHover) onHover(scenarioId, on); };
			regions.push_back(region);
		}

		redraw();
	}
};

EditorCampaignWindow::EditorCampaignWindow(EditorCallback * callback)
	: WindowBase(KEYBOARD)
	, callback(callback)
	, selected(CampaignScenarioID::NONE)
{
	build();
}

void EditorCampaignWindow::show(EditorCallback * callback, const boost::filesystem::path & file)
{
	auto window = std::make_shared<EditorCampaignWindow>(callback);
	ENGINE->windows().pushWindow(window);

	if(!file.empty())
		window->openCampaign(file);
}

void EditorCampaignWindow::build()
{
	OBJECT_CONSTRUCTION;

	const Point screen = ENGINE->screenDimensions();
	pos = Rect(0, 0, screen.x, screen.y);

	background = std::make_shared<CPanel>(Rect(0, 0, screen.x, screen.y), false);
	topPanel = std::make_shared<CPanel>(Rect(0, 0, screen.x, MENU_HEIGHT));
	statusPanel = std::make_shared<CPanel>(Rect(0, screen.y - STATUS_HEIGHT, screen.x, STATUS_HEIGHT));

	{
		OBJECT_CONSTRUCTION_TARGETED(topPanel.get());

		std::vector<CMenuBar::Menu> menus;
		menus.push_back({EditorTexts::get("menu.file"), [this]()
		{
			std::vector<CPopupMenu::Entry> entries;
			entries.push_back(makeEntry(EditorTexts::get("campaign.new"), true, [this]() { newCampaign(); }));
			entries.push_back(makeEntry(EditorTexts::get("campaign.open"), true, [this]() { openDialog(); }));
			entries.push_back(makeEntry(EditorTexts::get("campaign.openSet"), true, [this]() { openSetDialog(); }));
			entries.push_back(makeEntry(EditorTexts::get("menu.save"), campaign != nullptr, [this]() { save(); }));
			entries.push_back(makeEntry(EditorTexts::get("menu.saveAs"), campaign != nullptr, [this]() { saveAs(); }));
			entries.push_back(CPopupMenu::Entry::makeSeparator());
			entries.push_back(makeEntry(EditorTexts::get("menu.exit"), true, [this]() { closeEditor(); }));
			return entries;
		}});

		menus.push_back({EditorTexts::get("menu.edit"), [this]()
		{
			const bool hasScenario = campaign != nullptr && campaign->scenarios.count(selected);

			std::vector<CPopupMenu::Entry> entries;
			entries.push_back(makeEntry(EditorTexts::get("campaign.properties"), campaign != nullptr, [this]() { showCampaignProperties(); }));
			entries.push_back(makeEntry(EditorTexts::get("campaign.scenarioProperties"), hasScenario, [this]() { showScenarioProperties(selected); }));
			return entries;
		}});

		menus.push_back({EditorTexts::get("menu.view"), [this]()
		{
			std::vector<CPopupMenu::Entry> entries;
			entries.push_back(makeEntry(EditorTexts::get("campaign.fullBackground"), true, [this]()
			{
				fullBackground = !fullBackground;
				refresh();
			}, fullBackground));
			return entries;
		}});

		menuBar = std::make_shared<CMenuBar>(Rect(0, 0, screen.x, MENU_HEIGHT), menus);
		titleLabel = std::make_shared<CLabel>(screen.x - 8, MENU_HEIGHT / 2, UiTheme::FONT, ETextAlignment::CENTERRIGHT, UiTheme::TEXT, "");
	}

	{
		OBJECT_CONSTRUCTION_TARGETED(statusPanel.get());
		statusLabel = std::make_shared<CLabel>(8, STATUS_HEIGHT / 2, UiTheme::FONT, ETextAlignment::CENTERLEFT, UiTheme::TEXT, "");
	}

	view = std::make_shared<EditorCampaignView>(Point((screen.x - MAP_WIDTH) / 2, MENU_HEIGHT + (screen.y - MENU_HEIGHT - STATUS_HEIGHT - MAP_HEIGHT) / 2));
	view->onSelect = [this](CampaignScenarioID scenario)
	{
		if(selected != scenario)
		{
			selected = scenario;
			refresh();
		}
	};
	view->onOpen = [this](CampaignScenarioID scenario)
	{
		selected = scenario;
		showScenarioProperties(scenario);
	};
	view->onMenu = [this](CampaignScenarioID scenario, const Point & cursor)
	{
		selected = scenario;
		refresh();
		CPopupMenu::showAt(cursor, {makeEntry(EditorTexts::get("campaign.scenarioEditor"), true, [this, scenario]() { showScenarioProperties(scenario); })});
	};
	view->onHover = [this](CampaignScenarioID scenario, bool on)
	{
		setStatus(on && campaign ? campaign->scenarios.at(scenario).mapName : "");
	};

	updateTitle();
}

void EditorCampaignWindow::setCampaign(std::shared_ptr<CampaignState> newCampaign)
{
	campaignTexts.reset();
	campaign = std::move(newCampaign);

	// the texts of the campaign are not installed by the loader
	if(campaign)
		campaignTexts.emplace(campaign->getTexts());
}

void EditorCampaignWindow::refresh()
{
	view->setCampaign(campaign, selected, fullBackground);
	updateTitle();
	redraw();
}

void EditorCampaignWindow::updateTitle()
{
	const std::string name = filename.empty() ? "" : TextOperations::filesystemPathToUtf8(filename.filename());
	titleLabel->setText(name + (unsaved ? "*" : "") + " - " + EditorTexts::get("campaign.windowTitle") + " (" + GameConstants::VCMI_VERSION + ")");
}

void EditorCampaignWindow::changed()
{
	unsaved = true;
	updateTitle();
}

void EditorCampaignWindow::setStatus(const std::string & text)
{
	statusLabel->setText(text);
}

bool EditorCampaignWindow::validate()
{
	if(campaign->mapPieces.empty())
	{
		CMessageDialog::showMessage(EditorTexts::get("campaign.validationFailed"), EditorTexts::get("campaign.noMaps"));
		return false;
	}

	if(static_cast<int>(campaign->mapPieces.size()) != campaign->campaignRegions.regionsCount())
		logGlobal->trace("Not all regions have a map");

	return true;
}

bool EditorCampaignWindow::confirmDiscard(const std::function<void()> & next)
{
	if(!unsaved || !campaign)
	{
		next();
		return true;
	}

	CMessageDialog::askConfirmation(EditorTexts::get("confirm.title"), EditorTexts::get("confirm.unsaved"), next);
	return false;
}

void EditorCampaignWindow::newCampaign()
{
	confirmDiscard([this]()
	{
		auto created = std::make_shared<CampaignState>();
		created->campaignRegions = *LIBRARY->campaignRegions->getByIndex(0);
		for(int i = 0; i < created->campaignRegions.regionsCount(); ++i)
		{
			CampaignScenario scenario;
			scenario.travelOptions.startOptions = CampaignStartOptions::START_BONUS;
			created->scenarios.emplace(CampaignScenarioID(i), scenario);
		}
		created->modName = "mapEditor";
		created->creationDateTime = std::time(nullptr);

		setCampaign(created);
		filename.clear();
		selected = CampaignScenarioID::NONE;
		unsaved = true;
		refresh();
	});
}

void EditorCampaignWindow::openDialog()
{
	confirmDiscard([this]()
	{
		CFileDialog::openFile(EditorTexts::get("campaign.openTitle"), {"vcmp", "h3c"}, [this](const boost::filesystem::path & file) { openCampaign(file); });
	});
}

void EditorCampaignWindow::openCampaign(const boost::filesystem::path & file)
{
	std::shared_ptr<CampaignState> opened;
	try
	{
		opened = EditorFiles::openCampaign(file);
	}
	catch(const std::exception & e)
	{
		CMessageDialog::showMessage(EditorTexts::get("campaign.openFailed"), e.what());
		return;
	}

	for(const auto & scenario : opened->allScenarios())
	{
		// scenarios without a map are valid, the others have to load
		if(opened->mapPieces.count(scenario) && !tryToOpenMap(opened, scenario, callback))
			return;
	}

	while(static_cast<int>(opened->scenarios.size()) < opened->campaignRegions.regionsCount())
		opened->scenarios.emplace(CampaignScenarioID(std::prev(opened->scenarios.end())->first + 1), CampaignScenario());

	setCampaign(opened);
	selected = opened->scenarios.empty() ? CampaignScenarioID(CampaignScenarioID::NONE) : opened->scenarios.begin()->first;
	filename = file;
	unsaved = false;
	refresh();
}

void EditorCampaignWindow::openSetDialog()
{
	confirmDiscard([this]()
	{
		const auto campaignSets = JsonUtils::assembleFromFiles("config/campaignSets.json");

		struct Set
		{
			std::string name;
			std::vector<ResourcePath> campaigns;
		};
		auto sets = std::make_shared<std::vector<Set>>();

		for(const auto & [key, set] : campaignSets.Struct())
		{
			Set entry;
			entry.name = set["text"].isNull() ? key : LIBRARY->generaltexth->translate(set["text"].String());
			for(const auto & item : set["items"].Vector())
			{
				const ResourcePath resource(item["file"].String(), EResType::CAMPAIGN);
				if(CResourceHandler::get()->existsResource(resource))
					entry.campaigns.push_back(resource);
			}
			sets->push_back(std::move(entry));
		}

		std::vector<std::string> setNames;
		for(const auto & set : *sets)
			setNames.push_back(set.name);

		CChoiceDialog::chooseOne(EditorTexts::get("campaign.openSetTitle"), CChoiceDialog::makeItems(setNames), 0, [this, sets](int setIndex)
		{
			auto campaigns = std::make_shared<std::vector<ResourcePath>>(sets->at(setIndex).campaigns);
			std::vector<std::string> campaignNames;
			for(const auto & resource : *campaigns)
			{
				const auto header = CampaignHandler::getHeader(resource.getName());
				CompositeTranslator translator;
				translator.install(header->getTexts());
				campaignNames.push_back(header->getNameTranslated(&translator));
			}

			CChoiceDialog::chooseOne(EditorTexts::get("campaign.openCampaignTitle"), CChoiceDialog::makeItems(campaignNames), 0, [this, campaigns](int index)
			{
				auto opened = CampaignHandler::getCampaign(campaigns->at(index).getName());
				setCampaign(opened);
				filename.clear();
				selected = opened->scenarios.empty() ? CampaignScenarioID(CampaignScenarioID::NONE) : opened->scenarios.begin()->first;
				unsaved = false;
				refresh();
			});
		});
	});
}

void EditorCampaignWindow::save()
{
	if(!campaign)
		return;

	if(filename.empty())
	{
		saveAs();
		return;
	}

	if(!validate())
		return;

	try
	{
		EditorFiles::saveCampaign(campaign, filename);
	}
	catch(const std::exception & e)
	{
		CMessageDialog::showMessage(EditorTexts::get("campaign.saveFailed"), e.what());
		return;
	}

	unsaved = false;
	updateTitle();
}

void EditorCampaignWindow::saveAs()
{
	if(!campaign)
		return;

	const std::string defaultName = filename.empty() ? "campaign" : TextOperations::filesystemPathToUtf8(filename.stem());
	CFileDialog::saveFile(EditorTexts::get("campaign.saveTitle"), "vcmp", defaultName, [this](const boost::filesystem::path & file)
	{
		filename = file;
		save();
	});
}

void EditorCampaignWindow::showCampaignProperties()
{
	if(!campaign)
		return;

	ENGINE->windows().createAndPushWindow<EditorCampaignPropertiesDialog>(campaign, [this]()
	{
		changed();
		refresh();
	});
}

void EditorCampaignWindow::showScenarioProperties(CampaignScenarioID scenario)
{
	if(!campaign || scenario == CampaignScenarioID::NONE || !campaign->scenarios.count(scenario))
		return;

	ENGINE->windows().createAndPushWindow<EditorScenarioDialog>(campaign, scenario, callback, [this]()
	{
		changed();
		refresh();
	});
}

void EditorCampaignWindow::closeEditor()
{
	confirmDiscard([this]() { close(); });
}

std::unique_ptr<CMap> EditorCampaignWindow::tryToOpenMap(const std::shared_ptr<CampaignState> & campaign, CampaignScenarioID scenario, EditorCallback * callback)
{
	try
	{
		return campaign->getMap(scenario, callback);
	}
	catch(const ModIncompatibility & e)
	{
		assert(e.whatExcessive().empty());
		std::string message = e.getFullErrorMsg();
		boost::erase_all(message, "{");
		boost::erase_all(message, "}");
		CMessageDialog::showMessage(EditorTexts::get("error.modsRequired"), message);
	}
	catch(const IdentifierResolutionException & e)
	{
		MetaString errorMsg;
		errorMsg.appendTextID("vcmi.server.errors.campOrMapFile.unknownEntity");
		errorMsg.replaceRawString(e.identifierName);
		CMessageDialog::showMessage(EditorTexts::get("error.openMap"), EditorTexts::translate(errorMsg));
	}
	catch(const std::exception & e)
	{
		CMessageDialog::showMessage(EditorTexts::get("error.openMap"), e.what());
	}

	return nullptr;
}
