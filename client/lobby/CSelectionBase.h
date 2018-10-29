/*
 * CSelectionBase.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../mainmenu/CMainMenu.h"

class CButton;
class CTextBox;
class CTextInput;
class CAnimImage;
class CToggleGroup;
class RandomMapTab;
class OptionsTab;
class SelectionTab;
class InfoCard;
class CChatBox;
class CMapInfo;
struct StartInfo;
struct PlayerInfo;
class CLabel;
class CFlagBox;
class CLabelGroup;

class ISelectionScreenInfo
{
public:
	ESelectionScreen screenType;

	ISelectionScreenInfo(ESelectionScreen ScreenType = ESelectionScreen::unknown);
	virtual ~ISelectionScreenInfo();
	virtual const CMapInfo * getMapInfo() = 0;
	virtual const StartInfo * getStartInfo() = 0;

	virtual int getCurrentDifficulty();
	virtual PlayerInfo getPlayerInfo(int color);

};

/// The actual map selection screen which consists of the options and selection tab
class CSelectionBase : public CWindowObject, public ISelectionScreenInfo
{
public:
	std::shared_ptr<InfoCard> card;

	std::shared_ptr<CButton> buttonSelect;
	std::shared_ptr<CButton> buttonRMG;
	std::shared_ptr<CButton> buttonOptions;
	std::shared_ptr<CButton> buttonStart;
	std::shared_ptr<CButton> buttonBack;

	std::shared_ptr<SelectionTab> tabSel;
	std::shared_ptr<OptionsTab> tabOpt;
	std::shared_ptr<RandomMapTab> tabRand;
	std::shared_ptr<View> curTab;

	CSelectionBase(ESelectionScreen type);
	virtual void toggleTab(std::shared_ptr<View> tab);
};

class InfoCard : public View
{
	std::shared_ptr<CPicture> playerListBg;
	std::shared_ptr<CPicture> background;

	std::shared_ptr<CAnimImage> iconsVictoryCondition;
	std::shared_ptr<CAnimImage> iconsLossCondition;
	std::shared_ptr<CAnimImage> iconsMapSizes;

	std::shared_ptr<CLabel> labelSaveDate;
	std::shared_ptr<CLabel> labelScenarioName;
	std::shared_ptr<CLabel> labelScenarioDescription;
	std::shared_ptr<CLabel> labelVictoryCondition;
	std::shared_ptr<CLabel> labelLossCondition;
	std::shared_ptr<CLabel> labelMapDiff;
	std::shared_ptr<CLabel> labelPlayerDifficulty;
	std::shared_ptr<CLabel> labelRating;
	std::shared_ptr<CLabel> labelCampaignDescription;

	std::shared_ptr<CLabel> mapName;
	std::shared_ptr<CTextBox> mapDescription;
	std::shared_ptr<CLabel> labelDifficulty;
	std::shared_ptr<CLabel> labelDifficultyPercent;
	std::shared_ptr<CLabel> labelVictoryConditionText;
	std::shared_ptr<CLabel> labelLossConditionText;

	std::shared_ptr<CLabelGroup> labelGroupPlayersAssigned;
	std::shared_ptr<CLabelGroup> labelGroupPlayersUnassigned;
public:

	bool showChat;
	std::shared_ptr<CChatBox> chat;
	std::shared_ptr<CFlagBox> flagbox;

	std::shared_ptr<CToggleGroup> iconDifficulty;

	InfoCard();
	void changeSelection();
	void toggleChat();
	void setChat(bool activateChat);
};

class CChatBox : public View
{
public:
	std::shared_ptr<CTextBox> chatHistory;
	std::shared_ptr<CTextInput> inputBox;

	CChatBox(const Rect & rect);

	void keyPressed(const SDL_Event &event, const SDL_KeyboardEvent & key) override;

	void addNewMessage(const std::string & text);
};

class CFlagBox : public View
{
	std::shared_ptr<CAnimation> iconsTeamFlags;
	std::shared_ptr<CLabel> labelAllies;
	std::shared_ptr<CLabel> labelEnemies;
	std::vector<std::shared_ptr<CAnimImage>> flagsAllies;
	std::vector<std::shared_ptr<CAnimImage>> flagsEnemies;

public:
	CFlagBox(const Rect & rect);
	void recreate();
	void clickRight(const SDL_Event &event, tribool down) override;
	void showTeamsPopup();

	class CFlagBoxTooltipBox : public CWindowObject
	{
		std::shared_ptr<CLabel> labelTeamAlignment;
		std::shared_ptr<CLabelGroup> labelGroupTeams;
		std::vector<std::shared_ptr<CAnimImage>> iconsFlags;
	public:
		CFlagBoxTooltipBox(std::shared_ptr<CAnimation> icons);
	};
};

extern ISelectionScreenInfo * SEL;
