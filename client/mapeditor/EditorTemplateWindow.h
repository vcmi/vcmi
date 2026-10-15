/*
 * EditorTemplateWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../gui/CIntObject.h"

#include "../../lib/rmg/CRmgTemplate.h"

class CFormView;
class CLabel;
class CMenuBar;
class CPanel;
class CTabBar;
class EditorCallback;
class EditorTemplateView;

/// Window of the random map template editor
class EditorTemplateWindow : public WindowBase
{
	static constexpr int MENU_HEIGHT = 24;
	static constexpr int STATUS_HEIGHT = 22;
	static constexpr int PANEL_WIDTH = 420;
	static constexpr int TAB_COUNT = 3;

	std::unique_ptr<EditorCallback> callback;
	std::map<std::string, std::shared_ptr<CRmgTemplate>> templates;
	std::string selectedTemplate;
	TRmgTemplateZoneId selectedZone = -1;
	int selectedConnection = -1;
	boost::filesystem::path filename;
	bool unsaved = false;

	std::shared_ptr<CPanel> background;
	std::shared_ptr<CPanel> topPanel;
	std::shared_ptr<CPanel> sidePanel;
	std::shared_ptr<CPanel> statusPanel;
	std::shared_ptr<CMenuBar> menuBar;
	std::shared_ptr<CLabel> titleLabel;
	std::shared_ptr<CLabel> statusLabel;
	std::shared_ptr<EditorTemplateView> view;
	std::shared_ptr<CTabBar> tabs;
	std::array<std::shared_ptr<CFormView>, TAB_COUNT> pages;

	void build();
	void updateTitle();
	void changed();
	void setStatus(const std::string & text);
	bool validate();
	bool confirmDiscard(const std::function<void()> & next);

	CRmgTemplate & currentTemplate() const;
	rmg::ZoneOptions & currentZone() const;
	void initContent();
	void loadContent(bool autoPosition);
	void selectPage(int index);
	void buildTemplatePage();
	void buildZonePage();
	void buildConnectionsPage();
	void refreshCards();
	void autoPositionZones();
	void setDefaultZone(rmg::ZoneOptions & zone, TRmgTemplateZoneId id);
	void selectZone(TRmgTemplateZoneId zone);

	void newTemplates();
	void openDialog();
	void openTemplates(const boost::filesystem::path & file);
	void save();
	void saveAs();
	void addZone();
	void removeZone();
	void addTemplate();
	void removeTemplate();
	void renameTemplate();
	void closeEditor();

public:
	EditorTemplateWindow();
	~EditorTemplateWindow() override;

	static void show();
};
