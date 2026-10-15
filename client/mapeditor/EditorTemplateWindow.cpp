/*
 * EditorTemplateWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorTemplateWindow.h"

#include "EditorFiles.h"
#include "EditorTexts.h"
#include "templates/EditorTemplateDialogs.h"
#include "templates/EditorTemplateView.h"
#include "templates/GeometryAlgorithm.h"

#include "../GameEngine.h"
#include "../Version.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CCheckBox.h"
#include "../widgets/CDropDown.h"
#include "../widgets/CFormView.h"
#include "../widgets/CListEditor.h"
#include "../widgets/CMenuBar.h"
#include "../widgets/CNumberInput.h"
#include "../widgets/CPanel.h"
#include "../widgets/CTabBar.h"
#include "../widgets/CTextField.h"
#include "../widgets/TextControls.h"
#include "../widgets/UiTheme.h"
#include "../windows/CDialogWindow.h"
#include "../windows/CFileDialog.h"
#include "../windows/CPopupMenu.h"
#include "../windows/CPromptWindows.h"

#include "../../lib/callback/EditorCallback.h"
#include "../../lib/texts/TextOperations.h"

namespace
{
constexpr int LABEL_WIDTH = 150;
constexpr int LIST_HEIGHT = 110;
constexpr int MAX_SIZE = 999;
constexpr int MAX_ZONE_SIZE = 999;
constexpr int MAX_TOWNS = 99;
constexpr int MAX_DENSITY = 99;
constexpr int MAX_GUARD = 99999;
constexpr int MAX_ZONE_ID = 999;
constexpr int MIN_VISIBLE_SIZE_PERCENT = 10;
constexpr int MAX_VISIBLE_SIZE_PERCENT = 500;
constexpr int PERCENT = 100;
constexpr int LAYOUT_ITERATIONS = 1000;
constexpr double LAYOUT_SIZE = 500;
const std::string DEFAULT_TEMPLATE_NAME = "TemplateEditor";

CPopupMenu::Entry makeEntry(const std::string & text, bool enabled, const std::function<void()> & callback)
{
	CPopupMenu::Entry entry;
	entry.text = text;
	entry.enabled = enabled;
	entry.callback = callback;
	return entry;
}

/// Window that edits a range of the numbers of players
class RangeDialog : public CDialogWindow
{
	std::shared_ptr<CFormView> form;
	std::shared_ptr<CNumberInput> minInput;
	std::shared_ptr<CNumberInput> maxInput;

public:
	RangeDialog(const std::pair<int, int> & range, const std::function<void(const std::pair<int, int> &)> & onSaved)
		: CDialogWindow(Point(380, 170), EditorTexts::get("template.range"))
	{
		OBJECT_CONSTRUCTION_TARGETED(content());

		const Point size = contentSize();
		form = std::make_shared<CFormView>(Rect(0, 0, size.x, size.y), LABEL_WIDTH);
		minInput = form->addNumber(EditorTexts::get("template.min"), range.first, 0, MAX_SIZE, nullptr);
		maxInput = form->addNumber(EditorTexts::get("template.max"), range.second, 0, MAX_SIZE, nullptr);
		form->finish();

		onAccept = [this, onSaved]()
		{
			onSaved({minInput->getValue(), maxInput->getValue()});
			return true;
		};
	}
};

std::string connectionTypeName(rmg::EConnectionType type)
{
	switch(type)
	{
		case rmg::EConnectionType::GUARDED: return EditorTexts::get("template.guarded");
		case rmg::EConnectionType::FICTIVE: return EditorTexts::get("template.fictive");
		case rmg::EConnectionType::REPULSIVE: return EditorTexts::get("template.repulsive");
		case rmg::EConnectionType::WIDE: return EditorTexts::get("template.wide");
		case rmg::EConnectionType::FORCE_PORTAL: return EditorTexts::get("template.forcePortal");
	}
	return "";
}

const std::array<rmg::EConnectionType, 5> CONNECTION_TYPES{rmg::EConnectionType::GUARDED, rmg::EConnectionType::FICTIVE, rmg::EConnectionType::REPULSIVE, rmg::EConnectionType::WIDE, rmg::EConnectionType::FORCE_PORTAL};
const std::array<rmg::ERoadOption, 3> ROAD_OPTIONS{rmg::ERoadOption::ROAD_RANDOM, rmg::ERoadOption::ROAD_TRUE, rmg::ERoadOption::ROAD_FALSE};
const std::array<ETemplateZoneType, 6> ZONE_TYPES{ETemplateZoneType::PLAYER_START, ETemplateZoneType::CPU_START, ETemplateZoneType::TREASURE, ETemplateZoneType::JUNCTION, ETemplateZoneType::WATER, ETemplateZoneType::SEALED};
const std::array<EZoneLevel, 3> ZONE_LEVELS{EZoneLevel::AUTOMATIC, EZoneLevel::SURFACE, EZoneLevel::UNDERGROUND};
const std::array<EMonsterStrength::EMonsterStrength, 5> MONSTER_STRENGTHS{EMonsterStrength::ZONE_NONE, EMonsterStrength::RANDOM, EMonsterStrength::ZONE_WEAK, EMonsterStrength::ZONE_NORMAL, EMonsterStrength::ZONE_STRONG};

template<class T, size_t N>
int positionOf(const std::array<T, N> & values, const T & value)
{
	const auto found = std::ranges::find(values, value);
	return found == values.end() ? 0 : static_cast<int>(found - values.begin());
}
}

EditorTemplateWindow::EditorTemplateWindow()
	: WindowBase(KEYBOARD)
	, callback(std::make_unique<EditorCallback>(nullptr))
{
	build();
}

EditorTemplateWindow::~EditorTemplateWindow() = default;

void EditorTemplateWindow::show()
{
	ENGINE->windows().createAndPushWindow<EditorTemplateWindow>();
}

void EditorTemplateWindow::build()
{
	OBJECT_CONSTRUCTION;

	const Point screen = ENGINE->screenDimensions();
	pos = Rect(0, 0, screen.x, screen.y);

	background = std::make_shared<CPanel>(Rect(0, 0, screen.x, screen.y), false);
	view = std::make_shared<EditorTemplateView>(Rect(0, MENU_HEIGHT, screen.x - PANEL_WIDTH, screen.y - MENU_HEIGHT - STATUS_HEIGHT));
	topPanel = std::make_shared<CPanel>(Rect(0, 0, screen.x, MENU_HEIGHT));
	sidePanel = std::make_shared<CPanel>(Rect(screen.x - PANEL_WIDTH, MENU_HEIGHT, PANEL_WIDTH, screen.y - MENU_HEIGHT - STATUS_HEIGHT));
	statusPanel = std::make_shared<CPanel>(Rect(0, screen.y - STATUS_HEIGHT, screen.x, STATUS_HEIGHT));

	{
		OBJECT_CONSTRUCTION_TARGETED(topPanel.get());

		std::vector<CMenuBar::Menu> menus;
		menus.push_back({EditorTexts::get("menu.file"), [this]()
		{
			std::vector<CPopupMenu::Entry> entries;
			entries.push_back(makeEntry(EditorTexts::get("menu.new"), true, [this]() { newTemplates(); }));
			entries.push_back(makeEntry(EditorTexts::get("menu.open"), true, [this]() { openDialog(); }));
			entries.push_back(makeEntry(EditorTexts::get("menu.save"), !templates.empty(), [this]() { save(); }));
			entries.push_back(makeEntry(EditorTexts::get("menu.saveAs"), !templates.empty(), [this]() { saveAs(); }));
			entries.push_back(CPopupMenu::Entry::makeSeparator());
			entries.push_back(makeEntry(EditorTexts::get("menu.exit"), true, [this]() { closeEditor(); }));
			return entries;
		}});

		menus.push_back({EditorTexts::get("menu.edit"), [this]()
		{
			std::vector<CPopupMenu::Entry> entries;
			entries.push_back(makeEntry(EditorTexts::get("template.addZone"), !templates.empty(), [this]() { addZone(); }));
			entries.push_back(makeEntry(EditorTexts::get("template.removeZone"), selectedZone >= 0, [this]() { removeZone(); }));
			entries.push_back(makeEntry(EditorTexts::get("template.autoPosition"), !templates.empty(), [this]()
			{
				autoPositionZones();
				view->autoFit();
				changed();
			}));
			return entries;
		}});

		menus.push_back({EditorTexts::get("menu.view"), [this]()
		{
			std::vector<CPopupMenu::Entry> entries;
			entries.push_back(makeEntry(EditorTexts::get("template.zoomIn"), true, [this]() { view->zoom(1); }));
			entries.push_back(makeEntry(EditorTexts::get("template.zoomOut"), true, [this]() { view->zoom(-1); }));
			entries.push_back(makeEntry(EditorTexts::get("template.zoomFit"), true, [this]() { view->autoFit(); }));
			entries.push_back(makeEntry(EditorTexts::get("template.zoomReset"), true, [this]() { view->resetZoom(); }));
			return entries;
		}});

		menuBar = std::make_shared<CMenuBar>(Rect(0, 0, screen.x, MENU_HEIGHT), menus);
		titleLabel = std::make_shared<CLabel>(screen.x - 8, MENU_HEIGHT / 2, UiTheme::FONT, ETextAlignment::CENTERRIGHT, UiTheme::TEXT, "");
	}

	{
		OBJECT_CONSTRUCTION_TARGETED(statusPanel.get());
		statusLabel = std::make_shared<CLabel>(8, STATUS_HEIGHT / 2, UiTheme::FONT, ETextAlignment::CENTERLEFT, UiTheme::TEXT, "");
	}

	{
		OBJECT_CONSTRUCTION_TARGETED(sidePanel.get());

		const int tabHeight = UiTheme::CONTROL_HEIGHT + 4;
		tabs = std::make_shared<CTabBar>(Rect(UiTheme::SPACING, UiTheme::SPACING, PANEL_WIDTH - 2 * UiTheme::SPACING, tabHeight),
			std::vector<std::string>{EditorTexts::get("template.tabTemplate"), EditorTexts::get("template.tabZone"), EditorTexts::get("template.tabConnections")},
			0, [this](int index) { selectPage(index); });

		const Rect pageArea(UiTheme::SPACING, 2 * UiTheme::SPACING + tabHeight, PANEL_WIDTH - 2 * UiTheme::SPACING, screen.y - MENU_HEIGHT - STATUS_HEIGHT - tabHeight - 3 * UiTheme::SPACING);
		for(auto & page : pages)
			page = std::make_shared<CFormView>(pageArea, LABEL_WIDTH);
	}

	view->onZoneSelected = [this](TRmgTemplateZoneId zone)
	{
		selectedZone = zone;
		selectedConnection = -1;
		buildZonePage();
		if(zone >= 0)
			selectPage(1);
	};
	view->onZoneMoved = [this](TRmgTemplateZoneId)
	{
		changed();
	};
	view->onConnectionSelected = [this](int connection)
	{
		selectedConnection = connection;
		selectedZone = -1;
		buildConnectionsPage();
		selectPage(2);
	};

	initContent();
	loadContent(false);
	selectPage(0);
	updateTitle();
}

void EditorTemplateWindow::updateTitle()
{
	const std::string name = filename.empty() ? "" : TextOperations::filesystemPathToUtf8(filename.filename());
	titleLabel->setText(name + (unsaved ? "*" : "") + " - " + EditorTexts::get("template.windowTitle") + " (" + GameConstants::VCMI_VERSION + ")");
}

void EditorTemplateWindow::changed()
{
	unsaved = true;
	updateTitle();
	view->redraw();
}

void EditorTemplateWindow::setStatus(const std::string & text)
{
	statusLabel->setText(text);
}

bool EditorTemplateWindow::confirmDiscard(const std::function<void()> & next)
{
	if(!unsaved)
	{
		next();
		return true;
	}

	CMessageDialog::askConfirmation(EditorTexts::get("confirm.title"), EditorTexts::get("confirm.unsaved"), next);
	return false;
}

CRmgTemplate & EditorTemplateWindow::currentTemplate() const
{
	return *templates.at(selectedTemplate);
}

rmg::ZoneOptions & EditorTemplateWindow::currentZone() const
{
	return *currentTemplate().getZones().at(selectedZone);
}

void EditorTemplateWindow::selectPage(int index)
{
	for(int i = 0; i < TAB_COUNT; ++i)
		pages[i]->setEnabled(i == index);

	tabs->setSelected(index);
	redraw();
}

void EditorTemplateWindow::initContent()
{
	selectedTemplate = templates.empty() ? "" : templates.begin()->first;
}

void EditorTemplateWindow::loadContent(bool autoPosition)
{
	selectedZone = -1;
	selectedConnection = -1;

	if(templates.empty())
	{
		view->setTemplate(nullptr);
	}
	else
	{
		const auto & zones = currentTemplate().getZones();
		const bool allAtOrigin = std::ranges::all_of(zones, [](const auto & item)
		{
			return item.second->getVisiblePosition().x == 0 && item.second->getVisiblePosition().y == 0;
		});

		if(!zones.empty() && (autoPosition || allAtOrigin))
			autoPositionZones();

		view->setTemplate(templates.at(selectedTemplate));
		view->autoFit();
	}

	buildTemplatePage();
	buildZonePage();
	buildConnectionsPage();
}

void EditorTemplateWindow::autoPositionZones()
{
	auto & zones = currentTemplate().getZones();
	if(zones.empty())
		return;

	std::vector<GeometryAlgorithm::Node> nodes;
	std::default_random_engine rng(0);
	std::uniform_real_distribution<double> distribution(0.0, LAYOUT_SIZE);
	for(const auto & item : zones)
	{
		GeometryAlgorithm::Node node;
		node.x = distribution(rng);
		node.y = distribution(rng);
		node.id = item.first;
		nodes.push_back(node);
	}

	std::vector<GeometryAlgorithm::Edge> edges;
	for(const auto & connection : currentTemplate().getConnectedZoneIds())
	{
		const auto from = vstd::find_pos_if(nodes, [&connection](const auto & node) { return node.id == connection.getZoneA(); });
		const auto to = vstd::find_pos_if(nodes, [&connection](const auto & node) { return node.id == connection.getZoneB(); });
		if(from >= nodes.size() || to >= nodes.size())
			continue;

		edges.push_back({static_cast<int>(from), static_cast<int>(to)});
	}

	GeometryAlgorithm::forceDirectedLayout(nodes, edges, LAYOUT_ITERATIONS, LAYOUT_SIZE, LAYOUT_SIZE);

	for(const auto & node : nodes)
	{
		const int grid = EditorTemplateView::GRID_SIZE;
		zones.at(node.id)->setVisiblePosition(Point(grid * static_cast<int>(std::lround(node.x / grid)), grid * static_cast<int>(std::lround(node.y / grid))));
	}
}

void EditorTemplateWindow::setDefaultZone(rmg::ZoneOptions & zone, TRmgTemplateZoneId id)
{
	// the next free player slot, which is stored in the owner as 1 .. PLAYER_LIMIT_I
	std::set<int> used;
	for(const auto & [zoneId, other] : currentTemplate().getZones())
	{
		if(zoneId == id)
			continue;

		if(const auto owner = other->getOwner())
			used.insert(*owner);
	}

	int nextOwner = -1;
	for(int player = 1; player <= PlayerColor::PLAYER_LIMIT_I; ++player)
	{
		if(!used.count(player))
		{
			nextOwner = player;
			break;
		}
	}

	if(nextOwner != -1)
	{
		zone.setType(ETemplateZoneType::PLAYER_START);
		zone.owner = nextOwner;
	}
	else
	{
		// all the player slots are used
		zone.setType(ETemplateZoneType::TREASURE);
		zone.owner = std::nullopt;
	}
}

void EditorTemplateWindow::selectZone(TRmgTemplateZoneId zone)
{
	selectedZone = zone;
	selectedConnection = -1;
	view->setSelectedZone(zone);
	buildZonePage();
}

// ---------------------------------------------------------------------------------------------
// pages

void EditorTemplateWindow::buildTemplatePage()
{
	CFormView & page = *pages[0];
	page.clear();

	if(templates.empty())
	{
		page.addParagraph(EditorTexts::get("template.noTemplate"));
		page.finish();
		return;
	}

	CRmgTemplate & tpl = currentTemplate();
	const int controlWidth = page.contentWidth() - LABEL_WIDTH;

	std::vector<std::string> names;
	int selected = 0;
	for(const auto & [name, entry] : templates)
	{
		if(name == selectedTemplate)
			selected = static_cast<int>(names.size());
		names.push_back(name);
	}

	page.addChoice(EditorTexts::get("template.selected"), names, selected, [this, names](int index)
	{
		selectedTemplate = names.at(index);
		loadContent(false);
	});
	page.addButton(EditorTexts::get("template.subTemplates"), EditorTexts::get("template.add"), [this]() { addTemplate(); });
	page.addButton("", EditorTexts::get("template.remove"), [this]() { removeTemplate(); });
	page.addButton("", EditorTexts::get("template.rename"), [this]() { renameTemplate(); });

	page.addText(EditorTexts::get("template.name"), tpl.name, [this](const std::string & text) { currentTemplate().name = text; changed(); });
	page.addText(EditorTexts::get("template.description"), tpl.description, [this](const std::string & text) { currentTemplate().description = text; changed(); });

	const auto addSize = [&](const std::string & label, int3 & size)
	{
		int3 * target = &size;
		page.addNumber(label + " X", size.x, 0, MAX_SIZE, [this, target](int value) { target->x = value; changed(); });
		page.addNumber(label + " Y", size.y, 0, MAX_SIZE, [this, target](int value) { target->y = value; changed(); });
		page.addNumber(label + " Z", size.z, 0, MAX_SIZE, [this, target](int value) { target->z = value; changed(); });
	};
	addSize(EditorTexts::get("template.minSize"), tpl.minSize);
	addSize(EditorTexts::get("template.maxSize"), tpl.maxSize);

	const auto addWater = [&](const std::string & label, EWaterContent::EWaterContent content)
	{
		page.addCheck(label, vstd::contains(tpl.allowedWaterContent, content), [this, content](bool on)
		{
			if(on)
				currentTemplate().allowedWaterContent.insert(content);
			else
				currentTemplate().allowedWaterContent.erase(content);
			changed();
		});
	};
	addWater(EditorTexts::get("template.waterNone"), EWaterContent::NONE);
	addWater(EditorTexts::get("template.waterNormal"), EWaterContent::NORMAL);
	addWater(EditorTexts::get("template.waterIslands"), EWaterContent::ISLANDS);

	const auto addRanges = [&](const std::string & label, std::vector<std::pair<int, int>> & ranges)
	{
		auto list = std::make_shared<CListEditor>(Rect(0, 0, controlWidth, LIST_HEIGHT), true);
		std::vector<std::pair<int, int>> * target = &ranges;
		auto refresh = std::make_shared<std::function<void(int)>>();
		auto listPointer = list.get();
		*refresh = [target, listPointer](int selectedRange)
		{
			std::vector<std::string> entries;
			for(const auto & range : *target)
				entries.push_back(std::to_string(range.first) + " - " + std::to_string(range.second));
			listPointer->setEntries(entries, selectedRange);
		};

		list->onAdd = [this, target, refresh]()
		{
			target->emplace_back(0, 0);
			(*refresh)(static_cast<int>(target->size()) - 1);
			changed();
		};
		list->onEdit = [this, target, refresh](int index)
		{
			ENGINE->windows().createAndPushWindow<RangeDialog>(target->at(index), [this, target, refresh, index](const std::pair<int, int> & range)
			{
				target->at(index) = range;
				(*refresh)(index);
				changed();
			});
		};
		list->onRemove = [this, target, refresh](int index)
		{
			target->erase(target->begin() + index);
			(*refresh)(-1);
			changed();
		};
		(*refresh)(-1);
		page.addWidget(label, list);
	};
	addRanges(EditorTexts::get("template.players"), tpl.players.range);
	addRanges(EditorTexts::get("template.humanPlayers"), tpl.humanPlayers.range);

	page.addButton(EditorTexts::get("template.bannedSpells"), EditorTexts::get("inspector.edit"), [this]()
	{
		EditorTemplateDialogs::editSpells(EditorTexts::get("template.bannedSpells"), currentTemplate().bannedSpells);
	});
	page.addButton(EditorTexts::get("template.bannedArtifacts"), EditorTexts::get("inspector.edit"), [this]()
	{
		EditorTemplateDialogs::editArtifacts(EditorTexts::get("template.bannedArtifacts"), currentTemplate().bannedArtifacts);
	});
	page.addButton(EditorTexts::get("template.bannedSkills"), EditorTexts::get("inspector.edit"), [this]()
	{
		EditorTemplateDialogs::editSkills(EditorTexts::get("template.bannedSkills"), currentTemplate().bannedSkills);
	});
	page.addButton(EditorTexts::get("template.bannedHeroes"), EditorTexts::get("inspector.edit"), [this]()
	{
		EditorTemplateDialogs::editHeroes(EditorTexts::get("template.bannedHeroes"), currentTemplate().bannedHeroes);
	});

	page.finish();
}

void EditorTemplateWindow::refreshCards()
{
	view->redraw();
	changed();
}

void EditorTemplateWindow::buildZonePage()
{
	CFormView & page = *pages[1];
	page.clear();

	if(templates.empty() || selectedZone < 0 || !currentTemplate().getZones().count(selectedZone))
	{
		page.addParagraph(EditorTexts::get("template.noZone"));
		page.finish();
		return;
	}

	rmg::ZoneOptions & zone = currentZone();
	rmg::ZoneOptions * zonePointer = &zone;

	page.addInfo(EditorTexts::get("template.zoneId"), std::to_string(zone.id));

	page.addNumber(EditorTexts::get("template.visibleX"), zone.getVisiblePosition().x, -MAX_SIZE * PERCENT, MAX_SIZE * PERCENT, [this, zonePointer](int value)
	{
		zonePointer->setVisiblePosition(Point(value, zonePointer->getVisiblePosition().y));
		refreshCards();
	});
	page.addNumber(EditorTexts::get("template.visibleY"), zone.getVisiblePosition().y, -MAX_SIZE * PERCENT, MAX_SIZE * PERCENT, [this, zonePointer](int value)
	{
		zonePointer->setVisiblePosition(Point(zonePointer->getVisiblePosition().x, value));
		refreshCards();
	});
	// the size of the card is stored as a factor
	page.addNumber(EditorTexts::get("template.visibleSize"), static_cast<int>(std::lround(zone.getVisibleSize() * PERCENT)), MIN_VISIBLE_SIZE_PERCENT, MAX_VISIBLE_SIZE_PERCENT, [this, zonePointer](int value)
	{
		zonePointer->setVisibleSize(static_cast<float>(value) / PERCENT);
		refreshCards();
	});

	std::vector<std::string> typeNames{EditorTexts::get("template.zonePlayer"), EditorTexts::get("template.zoneCpu"), EditorTexts::get("template.zoneTreasure"),
		EditorTexts::get("template.zoneJunction"), EditorTexts::get("template.zoneWater"), EditorTexts::get("template.zoneSealed")};
	page.addChoice(EditorTexts::get("template.zoneType"), typeNames, positionOf(ZONE_TYPES, zone.getType()), [this, zonePointer](int index)
	{
		const ETemplateZoneType type = ZONE_TYPES.at(index);
		zonePointer->setType(type);

		if(type == ETemplateZoneType::PLAYER_START || type == ETemplateZoneType::CPU_START)
		{
			if(!zonePointer->owner)
				zonePointer->owner = 1;
		}
		else
			zonePointer->owner = std::nullopt;

		refreshCards();
		buildZonePage();
	});

	const bool hasOwner = zone.getType() == ETemplateZoneType::PLAYER_START || zone.getType() == ETemplateZoneType::CPU_START;
	if(hasOwner)
	{
		std::vector<std::string> owners;
		for(int i = 0; i < PlayerColor::PLAYER_LIMIT_I; ++i)
			owners.push_back(EditorTexts::playerName(PlayerColor(i)));

		page.addChoice(EditorTexts::get("template.zoneOwner"), owners, zone.getOwner().value_or(1) - 1, [this, zonePointer](int index)
		{
			zonePointer->owner = index + 1;
			refreshCards();
		});
	}

	page.addChoice(EditorTexts::get("template.forcedLevel"), {EditorTexts::get("template.levelAutomatic"), EditorTexts::get("template.levelSurface"), EditorTexts::get("template.levelUnderground")},
		positionOf(ZONE_LEVELS, zone.getForcedLevel()), [this, zonePointer](int index)
		{
			zonePointer->setForcedLevel(ZONE_LEVELS.at(index));
			changed();
		});
	page.addNumber(EditorTexts::get("template.zoneSize"), zone.getSize(), 0, MAX_ZONE_SIZE, [this, zonePointer](int value) { zonePointer->setSize(value); changed(); });

	page.addChoice(EditorTexts::get("template.monsterStrength"),
		{EditorTexts::get("template.strengthNone"), EditorTexts::get("template.strengthRandom"), EditorTexts::get("template.strengthWeak"), EditorTexts::get("template.strengthNormal"), EditorTexts::get("template.strengthStrong")},
		positionOf(MONSTER_STRENGTHS, zone.monsterStrength), [this, zonePointer](int index)
		{
			zonePointer->monsterStrength = MONSTER_STRENGTHS.at(index);
			refreshCards();
		});

	const auto addTowns = [&](const std::string & prefix, rmg::ZoneOptions::CTownInfo & towns)
	{
		rmg::ZoneOptions::CTownInfo * target = &towns;
		page.addNumber(prefix + " " + EditorTexts::get("template.townCount"), towns.townCount, 0, MAX_TOWNS, [this, target](int value) { target->townCount = value; changed(); });
		page.addNumber(prefix + " " + EditorTexts::get("template.castleCount"), towns.castleCount, 0, MAX_TOWNS, [this, target](int value) { target->castleCount = value; changed(); });
		page.addNumber(prefix + " " + EditorTexts::get("template.townDensity"), towns.townDensity, 0, MAX_DENSITY, [this, target](int value) { target->townDensity = value; changed(); });
		page.addNumber(prefix + " " + EditorTexts::get("template.castleDensity"), towns.castleDensity, 0, MAX_DENSITY, [this, target](int value) { target->castleDensity = value; changed(); });
	};
	addTowns(EditorTexts::get("template.playerTowns"), zone.playerTowns);
	addTowns(EditorTexts::get("template.neutralTowns"), zone.neutralTowns);

	page.addCheck(EditorTexts::get("template.matchTerrain"), zone.matchTerrainToTown, [this, zonePointer](bool on) { zonePointer->matchTerrainToTown = on; changed(); });
	page.addCheck(EditorTexts::get("template.sameTowns"), zone.townsAreSameType, [this, zonePointer](bool on) { zonePointer->townsAreSameType = on; changed(); });

	// each of the properties can be copied from another zone
	const auto addLink = [&](const std::string & label, TRmgTemplateZoneId & link)
	{
		TRmgTemplateZoneId * target = &link;
		const bool linked = link != rmg::ZoneOptions::NO_ZONE;
		page.addCheck(label, linked, [this, target](bool on)
		{
			*target = on ? 0 : rmg::ZoneOptions::NO_ZONE;
			changed();
			buildZonePage();
		});
		if(linked)
			page.addNumber(label + " #", link, 0, MAX_ZONE_ID, [this, target](int value) { *target = value; changed(); });
	};
	addLink(EditorTexts::get("template.linkTowns"), zone.townsLikeZone);
	addLink(EditorTexts::get("template.linkMines"), zone.minesLikeZone);
	addLink(EditorTexts::get("template.linkTerrain"), zone.terrainTypeLikeZone);
	addLink(EditorTexts::get("template.linkTreasure"), zone.treasureLikeZone);
	addLink(EditorTexts::get("template.linkObjects"), zone.customObjectsLikeZone);

	const auto addEdit = [&](const std::string & label, const std::function<void()> & opener)
	{
		page.addButton(label, EditorTexts::get("inspector.edit"), opener);
	};
	addEdit(EditorTexts::get("template.terrains"), [zonePointer]() { EditorTemplateDialogs::editTerrains(EditorTexts::get("template.terrains"), zonePointer->terrainTypes); });
	addEdit(EditorTexts::get("template.bannedTerrains"), [zonePointer]() { EditorTemplateDialogs::editTerrains(EditorTexts::get("template.bannedTerrains"), zonePointer->bannedTerrains); });
	addEdit(EditorTexts::get("template.allowedTowns"), [zonePointer]() { EditorTemplateDialogs::editFactions(EditorTexts::get("template.allowedTowns"), zonePointer->townTypes); });
	addEdit(EditorTexts::get("template.bannedTowns"), [zonePointer]() { EditorTemplateDialogs::editFactions(EditorTexts::get("template.bannedTowns"), zonePointer->bannedTownTypes); });
	addEdit(EditorTexts::get("template.townHints"), [zonePointer]() { EditorTemplateDialogs::editTownHints(zonePointer->townHints); });
	addEdit(EditorTexts::get("template.allowedMonsters"), [zonePointer]() { EditorTemplateDialogs::editFactions(EditorTexts::get("template.allowedMonsters"), zonePointer->monsterTypes); });
	addEdit(EditorTexts::get("template.bannedMonsters"), [zonePointer]() { EditorTemplateDialogs::editFactions(EditorTexts::get("template.bannedMonsters"), zonePointer->bannedMonsters); });
	addEdit(EditorTexts::get("template.treasures"), [zonePointer]() { EditorTemplateDialogs::editTreasures(zonePointer->treasureInfo); });
	addEdit(EditorTexts::get("template.mines"), [zonePointer]() { EditorTemplateDialogs::editMines(zonePointer->mines); });
	addEdit(EditorTexts::get("template.objects"), [this, zonePointer]()
	{
		ENGINE->windows().createAndPushWindow<EditorTemplateObjectDialog>(zonePointer->objectConfig, callback.get(), [this]() { changed(); });
	});

	page.finish();
}

void EditorTemplateWindow::buildConnectionsPage()
{
	CFormView & page = *pages[2];
	page.clear();

	if(templates.empty())
	{
		page.addParagraph(EditorTexts::get("template.noTemplate"));
		page.finish();
		return;
	}

	auto & connections = currentTemplate().connections;
	const int controlWidth = page.contentWidth() - LABEL_WIDTH;

	auto list = std::make_shared<CListEditor>(Rect(0, 0, controlWidth + LABEL_WIDTH - UiTheme::SLIDER_WIDTH, 2 * LIST_HEIGHT), false);
	std::vector<std::string> entries;
	for(const auto & connection : connections)
		entries.push_back(EditorTexts::format("template.connectionEntry", {std::to_string(connection.getZoneA()), std::to_string(connection.getZoneB()), connectionTypeName(connection.getConnectionType())}));
	list->setEntries(entries, selectedConnection);

	list->onAdd = [this]()
	{
		const auto & zones = currentTemplate().getZones();
		if(zones.size() < 2)
		{
			CMessageDialog::showMessage(EditorTexts::get("template.tooFewZones"), EditorTexts::get("template.tooFewZonesText"));
			return;
		}

		rmg::ZoneConnection connection;
		auto zoneIt = zones.begin();
		connection.zoneA = zoneIt->first;
		++zoneIt;
		connection.zoneB = zoneIt->first;
		currentTemplate().connections.push_back(connection);

		selectedConnection = static_cast<int>(currentTemplate().connections.size()) - 1;
		view->setSelectedConnection(selectedConnection);
		changed();
		buildConnectionsPage();
	};
	list->onRemove = [this](int index)
	{
		currentTemplate().connections.erase(currentTemplate().connections.begin() + index);
		selectedConnection = -1;
		view->setSelectedConnection(-1);
		changed();
		buildConnectionsPage();
	};
	list->onSelect = [this](int index)
	{
		if(index == selectedConnection)
			return;

		selectedConnection = index;
		view->setSelectedConnection(index);
		buildConnectionsPage();
	};
	page.addWidget("", list);

	if(selectedConnection >= 0 && selectedConnection < static_cast<int>(connections.size()))
	{
		rmg::ZoneConnection * connection = &connections.at(selectedConnection);

		page.addNumber(EditorTexts::get("template.zoneA"), connection->zoneA, 0, MAX_ZONE_ID, [this, connection](int value) { connection->zoneA = value; changed(); });
		page.addNumber(EditorTexts::get("template.zoneB"), connection->zoneB, 0, MAX_ZONE_ID, [this, connection](int value) { connection->zoneB = value; changed(); });
		page.addNumber(EditorTexts::get("template.guard"), connection->guardStrength, 0, MAX_GUARD, [this, connection](int value) { connection->guardStrength = value; changed(); });

		std::vector<std::string> typeNames;
		for(const auto & type : CONNECTION_TYPES)
			typeNames.push_back(connectionTypeName(type));
		page.addChoice(EditorTexts::get("template.connectionType"), typeNames, positionOf(CONNECTION_TYPES, connection->connectionType), [this, connection](int index)
		{
			connection->connectionType = CONNECTION_TYPES.at(index);
			changed();
		});

		page.addChoice(EditorTexts::get("template.road"), {EditorTexts::get("template.roadRandom"), EditorTexts::get("template.roadYes"), EditorTexts::get("template.roadNo")},
			positionOf(ROAD_OPTIONS, connection->hasRoad), [this, connection](int index)
			{
				connection->hasRoad = ROAD_OPTIONS.at(index);
				changed();
			});
	}

	page.finish();
}

// ---------------------------------------------------------------------------------------------
// actions

bool EditorTemplateWindow::validate()
{
	const std::string title = EditorTexts::get("template.validationFailed");

	for(const auto & [name, tpl] : templates)
	{
		if(tpl->players.range.empty())
		{
			CMessageDialog::showMessage(title, EditorTexts::get("template.noPlayerRange"));
			return false;
		}

		for(const auto & range : tpl->players.range)
		{
			if(range.second < range.first || range.first < 1)
			{
				CMessageDialog::showMessage(title, EditorTexts::get("template.invalidPlayerRange"));
				return false;
			}
		}

		for(const auto & range : tpl->humanPlayers.range)
		{
			if(range.second < range.first)
			{
				CMessageDialog::showMessage(title, EditorTexts::get("template.invalidHumanRange"));
				return false;
			}
		}

		const auto & connections = tpl->connections;
		const auto & zones = tpl->getZones();

		for(const auto & connection : connections)
		{
			if(!zones.count(connection.getZoneA()) || !zones.count(connection.getZoneB()))
			{
				CMessageDialog::showMessage(title, EditorTexts::format("template.missingZones", {std::to_string(connection.getZoneA()), std::to_string(connection.getZoneB())}));
				return false;
			}
		}

		for(const auto & [zoneId, zone] : zones)
		{
			const bool hasConnection = std::ranges::any_of(connections, [zoneId](const rmg::ZoneConnection & connection)
			{
				return connection.getZoneA() == zoneId || connection.getZoneB() == zoneId;
			});

			if(!hasConnection)
			{
				CMessageDialog::showMessage(title, EditorTexts::format("template.zoneWithoutConnection", {std::to_string(zoneId)}));
				return false;
			}
		}

		// every player of the biggest configuration needs exactly one start zone
		int maxPlayers = 0;
		for(const auto & range : tpl->players.range)
			maxPlayers = std::max(maxPlayers, range.second);

		for(int player = 1; player <= maxPlayers; ++player)
		{
			int count = 0;
			for(const auto & [zoneId, zone] : zones)
				if(zone->getType() == ETemplateZoneType::PLAYER_START && zone->getOwner() == player)
					++count;

			if(count != 1)
			{
				CMessageDialog::showMessage(title, EditorTexts::format("template.playerStartZones", {std::to_string(player), std::to_string(count)}));
				return false;
			}
		}
	}

	return true;
}

void EditorTemplateWindow::newTemplates()
{
	confirmDiscard([this]()
	{
		templates.clear();
		templates[DEFAULT_TEMPLATE_NAME] = std::make_shared<CRmgTemplate>();
		templates.at(DEFAULT_TEMPLATE_NAME)->players.range = {{1, 2}};

		filename.clear();
		initContent();
		loadContent(false);
		changed();
	});
}

void EditorTemplateWindow::openDialog()
{
	confirmDiscard([this]()
	{
		CFileDialog::openFile(EditorTexts::get("template.openTitle"), {"json"}, [this](const boost::filesystem::path & file) { openTemplates(file); });
	});
}

void EditorTemplateWindow::openTemplates(const boost::filesystem::path & file)
{
	try
	{
		templates = EditorFiles::openTemplate(file);
	}
	catch(const std::exception & e)
	{
		CMessageDialog::showMessage(EditorTexts::get("template.openFailed"), e.what());
		return;
	}

	filename = file;
	unsaved = false;
	initContent();
	loadContent(false);
	updateTitle();
}

void EditorTemplateWindow::save()
{
	if(templates.empty())
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
		EditorFiles::saveTemplate(templates, filename);
	}
	catch(const std::exception & e)
	{
		CMessageDialog::showMessage(EditorTexts::get("template.saveFailed"), e.what());
		return;
	}

	unsaved = false;
	updateTitle();
}

void EditorTemplateWindow::saveAs()
{
	if(templates.empty() || !validate())
		return;

	const std::string defaultName = filename.empty() ? "template" : TextOperations::filesystemPathToUtf8(filename.stem());
	CFileDialog::saveFile(EditorTexts::get("template.saveTitle"), "json", defaultName, [this](const boost::filesystem::path & file)
	{
		filename = file;
		save();
	});
}

void EditorTemplateWindow::addZone()
{
	if(templates.empty())
		return;

	auto & zones = currentTemplate().zones;
	for(int id = 1; id < MAX_ZONE_ID; ++id)
	{
		if(zones.count(id))
			continue;

		auto zone = std::make_shared<rmg::ZoneOptions>();
		zones[id] = zone;
		setDefaultZone(*zone, id);

		// the new card appears in the middle of the view
		const int grid = EditorTemplateView::GRID_SIZE;
		const Point center = view->getViewCenter();
		zone->setVisiblePosition(Point(grid * static_cast<int>(std::lround(double(center.x) / grid)), grid * static_cast<int>(std::lround(double(center.y) / grid))));

		changed();
		selectZone(id);
		return;
	}

	setStatus(EditorTexts::get("template.noFreeZoneId"));
}

void EditorTemplateWindow::removeZone()
{
	if(templates.empty() || selectedZone < 0)
		return;

	currentTemplate().zones.erase(selectedZone);
	selectZone(-1);
	changed();
}

void EditorTemplateWindow::addTemplate()
{
	CTextPrompt::show(EditorTexts::get("template.enterName"), EditorTexts::get("template.name"), "", [this](const std::string & name)
	{
		if(name.empty())
			return;

		if(templates.count(name))
		{
			CMessageDialog::showMessage(EditorTexts::get("template.alreadyExists"), EditorTexts::get("template.alreadyExistsText"));
			return;
		}

		selectedTemplate = name;
		templates[name] = std::make_shared<CRmgTemplate>();
		templates.at(name)->players.range = {{1, 2}};
		loadContent(false);
		changed();
	});
}

void EditorTemplateWindow::removeTemplate()
{
	if(templates.size() < 2)
	{
		CMessageDialog::showMessage(EditorTexts::get("template.tooFewTemplates"), EditorTexts::get("template.tooFewTemplatesText"));
		return;
	}

	templates.erase(selectedTemplate);
	selectedTemplate = templates.begin()->first;
	loadContent(false);
	changed();
}

void EditorTemplateWindow::renameTemplate()
{
	if(templates.empty())
		return;

	CTextPrompt::show(EditorTexts::get("template.enterName"), EditorTexts::get("template.name"), selectedTemplate, [this](const std::string & name)
	{
		if(name.empty() || name == selectedTemplate)
			return;

		if(templates.count(name))
		{
			CMessageDialog::showMessage(EditorTexts::get("template.alreadyExists"), EditorTexts::get("template.alreadyExistsText"));
			return;
		}

		templates[name] = templates.at(selectedTemplate);
		templates.erase(selectedTemplate);
		selectedTemplate = name;
		buildTemplatePage();
		changed();
	});
}

void EditorTemplateWindow::closeEditor()
{
	confirmDiscard([this]() { close(); });
}
