/*
 * EditorWindow.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "EditorWindow.h"

#include "EditorCampaignWindow.h"
#include "EditorConverters.h"
#include "EditorFiles.h"
#include "EditorInspector.h"
#include "EditorMapSettingsWindow.h"
#include "EditorMinimap.h"
#include "EditorNewMapWindow.h"
#include "EditorPlayerSettingsWindow.h"
#include "EditorResourceConverter.h"
#include "EditorSettings.h"
#include "EditorTemplateWindow.h"
#include "EditorTexts.h"
#include "EditorTranslationsWindow.h"
#include "EditorValidator.h"

#include "../GameEngine.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../widgets/CDropDown.h"
#include "../widgets/CMenuBar.h"
#include "../widgets/CPanel.h"
#include "../widgets/CSelectList.h"
#include "../widgets/CTabBar.h"
#include "../widgets/CTextButton.h"
#include "../widgets/CTextField.h"
#include "../widgets/CTextInput.h"
#include "../widgets/Images.h"
#include "../widgets/TextControls.h"
#include "../widgets/UiTheme.h"
#include "../windows/CChoiceDialog.h"
#include "../windows/CFileDialog.h"
#include "../windows/CPopupMenu.h"
#include "../windows/CPromptWindows.h"
#include "render/IImage.h"
#include "render/IRenderHandler.h"

#include "../../lib/CPlayerState.h"
#include "../../lib/GameLibrary.h"
#include "../../lib/MapLayerHandler.h"
#include "../../lib/RiverHandler.h"
#include "../../lib/RoadHandler.h"
#include "../../lib/TerrainHandler.h"
#include "../../lib/mapObjects/MapObjects.h"
#include "../../lib/mapObjects/ObjectTemplate.h"
#include "../../lib/mapping/CMap.h"
#include "../../lib/mapping/CMapService.h"
#include "../../lib/mapping/MapFormat.h"
#include "../../lib/modding/ModIncompatibility.h"
#include "../../lib/texts/MetaString.h"
#include "../../lib/texts/TextOperations.h"

#include <boost/algorithm/string/erase.hpp>

namespace
{
CPopupMenu::Entry makeEntry(const std::string & text, const std::string & shortcut, bool enabled, const std::function<void()> & callback, bool checked = false)
{
	CPopupMenu::Entry entry;
	entry.text = text;
	entry.shortcut = shortcut;
	entry.enabled = enabled;
	entry.checked = checked;
	entry.callback = callback;
	return entry;
}

std::string levelText(const CMap & map, int level)
{
	const std::string layer = map.mapLayers.at(level).toEntity(LIBRARY)->getNameTranslated();
	return EditorTexts::format("level.entry", {std::to_string(level + 1), layer});
}
}

EditorWindow::EditorWindow()
	: WindowBase(KEYBOARD | TIME)
{
	editor.setListener(this);
	catalog = std::make_unique<EditorObjectCatalog>(editor.getCallback());

	build();
}

EditorWindow::~EditorWindow()
{
	editor.setListener(nullptr);
}

void EditorWindow::build()
{
	OBJECT_CONSTRUCTION;
	built = false;

	const Point screen = ENGINE->screenDimensions();
	pos = Rect(0, 0, screen.x, screen.y);

	const int contentTop = MENU_HEIGHT + TOOLBAR_HEIGHT;
	const int contentHeight = screen.y - contentTop - STATUS_HEIGHT;

	mapView = std::make_shared<EditorMapView>(Rect(LEFT_WIDTH, contentTop, screen.x - LEFT_WIDTH - RIGHT_WIDTH, contentHeight), editor);
	topPanel = std::make_shared<CPanel>(Rect(0, 0, screen.x, contentTop));
	leftPanel = std::make_shared<CPanel>(Rect(0, contentTop, LEFT_WIDTH, contentHeight));
	rightPanel = std::make_shared<CPanel>(Rect(screen.x - RIGHT_WIDTH, contentTop, RIGHT_WIDTH, contentHeight));
	statusPanel = std::make_shared<CPanel>(Rect(0, screen.y - STATUS_HEIGHT, screen.x, STATUS_HEIGHT));

	buildMenu();
	buildToolbar();
	buildLeftPanel();
	buildRightPanel();

	{
		OBJECT_CONSTRUCTION_TARGETED(statusPanel.get());
		statusLabel = std::make_shared<CLabel>(8, STATUS_HEIGHT / 2, UiTheme::FONT, ETextAlignment::CENTERLEFT, UiTheme::TEXT, "");
		coordinateLabel = std::make_shared<CLabel>(screen.x - 480, STATUS_HEIGHT / 2, UiTheme::FONT, ETextAlignment::CENTERRIGHT, UiTheme::TEXT_HINT, "");

		playerDropDown = std::make_shared<CDropDown>(Rect(screen.x - 460, 0, 150, STATUS_HEIGHT), std::vector<std::string>(), -1, [this](int index)
		{
			switchDefaultPlayer(playerChoices.at(index));
		}, EditorTexts::get("player.title"));

		levelDropDown = std::make_shared<CDropDown>(Rect(screen.x - 300, 0, 296, STATUS_HEIGHT), std::vector<std::string>(), -1, [this](int index)
		{
			switchLevel(index);
		}, EditorTexts::get("level.title"));
	}

	mapView->onTileHovered = [this](const int3 & tile)
	{
		coordinateLabel->setText("x: " + std::to_string(tile.x) + "   y: " + std::to_string(tile.y));
	};
	mapView->onObjectOpened = [this](CGObjectInstance * object, bool switchTab)
	{
		if(switchTab)
			selectRightTab(1);
		inspector->showObject(object);
	};
	mapView->onPlacementRequested = [this](const int3 & tile) { tryPlacement(tile); };
	mapView->onPlacementCancelled = [this]() { setStatus(EditorTexts::get("status.placementCancelled")); };
	mapView->onViewChanged = [this]()
	{
		if(minimap)
			minimap->setViewport(mapView->getVisibleTiles());
	};
	minimap->onCenterRequested = [this](const Point & tile) { mapView->centerOnTile(tile); };

	inspector->onObjectChanged = [this](CGObjectInstance * object)
	{
		editor.commitObjectChange(mapView->getLevel(), object);
	};
	editor.pickObject = [this](const std::function<bool(const CGObjectInstance *)> & filter, const std::function<void(const CGObjectInstance *)> & callback)
	{
		mapView->startPicking(filter, callback);
		setStatus(EditorTexts::get("status.pickObject"), 5000);
	};

	built = true;

	mapView->reload();
	mapView->setGridVisible(toolbarButtons.at(ToolbarAction::GRID)->isHighlighted());
	mapView->setPassabilityVisible(toolbarButtons.at(ToolbarAction::PASSABILITY)->isHighlighted());
	rebuildLevelChoices();
	rebuildPlayerChoices();
	rebuildCatalog();
	rebuildPalette();
	minimap->setLevel(mapView->getLevel());
	updateToolbarState();
	updateTitle();
}

void EditorWindow::buildMenu()
{
	OBJECT_CONSTRUCTION_TARGETED(topPanel.get());
	const Point screen = ENGINE->screenDimensions();

	std::vector<CMenuBar::Menu> menus;

	menus.push_back({EditorTexts::get("menu.file"), [this]()
	{
		std::vector<CPopupMenu::Entry> recent;
		for(const auto & file : EditorSettings::getRecentFiles())
			recent.push_back(makeEntry(TextOperations::filesystemPathToUtf8(file), "", true, [this, file]() { confirmDiscard([this, file]() { openMap(file); }); }));

		recent.push_back(CPopupMenu::Entry::makeSeparator());
		recent.push_back(makeEntry(EditorTexts::get("menu.more"), "", true, [this]() { openRecentDialog(); }));

		CPopupMenu::Entry recentMenu = makeEntry(EditorTexts::get("menu.openRecent"), "", true, nullptr);
		recentMenu.children = recent;

		const bool hasMap = editor.hasMap();
		std::vector<CPopupMenu::Entry> entries;
		entries.push_back(makeEntry(EditorTexts::get("menu.new"), "Ctrl+N", true, [this]() { newMap(); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.open"), "Ctrl+O", true, [this]() { openMapDialog(); }));
		entries.push_back(recentMenu);
		entries.push_back(makeEntry(EditorTexts::get("menu.save"), "Ctrl+S", hasMap, [this]() { saveMap(false); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.saveAs"), "Ctrl+Shift+S", hasMap, [this]() { saveMapAs(); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.export"), "", hasMap, [this]() { exportImage(); }));
		entries.push_back(CPopupMenu::Entry::makeSeparator());
		entries.push_back(makeEntry(EditorTexts::get("menu.campaignEditor"), "", true, [this]() { openCampaignEditor(); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.templateEditor"), "", true, [this]() { openTemplateEditor(); }));
		entries.push_back(CPopupMenu::Entry::makeSeparator());
		entries.push_back(makeEntry(EditorTexts::get("menu.h3mConverter"), "", true, [this]() { EditorConverters::convertMaps(editor.getCallback()); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.h3cConverter"), "", true, []() { EditorConverters::convertCampaign(); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.resourceConverter"), "", true, []() { EditorResourceConverter::showDialog(); }));
		entries.push_back(CPopupMenu::Entry::makeSeparator());
		entries.push_back(makeEntry(EditorTexts::get("menu.exit"), "", true, [this]() { closeEditor(); }));
		return entries;
	}});

	menus.push_back({EditorTexts::get("menu.edit"), [this]()
	{
		const bool hasMap = editor.hasMap();
		const bool selected = hasMap && !editor.getObjectSelection(mapView->getLevel()).empty();

		std::vector<CPopupMenu::Entry> entries;
		entries.push_back(makeEntry(EditorTexts::get("menu.undo"), "Ctrl+Z", hasMap && !toolbarButtons.at(ToolbarAction::UNDO)->isBlocked(), [this]() { editor.undo(); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.redo"), "Ctrl+Y", hasMap && !toolbarButtons.at(ToolbarAction::REDO)->isBlocked(), [this]() { editor.redo(); }));
		entries.push_back(CPopupMenu::Entry::makeSeparator());
		entries.push_back(makeEntry(EditorTexts::get("menu.cut"), "Ctrl+X", selected, [this]() { cut(); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.copy"), "Ctrl+C", selected, [this]() { copy(); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.paste"), "Ctrl+V", hasMap, [this]() { paste(); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.erase"), "Del", selected, [this]() { erase(); }));
		entries.push_back(CPopupMenu::Entry::makeSeparator());
		entries.push_back(makeEntry(EditorTexts::get("menu.lock"), "", hasMap, [this]() { editor.lockAll(mapView->getLevel()); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.unlock"), "", hasMap, [this]() { editor.unlockAll(mapView->getLevel()); }));
		entries.push_back(CPopupMenu::Entry::makeSeparator());
		entries.push_back(makeEntry(EditorTexts::get("menu.fillObstacles"), "", hasMap, [this]() { editor.commitObstacleFill(mapView->getLevel()); }));
		return entries;
	}});

	menus.push_back({EditorTexts::get("menu.view"), [this]()
	{
		const bool hasMap = editor.hasMap();
		std::vector<CPopupMenu::Entry> entries;
		entries.push_back(makeEntry(EditorTexts::get("menu.grid"), "F6", hasMap, [this]() { toggleGrid(); }, mapView->isGridVisible()));
		entries.push_back(makeEntry(EditorTexts::get("menu.passability"), "F7", hasMap, [this]() { togglePassability(); }, mapView->isPassabilityVisible()));
		entries.push_back(makeEntry(EditorTexts::get("menu.toggleLevel"), "U", hasMap && editor.levels() > 1, [this]() { switchLevel((mapView->getLevel() + 1) % editor.levels()); }));
		entries.push_back(CPopupMenu::Entry::makeSeparator());
		entries.push_back(makeEntry(EditorTexts::get("menu.zoomIn"), "Keypad +", hasMap, [this]() { mapView->zoom(1); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.zoomOut"), "Keypad -", hasMap, [this]() { mapView->zoom(-1); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.zoomReset"), "Ctrl+0", hasMap, [this]() { mapView->resetZoom(); }));
		return entries;
	}});

	menus.push_back({EditorTexts::get("menu.map"), [this]()
	{
		const bool hasMap = editor.hasMap();
		std::vector<CPopupMenu::Entry> entries;
		entries.push_back(makeEntry(EditorTexts::get("menu.general"), "F4", hasMap, [this]() { showMapSettings(); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.players"), "", hasMap, [this]() { showPlayerSettings(); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.translations"), "", hasMap, [this]() { showTranslations(); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.validate"), "F5", hasMap, [this]() { validate(); }));
		entries.push_back(CPopupMenu::Entry::makeSeparator());
		entries.push_back(makeEntry(EditorTexts::get("menu.updateAppearance"), "", hasMap, [this]() { updateAppearance(); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.mapLayer"), "", hasMap, [this]() { selectMapLayer(); }));
		entries.push_back(makeEntry(EditorTexts::get("menu.addLevel"), "", hasMap, [this]() { addLevel(); }));
		return entries;
	}});

	menus.push_back({EditorTexts::get("menu.player"), [this]()
	{
		std::vector<CPopupMenu::Entry> entries;
		for(const auto & player : playerChoices)
			entries.push_back(makeEntry(EditorTexts::playerName(player), "", true, [this, player]() { switchDefaultPlayer(player); }, player == editor.defaultPlayer));
		return entries;
	}});

	menuBar = std::make_shared<CMenuBar>(Rect(0, 0, screen.x, MENU_HEIGHT), menus);
	titleLabel = std::make_shared<CLabel>(screen.x - 8, MENU_HEIGHT / 2, UiTheme::FONT, ETextAlignment::CENTERRIGHT, UiTheme::TEXT_HINT, "", screen.x - menuBar->getTitlesWidth() - 24);
}

void EditorWindow::buildToolbar()
{
	OBJECT_CONSTRUCTION_TARGETED(topPanel.get());

	struct ButtonInfo
	{
		ToolbarAction action;
		const char * icon;
		const char * hint;
		std::function<void()> callback;
	};

	const std::vector<std::vector<ButtonInfo>> groups = {
		{
			{ToolbarAction::NEW, "editor/document-new", "toolbar.new", [this]() { newMap(); }},
			{ToolbarAction::OPEN, "editor/document-open", "toolbar.open", [this]() { openMapDialog(); }},
			{ToolbarAction::SAVE, "editor/document-save", "toolbar.save", [this]() { saveMap(false); }},
		},
		{
			{ToolbarAction::UNDO, "editor/edit-undo", "toolbar.undo", [this]() { editor.undo(); }},
			{ToolbarAction::REDO, "editor/edit-redo", "toolbar.redo", [this]() { editor.redo(); }},
		},
		{
			{ToolbarAction::CUT, "editor/edit-cut", "toolbar.cut", [this]() { cut(); }},
			{ToolbarAction::COPY, "editor/edit-copy", "toolbar.copy", [this]() { copy(); }},
			{ToolbarAction::PASTE, "editor/edit-paste", "toolbar.paste", [this]() { paste(); }},
			{ToolbarAction::ERASE, "editor/edit-clear", "toolbar.erase", [this]() { erase(); }},
		},
		{
			{ToolbarAction::GRID, "editor/toggle-grid", "toolbar.grid", [this]() { toggleGrid(); }},
			{ToolbarAction::PASSABILITY, "editor/toggle-pass", "toolbar.passability", [this]() { togglePassability(); }},
		},
		{
			{ToolbarAction::LOCK, "editor/lock-closed", "toolbar.lock", [this]() { editor.lockAll(mapView->getLevel()); }},
			{ToolbarAction::UNLOCK, "editor/lock-open", "toolbar.unlock", [this]() { editor.unlockAll(mapView->getLevel()); }},
		},
		{
			{ToolbarAction::ZOOM_IN, "editor/zoom_plus", "toolbar.zoomIn", [this]() { mapView->zoom(1); }},
			{ToolbarAction::ZOOM_OUT, "editor/zoom_minus", "toolbar.zoomOut", [this]() { mapView->zoom(-1); }},
			{ToolbarAction::ZOOM_RESET, "editor/zoom_zero", "toolbar.zoomReset", [this]() { mapView->resetZoom(); }},
		},
		{
			{ToolbarAction::FILL_OBSTACLES, "editor/fill-obstacles", "toolbar.fillObstacles", [this]() { editor.commitObstacleFill(mapView->getLevel()); }},
			{ToolbarAction::TRANSLATIONS, "editor/translations", "toolbar.translations", [this]() { showTranslations(); }},
		},
	};

	toolbarButtons.clear();

	const int buttonSize = TOOLBAR_HEIGHT - 6;
	int x = 6;
	for(const auto & group : groups)
	{
		for(const auto & info : group)
		{
			toolbarButtons[info.action] = std::make_shared<CTextButton>(Rect(x, MENU_HEIGHT + 3, buttonSize, buttonSize), ImagePath::builtin(info.icon), EditorTexts::get(info.hint), info.callback);
			x += buttonSize + 2;
		}
		x += UiTheme::SPACING * 2;
	}
}

void EditorWindow::buildLeftPanel()
{
	OBJECT_CONSTRUCTION_TARGETED(leftPanel.get());

	const int minimapSize = LEFT_WIDTH - 16;
	minimap = std::make_shared<EditorMinimap>(Rect(8, 8, minimapSize, minimapSize), editor);

	struct ToolInfo
	{
		EditorMapView::Tool tool;
		const char * icon;
		const char * hint;
	};

	const std::vector<ToolInfo> tools = {
		{EditorMapView::Tool::SELECT, "editor/tool-select", "tool.select"},
		{EditorMapView::Tool::PAN, "editor/zoom_base", "tool.pan"},
		{EditorMapView::Tool::BRUSH, "editor/brush-1", "tool.brush"},
		{EditorMapView::Tool::BRUSH2, "editor/brush-2", "tool.brush2"},
		{EditorMapView::Tool::BRUSH4, "editor/brush-4", "tool.brush4"},
		{EditorMapView::Tool::LASSO, "editor/tool-lasso", "tool.lasso"},
		{EditorMapView::Tool::LINE, "editor/tool-line", "tool.line"},
		{EditorMapView::Tool::AREA, "editor/tool-area", "tool.area"},
		{EditorMapView::Tool::FILL, "editor/tool-fill", "tool.fill"},
	};

	toolButtons.clear();
	const int toolSize = 36;
	const int toolsTop = minimapSize + 16;
	const int perRow = std::max(1, (LEFT_WIDTH - 8) / (toolSize + 4));

	for(size_t i = 0; i < tools.size(); ++i)
	{
		const int column = static_cast<int>(i) % perRow;
		const int row = static_cast<int>(i) / perRow;
		const auto tool = tools[i].tool;

		toolButtons[tool] = std::make_shared<CTextButton>(Rect(8 + column * (toolSize + 4), toolsTop + row * (toolSize + 4), toolSize, toolSize),
			ImagePath::builtin(tools[i].icon), EditorTexts::get(tools[i].hint), [this, tool]() { setTool(tool); });
	}
	toolButtons.at(mapView->getTool())->setHighlighted(true);

	const int rows = (static_cast<int>(tools.size()) + perRow - 1) / perRow;
	const int paletteTop = toolsTop + rows * (toolSize + 4) + 6;
	const int paletteHeight = leftPanel->pos.h - paletteTop - 8;

	paletteTabs = std::make_shared<CTabBar>(Rect(8, paletteTop, LEFT_WIDTH - 16, UiTheme::CONTROL_HEIGHT),
		std::vector<std::string>{EditorTexts::get("palette.terrain"), EditorTexts::get("palette.roads"), EditorTexts::get("palette.rivers")}, paletteMode, [this](int index)
	{
		paletteMode = index;
		rebuildPalette();
	});

	paletteList = std::make_shared<CSelectList>(Rect(8, paletteTop + UiTheme::CONTROL_HEIGHT + 2, LEFT_WIDTH - 16, paletteHeight - UiTheme::CONTROL_HEIGHT - 2), CSelectList::Mode::SINGLE, UiTheme::ROW_HEIGHT);
	paletteList->onSelect = [this](int index) { paletteClicked(index); };
}

void EditorWindow::buildRightPanel()
{
	OBJECT_CONSTRUCTION_TARGETED(rightPanel.get());

	const int width = RIGHT_WIDTH - 16;
	const int tabHeight = UiTheme::CONTROL_HEIGHT + 4;
	const int areaTop = 8 + tabHeight + 4;
	const int areaHeight = rightPanel->pos.h - areaTop - 8;

	rightTabs = std::make_shared<CTabBar>(Rect(8, 8, width, tabHeight), std::vector<std::string>{EditorTexts::get("tab.objects"), EditorTexts::get("tab.inspector")}, 0, [this](int index)
	{
		selectRightTab(index);
	});

	objectsTab = std::make_shared<CIntObject>();
	objectsTab->pos = Rect(rightPanel->pos.x + 8, rightPanel->pos.y + areaTop, width, areaHeight);
	{
		OBJECT_CONSTRUCTION_TARGETED(objectsTab.get());

		catalogFilter = std::make_shared<CTextField>(Rect(0, 0, width, UiTheme::CONTROL_HEIGHT), "", [this](const std::string &) { rebuildCatalog(); });

		std::vector<std::string> terrainNames{EditorTexts::get("catalog.anyTerrain")};
		terrainChoices.clear();
		terrainChoices.emplace_back(ETerrainId::ANY_TERRAIN);
		for(const auto & terrain : LIBRARY->terrainTypeHandler->objects)
		{
			terrainNames.push_back(terrain->getNameTranslated());
			terrainChoices.push_back(terrain->getId());
		}
		terrainFilter = std::make_shared<CDropDown>(Rect(0, UiTheme::CONTROL_HEIGHT + 4, width, UiTheme::CONTROL_HEIGHT), terrainNames, 0, [this](int) { rebuildCatalog(); }, EditorTexts::get("catalog.terrainFilter"));

		const int previewHeight = 130;
		const int listTop = 2 * (UiTheme::CONTROL_HEIGHT + 4);
		catalogList = std::make_shared<CSelectList>(Rect(0, listTop, width, areaHeight - listTop - previewHeight - 4), CSelectList::Mode::SINGLE, UiTheme::ROW_HEIGHT);
		catalogList->onSelect = [this](int row) { catalogSelected(row); };
		catalogList->onBranch = [this](int row)
		{
			catalog->toggleExpanded(*catalogRows.at(row).node);
			rebuildCatalog();
		};

		previewLabel = std::make_shared<CLabel>(width / 2, areaHeight - previewHeight + 6, UiTheme::FONT, ETextAlignment::CENTER, UiTheme::TEXT_HINT, "", width);
	}

	inspector = std::make_shared<EditorInspector>(Rect(8, areaTop, width, areaHeight), editor);
	inspector->disable();
}

void EditorWindow::selectRightTab(int tab)
{
	rightTabs->setSelected(tab);

	objectsTab->setEnabled(tab == 0);
	inspector->setEnabled(tab == 1);
	rightPanel->redraw();
}

void EditorWindow::rebuildPalette()
{
	std::vector<CSelectList::Row> rows;

	switch(paletteMode)
	{
		case 0:
			for(const auto & terrain : LIBRARY->terrainTypeHandler->objects)
			{
				CSelectList::Row row;
				row.text = terrain->getNameTranslated();
				rows.push_back(std::move(row));
			}
			break;
		case 1:
			for(const auto & road : LIBRARY->roadTypeHandler->objects)
			{
				CSelectList::Row row;
				row.text = road->getNameTranslated();
				rows.push_back(std::move(row));
			}
			break;
		default:
			for(const auto & river : LIBRARY->riverTypeHandler->objects)
			{
				CSelectList::Row row;
				row.text = river->getNameTranslated();
				rows.push_back(std::move(row));
			}
			break;
	}

	paletteList->setRows(std::move(rows));
}

void EditorWindow::rebuildCatalog()
{
	const std::string filter = catalogFilter ? catalogFilter->getText() : "";
	const TerrainId terrain = terrainFilter && terrainFilter->getSelected() >= 0 ? terrainChoices.at(terrainFilter->getSelected()) : TerrainId(ETerrainId::ANY_TERRAIN);

	catalogRows = catalog->getRows(filter, terrain);

	std::vector<CSelectList::Row> rows;
	rows.reserve(catalogRows.size());
	for(const auto & entry : catalogRows)
	{
		CSelectList::Row row;
		row.text = entry.node->text;
		row.indent = entry.depth;
		if(!entry.node->leaf)
			row.branch = catalog->isExpanded(*entry.node) ? CSelectList::Branch::EXPANDED : CSelectList::Branch::COLLAPSED;
		rows.push_back(std::move(row));
	}

	catalogList->setRows(std::move(rows));
}

void EditorWindow::rebuildLevelChoices()
{
	std::vector<std::string> names;
	int selected = -1;

	if(editor.hasMap())
	{
		for(int i = 0; i < editor.levels(); ++i)
			names.push_back(levelText(*editor.getMap(), i));
		selected = std::min(mapView->getLevel(), editor.levels() - 1);
	}

	levelDropDown->setOptions(names, selected);
}

void EditorWindow::rebuildPlayerChoices()
{
	playerChoices.clear();
	std::vector<std::string> names;

	if(editor.hasMap())
	{
		playerChoices.push_back(PlayerColor::NEUTRAL);
		for(size_t i = 0; i < editor.getMap()->players.size(); ++i)
			if(editor.getMap()->players[i].canAnyonePlay())
				playerChoices.emplace_back(static_cast<int>(i));

		if(!vstd::contains(playerChoices, editor.defaultPlayer))
			editor.defaultPlayer = PlayerColor::NEUTRAL;
	}

	int selected = -1;
	for(size_t i = 0; i < playerChoices.size(); ++i)
	{
		names.push_back(EditorTexts::playerName(playerChoices[i]));
		if(playerChoices[i] == editor.defaultPlayer)
			selected = static_cast<int>(i);
	}

	playerDropDown->setOptions(names, selected);
}

void EditorWindow::updateToolbarState()
{
	const bool hasMap = editor.hasMap();

	for(auto & [action, button] : toolbarButtons)
	{
		switch(action)
		{
			case ToolbarAction::NEW:
			case ToolbarAction::OPEN:
				break;
			case ToolbarAction::UNDO:
			case ToolbarAction::REDO:
				// updated by onUndoStateChanged
				if(!hasMap)
					button->block(true);
				break;
			case ToolbarAction::CUT:
			case ToolbarAction::COPY:
			case ToolbarAction::ERASE:
				button->block(!hasMap || editor.getObjectSelection(mapView->getLevel()).empty());
				break;
			default:
				button->block(!hasMap);
				break;
		}
	}
}

void EditorWindow::updateTitle()
{
	std::string name = filename.empty() ? EditorTexts::get("title.untitled") : TextOperations::filesystemPathToUtf8(filename.filename());
	titleLabel->setText(name + (unsaved ? "*" : "") + " - " + EditorTexts::get("title.editor"));
}

void EditorWindow::setStatus(const std::string & text, int durationMs)
{
	statusLabel->setText(text);
	statusTimeLeft = durationMs;
}

void EditorWindow::showError(const std::string & title, const std::string & text)
{
	CMessageDialog::showMessage(title, text);
}

// ---------------------------------------------------------------------------------------------
// listener

void EditorWindow::onMapModified()
{
	unsaved = true;
	if(built)
		updateTitle();
}

void EditorWindow::onMapReloaded()
{
	if(!built)
		return;

	mapView->reload();
	minimap->setLevel(mapView->getLevel());
	inspector->clear();
	rebuildLevelChoices();
	rebuildPlayerChoices();
	updateToolbarState();
	updateTitle();
	minimap->setViewport(mapView->getVisibleTiles());
}

void EditorWindow::onContentChanged()
{
	if(built)
		minimap->refresh();
}

void EditorWindow::onObjectRemoving(const CGObjectInstance * object)
{
	if(built)
		mapView->objectRemoving(object);
}

void EditorWindow::onObjectAdded(const CGObjectInstance * object)
{
	if(built)
		mapView->objectAdded(object);
}

void EditorWindow::onSelectionChanged()
{
	if(!built)
		return;

	mapView->refreshLocked();
	updateToolbarState();
}

void EditorWindow::onUndoStateChanged(bool canUndo, bool canRedo)
{
	if(!built)
		return;

	toolbarButtons.at(ToolbarAction::UNDO)->block(!canUndo);
	toolbarButtons.at(ToolbarAction::REDO)->block(!canRedo);
}

// ---------------------------------------------------------------------------------------------
// file actions

bool EditorWindow::confirmDiscard(const std::function<void()> & next)
{
	if(!unsaved || !editor.hasMap())
	{
		next();
		return true;
	}

	CMessageDialog::askConfirmation(EditorTexts::get("confirm.title"), EditorTexts::get("confirm.unsaved"), next);
	return false;
}

void EditorWindow::newMap()
{
	confirmDiscard([this]()
	{
		EditorNewMapWindow::show(editor.getCallback(), [this](std::unique_ptr<CMap> map)
		{
			setMap(std::move(map), true);
		});
	});
}

void EditorWindow::openMapDialog()
{
	confirmDiscard([this]()
	{
		CFileDialog::openFile(EditorTexts::get("file.openMap"), {"vmap", "h3m"}, [this](const boost::filesystem::path & file) { openMap(file); });
	});
}

void EditorWindow::openRecentDialog()
{
	std::vector<CChoiceDialog::Item> items;
	const auto recent = EditorSettings::getRecentFiles();
	for(const auto & file : recent)
	{
		CChoiceDialog::Item item;
		item.text = TextOperations::filesystemPathToUtf8(file);
		items.push_back(std::move(item));
	}

	confirmDiscard([this, items, recent]()
	{
		CChoiceDialog::chooseOne(EditorTexts::get("file.recent"), items, items.empty() ? -1 : 0, [this, recent](int index) { openMap(recent.at(index)); });
	});
}

void EditorWindow::openMap(const boost::filesystem::path & file)
{
	std::unique_ptr<CMap> map;

	try
	{
		map = EditorFiles::openMap(file, editor.getCallback());
	}
	catch(const ModIncompatibility & e)
	{
		assert(e.whatExcessive().empty());
		std::string message = e.getFullErrorMsg();
		boost::erase_all(message, "{");
		boost::erase_all(message, "}");
		showError(EditorTexts::get("error.modsRequired"), message);
		return;
	}
	catch(const IdentifierResolutionException & e)
	{
		MetaString errorMsg;
		errorMsg.appendTextID("vcmi.server.errors.campOrMapFile.unknownEntity");
		errorMsg.replaceRawString(e.identifierName);
		showError(EditorTexts::get("error.openMap"), EditorTexts::translate(errorMsg));
		return;
	}
	catch(const std::exception & e)
	{
		showError(EditorTexts::get("error.openMap"), e.what());
		return;
	}

	const bool convert = map->version != EMapFormat::VCMI;
	setMap(std::move(map), convert);

	if(!convert)
		filename = file;

	EditorSettings::addRecentFile(file);
	updateTitle();
}

void EditorWindow::setMap(std::unique_ptr<CMap> map, bool isNew)
{
	filename.clear();
	editor.setMap(std::move(map));
	unsaved = isNew;

	//set minimal players count
	if(isNew)
	{
		editor.getMap()->players[0].canComputerPlay = true;
		editor.getMap()->players[0].canHumanPlay = true;
		rebuildPlayerChoices();
	}

	mapView->resetView();
	selectRightTab(0);
	updateTitle();
}

void EditorWindow::saveMap(bool force)
{
	if(!editor.hasMap())
		return;

	if(filename.empty())
	{
		saveMapAs();
		return;
	}

	if(!force && !unsaved)
		return;

	CMap & map = *editor.getMap();

	//validate map
	const auto issues = EditorValidator::validate(&map);
	bool critical = false;
	for(const auto & issue : issues)
		critical |= issue.critical;

	if(!issues.empty())
	{
		showError(EditorTexts::get("validation.title"), EditorTexts::get(critical ? "validation.critical" : "validation.errors"));
	}

	EditorTranslations::cleanupRemovedItems(map);

	for(const auto & obj : map.objects)
	{
		if(!obj)
			continue;

		if(obj->ID == Obj::HERO_PLACEHOLDER)
		{
			auto hero = dynamic_cast<CGHeroPlaceholder *>(obj.get());
			if(hero->heroType.has_value())
				map.reservedCampaignHeroes.insert(hero->heroType.value());
		}
	}

	try
	{
		EditorFiles::saveMap(editor.getMapUniquePtr(), filename);
	}
	catch(const std::exception & e)
	{
		showError(EditorTexts::get("error.saveMap"), e.what());
		return;
	}

	unsaved = false;
	updateTitle();
	setStatus(EditorTexts::get("status.saved"));
}

void EditorWindow::saveMapAs()
{
	if(!editor.hasMap())
		return;

	const std::string defaultName = filename.empty() ? "map" : TextOperations::filesystemPathToUtf8(filename.stem());
	CFileDialog::saveFile(EditorTexts::get("file.saveMap"), "vmap", defaultName, [this](const boost::filesystem::path & file)
	{
		filename = file;
		saveMap(true);
		EditorSettings::addRecentFile(file);
	});
}

void EditorWindow::exportImage()
{
	if(!editor.hasMap())
		return;

	CFileDialog::saveFile(EditorTexts::get("file.exportImage"), "png", "map", [this](const boost::filesystem::path & file)
	{
		try
		{
			mapView->exportLevel(file);
			setStatus(EditorTexts::get("status.exported"));
		}
		catch(const std::exception & e)
		{
			showError(EditorTexts::get("error.exportImage"), e.what());
		}
	});
}

void EditorWindow::closeEditor()
{
	confirmDiscard([this]() { close(); });
}

void EditorWindow::openCampaignEditor()
{
	confirmDiscard([this]() { EditorCampaignWindow::show(editor.getCallback(), boost::filesystem::path()); });
}

void EditorWindow::openCampaignFile(const boost::filesystem::path & file)
{
	EditorCampaignWindow::show(editor.getCallback(), file);
}

void EditorWindow::openTemplateEditor()
{
	confirmDiscard([]() { EditorTemplateWindow::show(); });
}

// ---------------------------------------------------------------------------------------------
// map actions

void EditorWindow::switchLevel(int level)
{
	if(!editor.hasMap() || level < 0 || level >= editor.levels())
		return;

	mapView->setLevel(level);
	minimap->setLevel(level);
	inspector->setLevel(level);
	levelDropDown->setSelected(level);
	minimap->setViewport(mapView->getVisibleTiles());
	updateToolbarState();
}

void EditorWindow::switchDefaultPlayer(const PlayerColor & player)
{
	editor.defaultPlayer = player;

	for(size_t i = 0; i < playerChoices.size(); ++i)
		if(playerChoices[i] == player)
			playerDropDown->setSelected(static_cast<int>(i));
}

void EditorWindow::addLevel()
{
	if(!editor.hasMap())
		return;

	if(!editor.canAddLevel())
	{
		showError(EditorTexts::get("menu.addLevel"), EditorTexts::get("level.maximum"));
		return;
	}

	std::vector<CChoiceDialog::Item> items;
	std::vector<MapLayerId> layers;
	for(const auto & layer : LIBRARY->mapLayerHandler->objects)
	{
		CChoiceDialog::Item item;
		item.text = layer->getNameTranslated();
		items.push_back(std::move(item));
		layers.push_back(layer->getId());
	}

	CChoiceDialog::chooseOne(EditorTexts::get("level.addPrompt"), items, 0, [this, layers](int index)
	{
		editor.addLevel(layers.at(index));
		switchLevel(editor.levels() - 1);
	});
}

void EditorWindow::selectMapLayer()
{
	if(!editor.hasMap())
		return;

	const int level = mapView->getLevel();
	std::vector<CChoiceDialog::Item> items;
	std::vector<MapLayerId> layers;
	int current = 0;
	for(const auto & layer : LIBRARY->mapLayerHandler->objects)
	{
		if(editor.getMap()->mapLayers[level] == layer->getId())
			current = static_cast<int>(layers.size());

		CChoiceDialog::Item item;
		item.text = layer->getNameTranslated();
		items.push_back(std::move(item));
		layers.push_back(layer->getId());
	}

	CChoiceDialog::chooseOne(EditorTexts::get("level.layerPrompt"), items, current, [this, level, layers](int index)
	{
		editor.getMap()->mapLayers[level] = layers.at(index);
		editor.commitChangeWithoutRedraw();
		rebuildLevelChoices();
	});
}

void EditorWindow::updateAppearance()
{
	if(!editor.hasMap())
		return;

	const int level = mapView->getLevel();
	if(editor.getObjectSelection(level).empty())
	{
		showError(EditorTexts::get("menu.updateAppearance"), EditorTexts::get("appearance.noSelection"));
		return;
	}

	CMessageDialog::askConfirmation(EditorTexts::get("menu.updateAppearance"), EditorTexts::get("appearance.irreversible"), [this, level]()
	{
		const int errors = editor.updateAppearance(level);

		// the objects that have no place are removed like in the original editor, then obstacles fill the free tiles
		editor.commitObjectErase(level);
		editor.commitObstacleFill(level);

		if(errors)
			showError(EditorTexts::get("menu.updateAppearance"), EditorTexts::format("appearance.errors", {std::to_string(errors)}));
	});
}

void EditorWindow::erase()
{
	if(!editor.hasMap())
		return;

	editor.commitObjectErase(mapView->getLevel());
	inspector->clear();
}

void EditorWindow::copy()
{
	if(editor.hasMap())
		editor.copyToClipboard(mapView->getLevel());
}

void EditorWindow::cut()
{
	if(editor.hasMap())
	{
		editor.copyToClipboard(mapView->getLevel());
		editor.commitObjectErase(mapView->getLevel());
		inspector->clear();
	}
}

void EditorWindow::paste()
{
	if(!editor.hasMap())
		return;

	const auto errors = editor.pasteFromClipboard(mapView->getLevel());
	if(!errors.empty())
	{
		std::string text;
		for(const auto & error : errors)
			text += error + "\n";
		showError(EditorTexts::get("error.cantPlace"), text);
	}
}

void EditorWindow::toggleGrid()
{
	mapView->setGridVisible(!mapView->isGridVisible());
	toolbarButtons.at(ToolbarAction::GRID)->setHighlighted(mapView->isGridVisible());
}

void EditorWindow::togglePassability()
{
	mapView->setPassabilityVisible(!mapView->isPassabilityVisible());
	toolbarButtons.at(ToolbarAction::PASSABILITY)->setHighlighted(mapView->isPassabilityVisible());
}

void EditorWindow::validate()
{
	if(editor.hasMap())
		EditorValidatorWindow::show(editor.getMap());
}

void EditorWindow::showTranslations()
{
	if(editor.hasMap())
		EditorTranslationsWindow::show(editor);
}

void EditorWindow::showMapSettings()
{
	if(editor.hasMap())
		EditorMapSettingsWindow::show(editor);
}

void EditorWindow::showPlayerSettings()
{
	if(editor.hasMap())
		EditorPlayerSettingsWindow::show(editor, [this]()
		{
			rebuildPlayerChoices();
			inspector->refresh();
		});
}

void EditorWindow::setTool(EditorMapView::Tool tool)
{
	mapView->setTool(tool);

	for(auto & [buttonTool, button] : toolButtons)
		button->setHighlighted(buttonTool == tool);

	if(tool != EditorMapView::Tool::SELECT && tool != EditorMapView::Tool::PAN)
		mapView->setPendingObject(nullptr);
}

void EditorWindow::paletteClicked(int index)
{
	if(!editor.hasMap())
		return;

	const int level = mapView->getLevel();

	switch(paletteMode)
	{
		case 0:
			editor.commitTerrainChange(level, LIBRARY->terrainTypeHandler->objects.at(index)->getId());
			break;
		case 1:
			editor.commitRoadOrRiverChange(level, static_cast<ui8>(LIBRARY->roadTypeHandler->objects.at(index)->getIndex()), true);
			break;
		default:
			editor.commitRoadOrRiverChange(level, static_cast<ui8>(LIBRARY->riverTypeHandler->objects.at(index)->getIndex()), false);
			break;
	}
}

// ---------------------------------------------------------------------------------------------
// object placement

void EditorWindow::catalogSelected(int row)
{
	const auto & node = *catalogRows.at(row).node;

	if(!node.leaf)
	{
		catalog->toggleExpanded(node);
		rebuildCatalog();
		return;
	}

	previewNode = &node;

	// preview
	{
		OBJECT_CONSTRUCTION_TARGETED(objectsTab.get());

		previewImage.reset();
		const auto templ = EditorObjectCatalog::getTemplate(node);
		const AnimationPath & animation = templ->editorAnimationFile.empty() ? templ->animationFile : templ->editorAnimationFile;

		const int previewHeight = 130;
		const Rect area(0, objectsTab->pos.h - previewHeight + 20, objectsTab->pos.w, previewHeight - 24);
		auto reference = ENGINE->renderHandler().loadImage(animation, 0, 0, EImageBlitMode::COLORKEY);
		const Point native = reference ? reference->dimensions() : Point(32, 32);

		const double scale = std::min(1.0, std::min(static_cast<double>(area.w) / std::max(1, native.x), static_cast<double>(area.h) / std::max(1, native.y)));
		const Point size(std::max(1, static_cast<int>(native.x * scale)), std::max(1, static_cast<int>(native.y * scale)));
		previewImage = std::make_shared<CAnimImage>(animation, 0, Rect(Rect::createCentered(area, size)), 0);

		previewLabel->setText(node.text);
	}

	if(!editor.hasMap())
		return;

	auto object = EditorObjectCatalog::createObject(node, editor.getCallback());
	setTool(EditorMapView::Tool::SELECT);
	editor.clearObjectSelection(mapView->getLevel());
	mapView->setPendingObject(object);
	setStatus(EditorTexts::get("status.placeObject"), 6000);
}

void EditorWindow::tryPlacement(const int3 & tile)
{
	auto object = mapView->getPendingObject();
	if(!object)
		return;

	const auto check = editor.checkPlacement(object.get());
	if(!check.error.empty())
	{
		mapView->setPendingObject(nullptr);
		showError(EditorTexts::get("error.cantPlace"), check.error);
		return;
	}

	auto askMods = [this, object, tile, check]()
	{
		if(check.missingMods.empty())
		{
			commitPlacement(object, tile);
			return;
		}

		std::string text;
		for(const auto & mod : check.missingMods)
			text += EditorMap::modMissingMessage(mod) + "\n";
		text += "\n" + EditorTexts::get("mods.addNow");

		CMessageDialog::askConfirmation(EditorTexts::get("mods.missingTitle"), text, [this, object, tile, check]()
		{
			editor.registerRequiredMods(check.missingMods);
			commitPlacement(object, tile);
		}, [this]()
		{
			mapView->setPendingObject(nullptr);
			showError(EditorTexts::get("error.cantPlace"), EditorTexts::get("mods.mandatory"));
		});
	};

	if(!check.needsPlayer)
	{
		askMods();
		return;
	}

	// heroes can not belong to the neutral side, so the owner has to be chosen first
	std::vector<CChoiceDialog::Item> items;
	std::vector<PlayerColor> players;
	for(const auto & player : playerChoices)
	{
		if(player == PlayerColor::NEUTRAL)
			continue;

		CChoiceDialog::Item item;
		item.text = EditorTexts::playerName(player);
		items.push_back(std::move(item));
		players.push_back(player);
	}

	auto dialog = std::make_shared<CChoiceDialog>(EditorTexts::get("player.selectTitle"), items, false);
	dialog->setSelected(items.empty() ? -1 : 0);
	auto * raw = dialog.get();
	dialog->onAccept = [raw]() { return raw->getSelected() >= 0; };
	dialog->afterAccept = [this, raw, players, askMods]()
	{
		switchDefaultPlayer(players.at(raw->getSelected()));
		askMods();
	};
	dialog->onCancel = [this, object]()
	{
		mapView->setPendingObject(nullptr);
		showError(EditorTexts::get("error.cantPlace"), EditorTexts::format("error.heroNeutral", {object->instanceName}));
	};
	ENGINE->windows().pushWindow(dialog);
}

void EditorWindow::commitPlacement(const std::shared_ptr<CGObjectInstance> & object, const int3 & tile)
{
	const int level = mapView->getLevel();

	mapView->setPendingObject(nullptr);
	editor.commitObjectCreate(level, object, tile);

	editor.clearObjectSelection(level);
	editor.selectObject(level, object.get());
	inspector->showObject(object.get());
}

// ---------------------------------------------------------------------------------------------
// input

void EditorWindow::keyPressed(EShortcut key)
{
	const bool typing = CFocusable::isAnyFocused();

	switch(key)
	{
		case EShortcut::EDITOR_NEW:
			newMap();
			break;
		case EShortcut::EDITOR_OPEN:
			openMapDialog();
			break;
		case EShortcut::EDITOR_SAVE:
			saveMap(false);
			break;
		case EShortcut::EDITOR_SAVE_AS:
			saveMapAs();
			break;
		case EShortcut::EDITOR_UNDO:
			if(editor.hasMap() && !toolbarButtons.at(ToolbarAction::UNDO)->isBlocked())
				editor.undo();
			break;
		case EShortcut::EDITOR_REDO:
			if(editor.hasMap() && !toolbarButtons.at(ToolbarAction::REDO)->isBlocked())
				editor.redo();
			break;
		case EShortcut::EDITOR_CUT:
			if(!typing)
				cut();
			break;
		case EShortcut::EDITOR_COPY:
			if(!typing)
				copy();
			break;
		case EShortcut::EDITOR_PASTE:
			if(!typing)
				paste();
			break;
		case EShortcut::EDITOR_ERASE:
			if(!typing)
				erase();
			break;
		case EShortcut::EDITOR_TOGGLE_GRID:
			toggleGrid();
			break;
		case EShortcut::EDITOR_TOGGLE_PASSABILITY:
			togglePassability();
			break;
		case EShortcut::EDITOR_TOGGLE_LEVEL:
			if(editor.hasMap() && editor.levels() > 1 && !typing)
				switchLevel((mapView->getLevel() + 1) % editor.levels());
			break;
		case EShortcut::EDITOR_ZOOM_IN:
			mapView->zoom(1);
			break;
		case EShortcut::EDITOR_ZOOM_OUT:
			mapView->zoom(-1);
			break;
		case EShortcut::EDITOR_ZOOM_RESET:
			mapView->resetZoom();
			break;
		case EShortcut::EDITOR_VALIDATE:
			validate();
			break;
		case EShortcut::EDITOR_MAP_SETTINGS:
			showMapSettings();
			break;
		case EShortcut::GLOBAL_CANCEL:
			if(editor.hasMap() && !typing && !mapView->getPendingObject())
			{
				editor.clearObjectSelection(mapView->getLevel());
				editor.clearTerrainSelection(mapView->getLevel());
			}
			break;
		default:
			break;
	}
}

void EditorWindow::tick(uint32_t msPassed)
{
	if(statusTimeLeft <= 0)
		return;

	statusTimeLeft -= static_cast<int>(msPassed);
	if(statusTimeLeft <= 0)
		statusLabel->setText("");
}

void EditorWindow::onScreenResize()
{
	const bool wasActive = isActive();
	if(wasActive)
		deactivate();

	const bool inspectorShown = inspector && !inspector->isDisabled();

	menuBar.reset();
	titleLabel.reset();
	toolbarButtons.clear();
	levelDropDown.reset();
	playerDropDown.reset();
	statusLabel.reset();
	coordinateLabel.reset();
	mapView.reset();
	minimap.reset();
	toolButtons.clear();
	paletteTabs.reset();
	paletteList.reset();
	rightTabs.reset();
	catalogFilter.reset();
	terrainFilter.reset();
	catalogList.reset();
	previewImage.reset();
	previewLabel.reset();
	objectsTab.reset();
	inspector.reset();
	topPanel.reset();
	leftPanel.reset();
	rightPanel.reset();
	statusPanel.reset();

	build();
	selectRightTab(inspectorShown ? 1 : 0);

	if(wasActive)
		activate();
}

void EditorWindow::showAll(Canvas & to)
{
	CIntObject::showAll(to);
}
