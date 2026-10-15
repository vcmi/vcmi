/*
 * EditorScenarioDialog.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../../Translator.h"
#include "../../windows/CDialogWindow.h"

#include "../../../lib/campaign/CampaignBonus.h"

class CampaignState;
class CFormView;
class CListEditor;
class CMap;
class CTabBar;
class EditorCallback;

/// Window that edits one scenario of a campaign
class EditorScenarioDialog : public CDialogWindow
{
	static constexpr int MAX_START_OPTIONS = 3;

	struct Draft
	{
		std::string regionText;
		int regionColor = 0;
		int difficulty = 0;
		std::set<int> prerequisites;

		bool prologEnabled = false;
		std::array<std::string, 5> prolog; ///< video, video loop, music, voice, text
		bool epilogEnabled = false;
		std::array<std::string, 5> epilog;

		bool keepExperience = false;
		bool keepPrimary = false;
		bool keepSecondary = false;
		bool keepSpells = false;
		bool keepArtifacts = false;
		std::set<int> keptArtifacts;
		std::set<int> keptCreatures;

		CampaignStartOptions startOption = CampaignStartOptions::START_BONUS;
		int playerPosition = 0;
		std::vector<CampaignBonus> bonuses;
		/// Scenario or hero, and the player position
		std::vector<std::pair<int, int>> rows;
	};

	std::shared_ptr<CampaignState> campaign;
	CampaignScenarioID scenario;
	EditorCallback * callback;
	std::function<void()> changedCallback;
	std::unique_ptr<CMap> map;
	std::optional<TranslatorOverlay> mapTexts;
	Draft draft;
	std::vector<PlayerColor> playerPositions;
	std::vector<int> heroIds;
	std::vector<std::string> heroNames;

	std::shared_ptr<CTabBar> tabs;
	std::array<std::shared_ptr<CFormView>, 4> pages;
	std::shared_ptr<CListEditor> startList;

	void loadDraft();
	void loadMap();
	void buildPages();
	void buildGeneralPage();
	void buildTextsPage();
	void buildCrossoverPage();
	void buildStartPage();
	void selectPage(int index);
	void refreshStartList(int selected);
	void editStartRow(int index);
	void editStartBonus(int index);
	void importMap();
	void exportMap();
	void removeMap();
	std::string scenarioTitle(int index) const;
	std::string playerPositionName(const PlayerColor & player) const;
	bool commit();

public:
	EditorScenarioDialog(const std::shared_ptr<CampaignState> & campaign, CampaignScenarioID scenario, EditorCallback * callback, const std::function<void()> & onChanged);
	~EditorScenarioDialog() override;
};
