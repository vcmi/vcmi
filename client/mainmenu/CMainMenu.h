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
#include "../../lib/JsonNode.h"

VCMI_LIB_NAMESPACE_BEGIN

class CCampaignState;

VCMI_LIB_NAMESPACE_END

class CTextInput;
class CGStatusBar;
class CTextBox;
class CTabbedInt;
class CAnimation;
class CButton;
class CFilledTexture;


// TODO: Find new location for these enums
enum ESelectionScreen : ui8 {
	unknown = 0, newGame, loadGame, saveGame, scenarioInfo, campaignList
};

enum ELoadMode : ui8
{
	NONE = 0, SINGLE, MULTI, CAMPAIGN
};

/// The main menu screens listed in the EState enum
class CMenuScreen : public CWindowObject
{
	const JsonNode & config;

	std::shared_ptr<CTabbedInt> tabs;

	std::shared_ptr<CPicture> background;
	std::vector<std::shared_ptr<CPicture>> images;

	std::shared_ptr<CIntObject> createTab(size_t index);

public:
	std::vector<std::string> menuNameToEntry;

	CMenuScreen(const JsonNode & configNode);

	void show(SDL_Surface * to) override;
	void activate() override;
	void deactivate() override;

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
	std::shared_ptr<CTextInput> playerName;
	std::shared_ptr<CButton> buttonHotseat;
	std::shared_ptr<CButton> buttonHost;
	std::shared_ptr<CButton> buttonJoin;
	std::shared_ptr<CButton> buttonCancel;
	std::shared_ptr<CGStatusBar> statusBar;

	CMultiMode(ESelectionScreen ScreenType);
	void hostTCP();
	void joinTCP();

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
	CMultiPlayers(const std::string & firstPlayer, ESelectionScreen ScreenType, bool Host, ELoadMode LoadMode);
};

/// Manages the configuration of pregame GUI elements like campaign screen, main menu, loading screen,...
class CMainMenuConfig
{
public:
	static CMainMenuConfig & get();
	const JsonNode & getConfig() const;
	const JsonNode & getCampaigns() const;

private:
	CMainMenuConfig();

	const JsonNode campaignSets;
	const JsonNode config;
};

/// Handles background screen, loads graphics for victory/loss condition and random town or hero selection
class CMainMenu : public CIntObject, public IUpdateable, public std::enable_shared_from_this<CMainMenu>
{
	std::shared_ptr<CFilledTexture> backgroundAroundMenu;

	CMainMenu(); //Use CMainMenu::create

public:
	std::shared_ptr<CMenuScreen> menu;

	~CMainMenu();
	void update() override;
	static void openLobby(ESelectionScreen screenType, bool host, const std::vector<std::string> * names, ELoadMode loadMode);
	static void openCampaignLobby(const std::string & campaignFileName);
	static void openCampaignLobby(std::shared_ptr<CCampaignState> campaign);
	void openCampaignScreen(std::string name);

	static std::shared_ptr<CMainMenu> create();

	static std::shared_ptr<CPicture> createPicture(const JsonNode & config);

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
	void connectThread(const std::string addr = "", const ui16 inputPort = 0);

public:
	CSimpleJoinScreen(bool host = true);
};

class CLoadingScreen : public CWindowObject
{
	boost::thread loadingThread;

	std::string getBackground();

public:
	CLoadingScreen(std::function<void()> loader);
	~CLoadingScreen();

	void showAll(SDL_Surface * to) override;
};

extern std::shared_ptr<CMainMenu> CMM;
