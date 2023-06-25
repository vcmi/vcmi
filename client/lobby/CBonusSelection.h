/*
 * CBonusSelection.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"
#include "../lib/mapping/CCampaignHandler.h"

VCMI_LIB_NAMESPACE_BEGIN

class CampaignState;
struct CampaignRegions;

VCMI_LIB_NAMESPACE_END

class CButton;
class CTextBox;
class CToggleGroup;
class CAnimImage;
class CLabel;
class CFlagBox;
class ISelectionScreenInfo;

/// Campaign screen where you can choose one out of three starting bonuses
class CBonusSelection : public CWindowObject
{
public:
	std::shared_ptr<CampaignState> getCampaign();
	CBonusSelection();

	class CRegion
		: public CIntObject
	{
		std::shared_ptr<CPicture> graphicsNotSelected;
		std::shared_ptr<CPicture> graphicsSelected;
		std::shared_ptr<CPicture> graphicsStriped;
		CampaignScenarioID idOfMapAndRegion;
		bool accessible; // false if region should be striped
		bool selectable; // true if region should be selectable
	public:
		CRegion(CampaignScenarioID id, bool accessible, bool selectable, const CampaignRegions & campDsc);
		void updateState();
		void clickLeft(tribool down, bool previousState) override;
		void showPopupWindow() override;
	};

	void createBonusesIcons();
	void updateAfterStateChange();

	// Event handlers
	void goBack();
	void startMap();
	void restartMap();
	void increaseDifficulty();
	void decreaseDifficulty();

	std::shared_ptr<CPicture> panelBackground;
	std::shared_ptr<CButton> buttonStart;
	std::shared_ptr<CButton> buttonRestart;
	std::shared_ptr<CButton> buttonBack;
	std::shared_ptr<CLabel> campaignName;
	std::shared_ptr<CLabel> labelCampaignDescription;
	std::shared_ptr<CTextBox> campaignDescription;
	std::shared_ptr<CLabel> mapName;
	std::shared_ptr<CLabel> labelMapDescription;
	std::shared_ptr<CTextBox> mapDescription;
	std::vector<std::shared_ptr<CRegion>> regions;
	std::shared_ptr<CFlagBox> flagbox;

	std::shared_ptr<CLabel> labelChooseBonus;
	std::shared_ptr<CToggleGroup> groupBonuses;
	std::shared_ptr<CLabel> labelDifficulty;
	std::array<std::shared_ptr<CAnimImage>, 5> difficultyIcons;
	std::shared_ptr<CButton> buttonDifficultyLeft;
	std::shared_ptr<CButton> buttonDifficultyRight;
	std::shared_ptr<CAnimImage> iconsMapSizes;
};
