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

VCMI_LIB_NAMESPACE_BEGIN

class CMapInfo;
struct StartInfo;
struct PlayerInfo;

VCMI_LIB_NAMESPACE_END

class CButton;
class CTextBox;
class CTextInput;
class CAnimImage;
class CToggleGroup;
class RandomMapTab;
class OptionsTab;
class TurnOptionsTab;
class ExtraOptionsTab;
class SelectionTab;
class BattleOnlyModeTab;
class InfoCard;
class CChatBox;
class PvPBox;
class TownSelector;
class CLabel;
class CSlider;
class CFlagBox;
class CLabelGroup;
class TransparentFilledRectangle;
class FilledTexturePlayerColored;
class LRClickableArea;

class ISelectionScreenInfo
{
public:
	ESelectionScreen screenType;

	ISelectionScreenInfo(ESelectionScreen ScreenType = ESelectionScreen::unknown);
	virtual ~ISelectionScreenInfo();
	virtual const CMapInfo * getMapInfo() = 0;
	virtual const StartInfo * getStartInfo() = 0;

	virtual int getCurrentDifficulty();
	virtual PlayerInfo getPlayerInfo(PlayerColor color);

};

/// The actual map selection screen which consists of the options and selection tab
class CSelectionBase : public CWindowObject, public ISelectionScreenInfo
{
public:
	std::shared_ptr<InfoCard> card;

	std::shared_ptr<CButton> buttonSelect;
	std::shared_ptr<CButton> buttonRMG;
	std::shared_ptr<CButton> buttonOptions;
	std::shared_ptr<CButton> buttonTurnOptions;
	std::shared_ptr<CButton> buttonExtraOptions;
	std::shared_ptr<CButton> buttonStart;
	std::shared_ptr<CButton> buttonBack;
	std::shared_ptr<CButton> buttonSimturns;

	std::vector<std::shared_ptr<CPicture>> images;

	std::shared_ptr<SelectionTab> tabSel;
	std::shared_ptr<OptionsTab> tabOpt;
	std::shared_ptr<TurnOptionsTab> tabTurnOptions;
	std::shared_ptr<ExtraOptionsTab> tabExtraOptions;
	std::shared_ptr<RandomMapTab> tabRand;
	std::shared_ptr<BattleOnlyModeTab> tabBattleOnlyMode;
	std::shared_ptr<CIntObject> curTab;

	CSelectionBase(ESelectionScreen type);
	virtual void toggleTab(std::shared_ptr<CIntObject> tab);
};

class InfoCard : public CIntObject
{
	std::shared_ptr<CPicture> playerListBg;
	std::shared_ptr<CPicture> background;

	std::shared_ptr<CAnimImage> iconsVictoryCondition;
	std::shared_ptr<CAnimImage> iconsLossCondition;
	std::shared_ptr<CAnimImage> iconsMapSizes;

	std::shared_ptr<CLabel> labelSaveDate;
	std::shared_ptr<CLabel> labelMapSize;
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

	std::shared_ptr<CLabelGroup> labelGroupPlayers;
	std::shared_ptr<CButton> buttonInvitePlayers;
	std::shared_ptr<CButton> buttonOpenGlobalLobby;

	std::shared_ptr<PvPBox> pvpBox;
public:

	bool showChat;
	std::shared_ptr<CChatBox> chat;
	std::shared_ptr<CFlagBox> flagbox;

	std::shared_ptr<CToggleGroup> iconDifficulty;

	InfoCard();
	void disableLabelRedraws();
	void changeSelection();
	void clearSelection();
	void toggleChat();
	void setChat(bool activateChat);
};

class CChatBox : public CIntObject
{
public:
	std::shared_ptr<CTextBox> chatHistory;
	std::shared_ptr<CTextInput> inputBox;
	std::shared_ptr<TransparentFilledRectangle> inputBackground;

	CChatBox(const Rect & rect);

	void keyPressed(EShortcut key) override;
	bool captureThisKey(EShortcut key) override;
	void addNewMessage(const std::string & text);
};

class PvPBox : public CIntObject
{
	std::shared_ptr<FilledTexturePlayerColored> backgroundTexture;
	std::shared_ptr<TransparentFilledRectangle> backgroundBorder;
	
	std::shared_ptr<TownSelector> townSelector;

	std::shared_ptr<CButton> buttonFlipCoin;
	std::shared_ptr<CButton> buttonRandomTown;
	std::shared_ptr<CButton> buttonRandomTownVs;
	std::shared_ptr<CButton> buttonHandicap;
public:
	PvPBox(const Rect & rect);
};

class TownSelector : public CIntObject
{
	std::map<FactionID, std::shared_ptr<CAnimImage>> towns;
	std::map<FactionID, std::shared_ptr<LRClickableArea>> townsArea;
	std::shared_ptr<CSlider> slider;

	void sliderMove(int slidPos);
	void updateListItems();

public:
	std::map<FactionID, bool> townsEnabled;

	TownSelector(const Point & loc);
};

class CFlagBox : public CIntObject
{
	std::shared_ptr<CLabel> labelAllies;
	std::shared_ptr<CLabel> labelEnemies;
	std::vector<std::shared_ptr<CAnimImage>> flagsAllies;
	std::vector<std::shared_ptr<CAnimImage>> flagsEnemies;

public:
	CFlagBox(const Rect & rect);
	void recreate();
	void showPopupWindow(const Point & cursorPosition) override;
	void showTeamsPopup();

	class CFlagBoxTooltipBox : public CWindowObject
	{
		std::shared_ptr<CLabel> labelTeamAlignment;
		std::shared_ptr<CLabelGroup> labelGroupTeams;
		std::vector<std::shared_ptr<CAnimImage>> iconsFlags;
	public:
		CFlagBoxTooltipBox();
	};
};

extern ISelectionScreenInfo * SEL;
