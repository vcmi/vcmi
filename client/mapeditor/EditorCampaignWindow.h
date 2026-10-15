/*
 * EditorCampaignWindow.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../Translator.h"
#include "../gui/CIntObject.h"

#include "../../lib/constants/EntityIdentifiers.h"

class CampaignState;
class CLabel;
class CMap;
class CMenuBar;
class CPanel;
class EditorCallback;
class EditorCampaignView;

/// Window of the campaign editor
class EditorCampaignWindow : public WindowBase
{
	static constexpr int MENU_HEIGHT = 24;
	static constexpr int STATUS_HEIGHT = 22;

	EditorCallback * callback;
	std::shared_ptr<CampaignState> campaign;
	std::optional<TranslatorOverlay> campaignTexts;
	boost::filesystem::path filename;
	CampaignScenarioID selected;
	bool unsaved = false;
	bool fullBackground = false;

	std::shared_ptr<CPanel> background;
	std::shared_ptr<CPanel> topPanel;
	std::shared_ptr<CPanel> statusPanel;
	std::shared_ptr<CMenuBar> menuBar;
	std::shared_ptr<CLabel> titleLabel;
	std::shared_ptr<CLabel> statusLabel;
	std::shared_ptr<EditorCampaignView> view;

	void build();
	void setCampaign(std::shared_ptr<CampaignState> newCampaign);
	void refresh();
	void updateTitle();
	void changed();
	void setStatus(const std::string & text);
	bool validate();

	bool confirmDiscard(const std::function<void()> & next);
	void newCampaign();
	void openDialog();
	void openSetDialog();
	void openCampaign(const boost::filesystem::path & file);
	void save();
	void saveAs();
	void showCampaignProperties();
	void showScenarioProperties(CampaignScenarioID scenario);
	void closeEditor();

public:
	explicit EditorCampaignWindow(EditorCallback * callback);

	/// Opens the window and the campaign file, if the path is not empty
	static void show(EditorCallback * callback, const boost::filesystem::path & file);

	/// Loads the map of the scenario. Shows the reason and returns nullptr if the map can not be loaded or the scenario has no map
	static std::unique_ptr<CMap> tryToOpenMap(const std::shared_ptr<CampaignState> & campaign, CampaignScenarioID scenario, EditorCallback * callback);
};
