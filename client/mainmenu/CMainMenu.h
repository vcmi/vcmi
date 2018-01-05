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

#include "../lib/StartInfo.h"
#include "../lib/FunctionList.h"
#include "../lib/mapping/CMapInfo.h"
#include "../lib/rmg/CMapGenerator.h"
#include "windows/CWindowObject.h"


class CMapInfo;
class CMusicHandler;
class CMapHeader;
class CCampaignHeader;
class CTextInput;
class CCampaign;
class CGStatusBar;
class CTextBox;
class CConnection;
class JsonNode;
class CMultiLineLabel;
class CToggleButton;
class CToggleGroup;
class CTabbedInt;
class CAnimation;
class CAnimImage;
class CButton;
class CLabel;
class CSlider;
struct ClientPlayer;


// TODO: Find new location for these enums
enum ESelectionScreen : ui8 {
	unknown = 0, newGame, loadGame, saveGame, scenarioInfo, campaignList
};

enum ELoadMode : ui8
{
	NONE = 0, SINGLE, MULTI, CAMPAIGN
};

namespace boost{ class thread; class recursive_mutex;}

/// The main menu screens listed in the EState enum
class CMenuScreen : public CIntObject
{
	const JsonNode & config;

	CTabbedInt * tabs;

	CPicture * background;
	std::vector<CPicture *> images;

	CIntObject * createTab(size_t index);

public:
	std::vector<std::string> menuNameToEntry;

	CMenuScreen(const JsonNode & configNode);

	void showAll(SDL_Surface * to) override;
	void show(SDL_Surface * to) override;
	void activate() override;
	void deactivate() override;

	void switchToTab(size_t index);
};

class CMenuEntry : public CIntObject
{
	std::vector<CPicture *> images;
	std::vector<CButton *> buttons;

	CButton * createButton(CMenuScreen * parent, const JsonNode & button);

public:
	CMenuEntry(CMenuScreen * parent, const JsonNode & config);
};

/// Multiplayer mode
class CMultiMode : public CIntObject
{
public:
	ESelectionScreen screenType;
	CPicture * bg;
	CTextInput * txt;
	CButton * btns[7]; //0 - hotseat, 6 - cancel
	CGStatusBar * bar;

	CMultiMode(ESelectionScreen ScreenType);
	void hostTCP();
	void joinTCP();

	void onNameChange(std::string newText);
};

/// Hot seat player window
class CMultiPlayers : public CIntObject
{
	bool host;
	ELoadMode loadMode;
	ESelectionScreen screenType;
	CPicture * bg;
	CTextBox * title;
	CTextInput * txt[8];
	CButton * ok, * cancel;
	CGStatusBar * bar;

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
class CMainMenu : public CIntObject, public IUpdateable
{
	void loadGraphics();
	void disposeGraphics();

	CMainMenu(); //Use createIfNotPresent

public:
	CMenuScreen * menu;

	std::shared_ptr<CAnimation> victoryIcons, lossIcons;

	~CMainMenu();
	void update() override;
	static void openLobby(ESelectionScreen screenType, bool host, const std::vector<std::string> * names, ELoadMode loadMode);
	static void openCampaignLobby(const std::string & campaignFileName);
	static void openCampaignLobby(std::shared_ptr<CCampaignState> campaign);
	void openCampaignScreen(std::string name);

	static CMainMenu * create();
	void removeFromGui();
	static void showLoadingScreen(std::function<void()> loader);

	static CPicture * createPicture(const JsonNode & config);

};

/// Simple window to enter the server's address.
class CSimpleJoinScreen : public CIntObject
{
	CPicture * bg;
	CTextBox * title;
	CButton * ok, * cancel;
	CGStatusBar * bar;
	CTextInput * address;
	CTextInput * port;

	void connectToServer();
	void leaveScreen();
	void onChange(const std::string & newText);
	void connectThread(const std::string addr = "", const ui16 port = 0);

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

extern CMainMenu * CMM;
