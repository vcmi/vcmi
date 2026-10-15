/*
 * EditorWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "EditorMap.h"
#include "EditorMapView.h"
#include "EditorObjectCatalog.h"

#include "../gui/CIntObject.h"

class CLabel;
class CMenuBar;
class CPanel;
class CSelectList;
class CTabBar;
class CTextButton;
class CTextField;
class CDropDown;
class CAnimImage;
class EditorInspector;
class EditorMinimap;

/// Main window of the map editor
class EditorWindow : public WindowBase, public IEditorMapListener
{
	static constexpr int MENU_HEIGHT = 24;
	static constexpr int TOOLBAR_HEIGHT = 36;
	static constexpr int STATUS_HEIGHT = 22;
	static constexpr int LEFT_WIDTH = 220;
	static constexpr int RIGHT_WIDTH = 290;

	enum class ToolbarAction
	{
		NEW,
		OPEN,
		SAVE,
		UNDO,
		REDO,
		CUT,
		COPY,
		PASTE,
		ERASE,
		GRID,
		PASSABILITY,
		LOCK,
		UNLOCK,
		ZOOM_IN,
		ZOOM_OUT,
		ZOOM_RESET,
		FILL_OBSTACLES,
		TRANSLATIONS,
		COUNT
	};

	EditorMap editor;
	std::unique_ptr<EditorObjectCatalog> catalog;
	std::vector<EditorObjectCatalog::Row> catalogRows;
	const EditorObjectCatalog::Node * previewNode = nullptr;

	std::shared_ptr<CPanel> topPanel;
	std::shared_ptr<CPanel> leftPanel;
	std::shared_ptr<CPanel> rightPanel;
	std::shared_ptr<CPanel> statusPanel;
	std::shared_ptr<CMenuBar> menuBar;
	std::map<ToolbarAction, std::shared_ptr<CTextButton>> toolbarButtons;
	std::shared_ptr<CDropDown> levelDropDown;
	std::shared_ptr<CDropDown> playerDropDown;
	std::shared_ptr<EditorMapView> mapView;
	std::shared_ptr<EditorMinimap> minimap;
	std::map<EditorMapView::Tool, std::shared_ptr<CTextButton>> toolButtons;
	std::shared_ptr<CTabBar> paletteTabs;
	std::shared_ptr<CSelectList> paletteList;
	std::shared_ptr<CTabBar> rightTabs;
	std::shared_ptr<CIntObject> objectsTab;
	std::shared_ptr<CLabel> titleLabel;
	std::shared_ptr<CLabel> coordinateLabel;
	std::shared_ptr<CTextField> catalogFilter;
	std::shared_ptr<CDropDown> terrainFilter;
	std::shared_ptr<CSelectList> catalogList;
	std::shared_ptr<CAnimImage> previewImage;
	std::shared_ptr<CLabel> previewLabel;
	std::shared_ptr<EditorInspector> inspector;
	std::shared_ptr<CLabel> statusLabel;

	std::vector<PlayerColor> playerChoices;
	std::vector<TerrainId> terrainChoices;
	int paletteMode = 0;
	boost::filesystem::path filename;
	bool unsaved = false;
	int statusTimeLeft = 0;
	bool built = false;

	void build();
	void buildMenu();
	void buildToolbar();
	void buildLeftPanel();
	void buildRightPanel();
	void rebuildPalette();
	void rebuildCatalog();
	void rebuildLevelChoices();
	void rebuildPlayerChoices();
	void selectRightTab(int tab);


	void updateToolbarState();
	void updateTitle();
	void setStatus(const std::string & text, int durationMs = 2000);

	// actions
	bool confirmDiscard(const std::function<void()> & next);
	void newMap();
	void openMapDialog();
	void openMap(const boost::filesystem::path & file);
	void openRecentDialog();
	void saveMap(bool force);
	void saveMapAs();
	void exportImage();
	void closeEditor();
	void setMap(std::unique_ptr<CMap> map, bool isNew);
	void switchLevel(int level);
	void switchDefaultPlayer(const PlayerColor & player);
	void addLevel();
	void selectMapLayer();
	void updateAppearance();
	void erase();
	void copy();
	void cut();
	void paste();
	void toggleGrid();
	void togglePassability();
	void validate();
	void showTranslations();
	void showMapSettings();
	void showPlayerSettings();
	void openCampaignEditor();
	void openTemplateEditor();
	void openCampaignFile(const boost::filesystem::path & file);
	void setTool(EditorMapView::Tool tool);
	void paletteClicked(int index);
	void catalogSelected(int row);
	void tryPlacement(const int3 & tile);
	void commitPlacement(const std::shared_ptr<CGObjectInstance> & object, const int3 & tile);
	void showError(const std::string & title, const std::string & text);

public:
	EditorWindow();
	~EditorWindow() override;

	// IEditorMapListener
	void onMapModified() override;
	void onMapReloaded() override;
	void onContentChanged() override;
	void onObjectRemoving(const CGObjectInstance * object) override;
	void onObjectAdded(const CGObjectInstance * object) override;
	void onSelectionChanged() override;
	void onUndoStateChanged(bool canUndo, bool canRedo) override;

	void keyPressed(EShortcut key) override;
	void tick(uint32_t msPassed) override;
	void onScreenResize() override;
	void showAll(Canvas & to) override;
};
