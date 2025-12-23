/*
 * CMainMenu.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "../windows/CWindowObject.h"
#include "../../lib/json/JsonNode.h"
#include "../../lib/LoadProgress.h"

VCMI_LIB_NAMESPACE_BEGIN

class CampaignState;

VCMI_LIB_NAMESPACE_END

class CTextInput;
class CGStatusBar;
class CTextBox;
class CTabbedInt;
class CAnimImage;
class CButton;
class CFilledTexture;
class CLabel;
class VideoWidget;

// TODO: Find new location for these enums
enum class ESelectionScreen : ui8 {
	unknown = 0, newGame, loadGame, saveGame, scenarioInfo, campaignList
};

enum class ELoadMode : ui8
{
	NONE = 0, SINGLE, MULTI, CAMPAIGN, TUTORIAL
};

/// The main menu screens listed in the EState enum
class CMenuScreen : public CWindowObject
{
	const JsonNode & config;

	std::shared_ptr<CTabbedInt> tabs;

	std::shared_ptr<CPicture> background;
	std::shared_ptr<VideoWidget> videoPlayer;
	std::vector<std::shared_ptr<CPicture>> images;

	std::shared_ptr<CIntObject> createTab(size_t index);

public:
	std::vector<std::string> menuNameToEntry;

	CMenuScreen(const JsonNode & configNode);

	void activate() override;
	void show(Canvas & to) override;

	void switchToTab(size_t index);
	void switchToTab(std::string name);
	size_t getActiveTab() const;
};

class CMenuEntry : public CIntObject
{
	std::vector<std::shared_ptr<CPicture>> images;
	std::vector<std::shared_ptr<CButton>> buttons;

	std::shared_ptr<CButton> createButton(CMenuScreen * parent, const JsonNode & button);

public:
	CMenuEntry(CMenuScreen * parent, const JsonNode & config);
};

/// Multiplayer mode
class CMultiMode : public WindowBase
{
public:
	ESelectionScreen screenType;
	std::shared_ptr<CPicture> background;
	std::shared_ptr<CPicture> picture;
	std::shared_ptr<CTextBox> textTitle;
	std::shared_ptr<CTextInput> playerName;
	std::shared_ptr<CButton> buttonHotseat;
	std::shared_ptr<CButton> buttonLobby;
	std::shared_ptr<CButton> buttonHost;
	std::shared_ptr<CButton> buttonJoin;
	std::shared_ptr<CButton> buttonCancel;
	std::shared_ptr<CGStatusBar> statusBar;

	CMultiMode(ESelectionScreen ScreenType);
	void openLobby();
	void hostTCP(EShortcut shortcut);
	void joinTCP(EShortcut shortcut);

	/// Get all configured player names. The first name would always be present and initialized to its default value.
	std::vector<std::string> getPlayersNames();

	void onNameChange(std::string newText);
};

/// Hot seat player window
class CMultiPlayers : public WindowBase
{
	bool host;
	ELoadMode loadMode;
	ESelectionScreen screenType;
	std::shared_ptr<CPicture> background;
	std::shared_ptr<CTextBox> textTitle;
	std::array<std::shared_ptr<CTextInput>, 8> inputNames;
	std::shared_ptr<CButton> buttonOk;
	std::shared_ptr<CButton> buttonCancel;
	std::shared_ptr<CGStatusBar> statusBar;

	void onChange(std::string newText);
	void enterSelectionScreen();

public:
	CMultiPlayers(const std::vector<std::string> & playerNames, ESelectionScreen ScreenType, bool Host, ELoadMode LoadMode, EShortcut shortcut);
};

/// Manages the configuration of pregame GUI elements like campaign screen, main menu, loading screen,...
class CMainMenuConfig
{
public:
	static const CMainMenuConfig & get();
	const JsonNode & getConfig() const;
	const JsonNode & getCampaigns() const;

private:
	CMainMenuConfig();

	const JsonNode campaignSets;
	const JsonNode config;
};

/// Handles background screen, loads graphics for victory/loss condition and random town or hero selection
class CMainMenu final : public CIntObject, public std::enable_shared_from_this<CMainMenu>
{
	std::shared_ptr<CFilledTexture> backgroundAroundMenu;

	std::vector<VideoPath> videoPlayList;

public:
	CMainMenu();

	std::shared_ptr<CMenuScreen> menu;

	~CMainMenu();
	void activate() override;
	void onScreenResize() override;
	void makeActiveInterface();
	static void openLobby(ESelectionScreen screenType, bool host, const std::vector<std::string> & names, ELoadMode loadMode, bool battleMode);
	static void openCampaignLobby(const std::string & campaignFileName, std::string campaignSet = "");
	static void openCampaignLobby(std::shared_ptr<CampaignState> campaign);
	static void startTutorial();
	static void openHighScoreScreen();
	void openCampaignScreen(std::string name);

	static std::shared_ptr<CPicture> createPicture(const JsonNode & config);

	void playIntroVideos();
	void playMusic();
};

/// Simple window to enter the server's address.
class CSimpleJoinScreen : public WindowBase
{
	std::shared_ptr<CPicture> background;
	std::shared_ptr<CTextBox> textTitle;
	std::shared_ptr<CButton> buttonOk;
	std::shared_ptr<CButton> buttonCancel;
	std::shared_ptr<CGStatusBar> statusBar;
	std::shared_ptr<CTextInput> inputAddress;
	std::shared_ptr<CTextInput> inputPort;

	void connectToServer();
	void leaveScreen();
	void onChange(const std::string & newText);
	void startConnection(const std::string & addr = {}, ui16 port = 0);

public:
	CSimpleJoinScreen(bool host = true);
};

class CLoadingScreen : virtual public CWindowObject, virtual public Load::Progress
{
	std::shared_ptr<CPicture> backimg;
	std::vector<std::shared_ptr<CPicture>> images;
	std::shared_ptr<CPicture> loadFrame;
	std::vector<std::shared_ptr<CAnimImage>> progressBlocks;

	ImagePath getBackground();

public:	
	CLoadingScreen();
	CLoadingScreen(ImagePath background);
	~CLoadingScreen();

	void tick(uint32_t msPassed) override;
};


