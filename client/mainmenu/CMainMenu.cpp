/*
 * CMainMenu.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CMainMenu.h"

#include "CCampaignScreen.h"
#include "CreditsScreen.h"
#include "CHighScoreScreen.h"

#include "../lobby/CBonusSelection.h"
#include "../lobby/CSelectionBase.h"
#include "../lobby/CLobbyScreen.h"
#include "../media/IMusicPlayer.h"
#include "../gui/CursorHandler.h"
#include "../windows/GUIClasses.h"
#include "../gui/CGuiHandler.h"
#include "../gui/ShortcutHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../render/Canvas.h"
#include "../globalLobby/GlobalLobbyLoginWindow.h"
#include "../globalLobby/GlobalLobbyClient.h"
#include "../globalLobby/GlobalLobbyWindow.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/CTextInput.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../widgets/VideoWidget.h"
#include "../windows/InfoWindows.h"
#include "../CServerHandler.h"

#include "../CGameInfo.h"
#include "../CPlayerInterface.h"
#include "../Client.h"
#include "../CMT.h"

#include "../../CCallback.h"

#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/campaign/CampaignHandler.h"
#include "../../lib/serializer/CTypeList.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/CCompressedStream.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/modding/CModHandler.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/CStopWatch.h"
#include "../../lib/CThreadHelper.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/GameConstants.h"
#include "../../lib/CRandomGenerator.h"

std::shared_ptr<CMainMenu> CMM;
ISelectionScreenInfo * SEL = nullptr;

static void do_quit()
{
	GH.dispatchMainThread([]()
	{
		handleQuit(false);
	});
}

CMenuScreen::CMenuScreen(const JsonNode & configNode)
	: CWindowObject(BORDERED), config(configNode)
{
	OBJECT_CONSTRUCTION;

	background = std::make_shared<CPicture>(ImagePath::fromJson(config["background"]));
	if(config["scalable"].Bool())
		background->scaleTo(GH.screenDimensions());

	pos = background->center();

	for(const JsonNode & node : config["items"].Vector())
		menuNameToEntry.push_back(node["name"].String());

	for(const JsonNode & node : config["images"].Vector())
		images.push_back(CMainMenu::createPicture(node));

	//Hardcoded entry
	menuNameToEntry.push_back("credits");

	tabs = std::make_shared<CTabbedInt>(std::bind(&CMenuScreen::createTab, this, _1));
	if(!config["video"].isNull())
	{
		Point videoPosition(config["video"]["x"].Integer(), config["video"]["y"].Integer());
		videoPlayer = std::make_shared<VideoWidget>(videoPosition, VideoPath::fromJson(config["video"]["name"]), false);
	}
	else
		tabs->setRedrawParent(true);

}

std::shared_ptr<CIntObject> CMenuScreen::createTab(size_t index)
{
	if(config["items"].Vector().size() == index)
		return std::make_shared<CreditsScreen>(this->pos);
	else
		return std::make_shared<CMenuEntry>(this, config["items"].Vector()[index]);
}

void CMenuScreen::show(Canvas & to)
{
	// TODO: avoid excessive redraws
	CIntObject::showAll(to);
}

void CMenuScreen::activate()
{
	CCS->musich->playMusic(AudioPath::builtin("Music/MainMenu"), true, true);
	CIntObject::activate();
}

void CMenuScreen::switchToTab(size_t index)
{
	tabs->setActive(index);
}

void CMenuScreen::switchToTab(std::string name)
{
	switchToTab(vstd::find_pos(menuNameToEntry, name));
}

size_t CMenuScreen::getActiveTab() const
{
	return tabs->getActive();
}

//function for std::string -> std::function conversion for main menu
static std::function<void()> genCommand(CMenuScreen * menu, std::vector<std::string> menuType, const std::string & string)
{
	static const std::vector<std::string> commandType = {"to", "campaigns", "start", "load", "exit", "highscores"};

	static const std::vector<std::string> gameType = {"single", "multi", "campaign", "tutorial"};

	std::list<std::string> commands;
	boost::split(commands, string, boost::is_any_of("\t "));

	if(!commands.empty())
	{
		size_t index = std::find(commandType.begin(), commandType.end(), commands.front()) - commandType.begin();
		commands.pop_front();
		if(index > 3 || !commands.empty())
		{
			switch(index)
			{
			case 0: //to - switch to another tab, if such tab exists
			{
				size_t index2 = std::find(menuType.begin(), menuType.end(), commands.front()) - menuType.begin();
				if(index2 != menuType.size())
					return std::bind((void(CMenuScreen::*)(size_t))&CMenuScreen::switchToTab, menu, index2);
				break;
			}
			case 1: //open campaign selection window
			{
				return std::bind(&CMainMenu::openCampaignScreen, CMM, commands.front());
				break;
			}
			case 2: //start
			{
				switch(std::find(gameType.begin(), gameType.end(), commands.front()) - gameType.begin())
				{
				case 0:
					return []() { CMainMenu::openLobby(ESelectionScreen::newGame, true, {}, ELoadMode::NONE);};
				case 1:
					return []() { GH.windows().createAndPushWindow<CMultiMode>(ESelectionScreen::newGame); };
				case 2:
					return []() { CMainMenu::openLobby(ESelectionScreen::campaignList, true, {}, ELoadMode::NONE);};
				case 3:
					return std::bind(CMainMenu::startTutorial);
				}
				break;
			}
			case 3: //load
			{
				switch(std::find(gameType.begin(), gameType.end(), commands.front()) - gameType.begin())
				{
				case 0:
					return []() { CMainMenu::openLobby(ESelectionScreen::loadGame, true, {}, ELoadMode::SINGLE);};
				case 1:
					return []() { GH.windows().createAndPushWindow<CMultiMode>(ESelectionScreen::loadGame); };
				case 2:
					return []() { CMainMenu::openLobby(ESelectionScreen::loadGame, true, {}, ELoadMode::CAMPAIGN);};
				case 3:
					return []() { CMainMenu::openLobby(ESelectionScreen::loadGame, true, {}, ELoadMode::TUTORIAL);};

				}
			}
			break;
			case 4: //exit
			{
				return std::bind(CInfoWindow::showYesNoDialog, CGI->generaltexth->allTexts[69], std::vector<std::shared_ptr<CComponent>>(), do_quit, 0, PlayerColor(1));
			}
			break;
			case 5: //highscores
			{
				return std::bind(CMainMenu::openHighScoreScreen);
			}
			}
		}
	}
	logGlobal->error("Failed to parse command: %s", string);
	return std::function<void()>();
}

std::shared_ptr<CButton> CMenuEntry::createButton(CMenuScreen * parent, const JsonNode & button)
{
	std::function<void()> command = genCommand(parent, parent->menuNameToEntry, button["command"].String());

	std::pair<std::string, std::string> help;
	if(!button["help"].isNull() && button["help"].Float() > 0)
		help = CGI->generaltexth->zelp[(size_t)button["help"].Float()];

	int posx = static_cast<int>(button["x"].Float());
	if(posx < 0)
		posx = pos.w + posx;

	int posy = static_cast<int>(button["y"].Float());
	if(posy < 0)
		posy = pos.h + posy;

	EShortcut shortcut = GH.shortcuts().findShortcut(button["shortcut"].String());

	if (shortcut == EShortcut::NONE && !button["shortcut"].String().empty())
	{
		logGlobal->warn("Unknown shortcut '%s' found when loading main menu config!", button["shortcut"].String());
	}

	auto result = std::make_shared<CButton>(Point(posx, posy), AnimationPath::fromJson(button["name"]), help, command, shortcut);

	if (button["center"].Bool())
		result->moveBy(Point(-result->pos.w/2, -result->pos.h/2));
	return result;
}

CMenuEntry::CMenuEntry(CMenuScreen * parent, const JsonNode & config)
{
	OBJECT_CONSTRUCTION;
	setRedrawParent(true);
	pos = parent->pos;

	for(const JsonNode & node : config["images"].Vector())
		images.push_back(CMainMenu::createPicture(node));

	for(const JsonNode & node : config["buttons"].Vector())
	{
		buttons.push_back(createButton(parent, node));
		buttons.back()->setHoverable(true);
		buttons.back()->setRedrawParent(true);
	}
}

CMainMenuConfig::CMainMenuConfig()
	: campaignSets(JsonPath::builtin("config/campaignSets.json"))
	, config(JsonPath::builtin("config/mainmenu.json"))
{
	if (config["game-select"].Vector().empty())
		handleFatalError("Main menu config is invalid or corrupted. Please disable any mods or reinstall VCMI", false);
}

const CMainMenuConfig & CMainMenuConfig::get()
{
	static const CMainMenuConfig config;
	return config;
}

const JsonNode & CMainMenuConfig::getConfig() const
{
	return config;
}

const JsonNode & CMainMenuConfig::getCampaigns() const
{
	return campaignSets;
}

CMainMenu::CMainMenu()
{
	pos.w = GH.screenDimensions().x;
	pos.h = GH.screenDimensions().y;

	menu = std::make_shared<CMenuScreen>(CMainMenuConfig::get().getConfig()["window"]);
	OBJECT_CONSTRUCTION;
	backgroundAroundMenu = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), pos);
}

CMainMenu::~CMainMenu()
{
	if(GH.curInt == this)
		GH.curInt = nullptr;
}

void CMainMenu::activate()
{
	// check if screen was resized while main menu was inactive - e.g. in gameplay mode
	if (pos.dimensions() != GH.screenDimensions())
		onScreenResize();

	CIntObject::activate();
}

void CMainMenu::onScreenResize()
{
	pos.w = GH.screenDimensions().x;
	pos.h = GH.screenDimensions().y;

	menu = nullptr;
	menu = std::make_shared<CMenuScreen>(CMainMenuConfig::get().getConfig()["window"]);

	backgroundAroundMenu->pos = pos;
}

void CMainMenu::update()
{
	if(CMM != this->shared_from_this()) //don't update if you are not a main interface
		return;

	if(GH.windows().count() == 0)
	{
		GH.windows().pushWindow(CMM);
		GH.windows().pushWindow(menu);
		menu->switchToTab(menu->getActiveTab());
	}

	static bool warnedAboutModDependencies = false;

	if (!warnedAboutModDependencies)
	{
		warnedAboutModDependencies = true;
		auto errorMessages = CGI->modh->getModLoadErrors();

		if (!errorMessages.empty())
			CInfoWindow::showInfoDialog(errorMessages, std::vector<std::shared_ptr<CComponent>>(), PlayerColor(1));
	}

	// Handles mouse and key input
	GH.handleEvents();
	GH.windows().simpleRedraw();
}

void CMainMenu::openLobby(ESelectionScreen screenType, bool host, const std::vector<std::string> & names, ELoadMode loadMode)
{
	CSH->resetStateForLobby(screenType == ESelectionScreen::newGame ? EStartMode::NEW_GAME : EStartMode::LOAD_GAME, screenType, EServerMode::LOCAL, names);
	CSH->loadMode = loadMode;

	GH.windows().createAndPushWindow<CSimpleJoinScreen>(host);
}

void CMainMenu::openCampaignLobby(const std::string & campaignFileName, std::string campaignSet)
{
	auto ourCampaign = CampaignHandler::getCampaign(campaignFileName);
	ourCampaign->campaignSet = campaignSet;
	openCampaignLobby(ourCampaign);
}

void CMainMenu::openCampaignLobby(std::shared_ptr<CampaignState> campaign)
{
	CSH->resetStateForLobby(EStartMode::CAMPAIGN, ESelectionScreen::campaignList, EServerMode::LOCAL, {});
	CSH->campaignStateToSend = campaign;
	GH.windows().createAndPushWindow<CSimpleJoinScreen>();
}

void CMainMenu::openCampaignScreen(std::string name)
{
	auto const & config = CMainMenuConfig::get().getCampaigns();

	if(!vstd::contains(config.Struct(), name))
	{
		logGlobal->error("Unknown campaign set: %s", name);
		return;
	}

	bool campaignsFound = true;
	for (auto const & entry : config[name]["items"].Vector())
	{
		ResourcePath resourceID(entry["file"].String(), EResType::CAMPAIGN);
		if (!CResourceHandler::get()->existsResource(resourceID))
			campaignsFound = false;
	}

	if (!campaignsFound)
	{
		CInfoWindow::showInfoDialog(CGI->generaltexth->translate("vcmi.client.errors.missingCampaigns"), std::vector<std::shared_ptr<CComponent>>(), PlayerColor(1));
		return;
	}

	GH.windows().createAndPushWindow<CCampaignScreen>(config, name);
}

void CMainMenu::startTutorial()
{
	ResourcePath tutorialMap("Maps/Tutorial.tut", EResType::MAP);
	if(!CResourceHandler::get()->existsResource(tutorialMap))
	{
		CInfoWindow::showInfoDialog(CGI->generaltexth->translate("core.genrltxt.742"), std::vector<std::shared_ptr<CComponent>>(), PlayerColor(1));
		return;
	}
		
	auto mapInfo = std::make_shared<CMapInfo>();
	mapInfo->mapInit(tutorialMap.getName());
	CMainMenu::openLobby(ESelectionScreen::newGame, true, {}, ELoadMode::NONE);
	CSH->startMapAfterConnection(mapInfo);
}

void CMainMenu::openHighScoreScreen()
{
	GH.windows().createAndPushWindow<CHighScoreScreen>(CHighScoreScreen::HighScorePage::SCENARIO);
	return;
}

std::shared_ptr<CMainMenu> CMainMenu::create()
{
	if(!CMM)
		CMM = std::shared_ptr<CMainMenu>(new CMainMenu());

	return CMM;
}

std::shared_ptr<CPicture> CMainMenu::createPicture(const JsonNode & config)
{
	return std::make_shared<CPicture>(ImagePath::fromJson(config["name"]), (int)config["x"].Float(), (int)config["y"].Float());
}

CMultiMode::CMultiMode(ESelectionScreen ScreenType)
	: screenType(ScreenType)
{
	OBJECT_CONSTRUCTION;

	background = std::make_shared<CPicture>(ImagePath::builtin("MUPOPUP.bmp"));
	pos = background->center(); //center, window has size of bg graphic

	picture = std::make_shared<CPicture>(ImagePath::builtin("MUMAP.bmp"), 16, 77);

	statusBar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(7, 465, 440, 18), 7, 465));
	playerName = std::make_shared<CTextInput>(Rect(19, 436, 334, 16), background->getSurface());
	playerName->setText(getPlayersNames()[0]);
	playerName->setCallback(std::bind(&CMultiMode::onNameChange, this, _1));

	buttonHotseat = std::make_shared<CButton>(Point(373, 78 + 57 * 0), AnimationPath::builtin("MUBHOT.DEF"), CGI->generaltexth->zelp[266], std::bind(&CMultiMode::hostTCP, this), EShortcut::MAIN_MENU_HOTSEAT);
	buttonLobby = std::make_shared<CButton>(Point(373, 78 + 57 * 1), AnimationPath::builtin("MUBONL.DEF"), CGI->generaltexth->zelp[265], std::bind(&CMultiMode::openLobby, this), EShortcut::MAIN_MENU_LOBBY);

	buttonHost = std::make_shared<CButton>(Point(373, 78 + 57 * 3), AnimationPath::builtin("MUBHOST.DEF"), CButton::tooltip(CGI->generaltexth->translate("vcmi.mainMenu.hostTCP"), ""), std::bind(&CMultiMode::hostTCP, this), EShortcut::MAIN_MENU_HOST_GAME);
	buttonJoin = std::make_shared<CButton>(Point(373, 78 + 57 * 4), AnimationPath::builtin("MUBJOIN.DEF"), CButton::tooltip(CGI->generaltexth->translate("vcmi.mainMenu.joinTCP"), ""), std::bind(&CMultiMode::joinTCP, this), EShortcut::MAIN_MENU_JOIN_GAME);

	buttonCancel = std::make_shared<CButton>(Point(373, 424), AnimationPath::builtin("MUBCANC.DEF"), CGI->generaltexth->zelp[288], [=](){ close();}, EShortcut::GLOBAL_CANCEL);
}

void CMultiMode::openLobby()
{
	close();
	CSH->getGlobalLobby().activateInterface();
}

void CMultiMode::hostTCP()
{
	auto savedScreenType = screenType;
	close();
	GH.windows().createAndPushWindow<CMultiPlayers>(getPlayersNames(), savedScreenType, true, ELoadMode::MULTI);
}

void CMultiMode::joinTCP()
{
	auto savedScreenType = screenType;
	close();
	GH.windows().createAndPushWindow<CMultiPlayers>(getPlayersNames(), savedScreenType, false, ELoadMode::MULTI);
}

std::vector<std::string> CMultiMode::getPlayersNames()
{
	std::vector<std::string> playerNames;

	std::string playerNameStr = settings["general"]["playerName"].String();
	if (playerNameStr == "Player")
		playerNameStr = CGI->generaltexth->translate("core.genrltxt.434");
	playerNames.push_back(playerNameStr);

	for (const auto & playerName : settings["general"]["multiPlayerNames"].Vector())
	{
		const std::string &nameStr = playerName.String();
		if (!nameStr.empty())
		{
			playerNames.push_back(nameStr);
		}
	}

	return playerNames;
}

void CMultiMode::onNameChange(std::string newText)
{
	Settings name = settings.write["general"]["playerName"];
	name->String() = newText;
}

CMultiPlayers::CMultiPlayers(const std::vector<std::string> & playerNames, ESelectionScreen ScreenType, bool Host, ELoadMode LoadMode)
	: loadMode(LoadMode), screenType(ScreenType), host(Host)
{
	OBJECT_CONSTRUCTION;
	background = std::make_shared<CPicture>(ImagePath::builtin("MUHOTSEA.bmp"));
	pos = background->center(); //center, window has size of bg graphic

	std::string text = CGI->generaltexth->allTexts[446];
	boost::replace_all(text, "\t", "\n");
	textTitle = std::make_shared<CTextBox>(text, Rect(25, 20, 315, 50), 0, FONT_BIG, ETextAlignment::CENTER, Colors::WHITE); //HOTSEAT	Please enter names

	for(int i = 0; i < inputNames.size(); i++)
	{
		inputNames[i] = std::make_shared<CTextInput>(Rect(60, 85 + i * 30, 280, 16), background->getSurface());
		inputNames[i]->setCallback(std::bind(&CMultiPlayers::onChange, this, _1));
	}

	buttonOk = std::make_shared<CButton>(Point(95, 338), AnimationPath::builtin("MUBCHCK.DEF"), CGI->generaltexth->zelp[560], std::bind(&CMultiPlayers::enterSelectionScreen, this), EShortcut::GLOBAL_ACCEPT);
	buttonCancel = std::make_shared<CButton>(Point(205, 338), AnimationPath::builtin("MUBCANC.DEF"), CGI->generaltexth->zelp[561], [=](){ close();}, EShortcut::GLOBAL_CANCEL);
	statusBar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(7, 381, 348, 18), 7, 381));

	for(int i = 0; i < playerNames.size(); i++)
	{
		inputNames[i]->setText(playerNames[i]);
	}
#ifndef VCMI_MOBILE
	inputNames[0]->giveFocus();
#endif
}

void CMultiPlayers::onChange(std::string newText)
{
}

void CMultiPlayers::enterSelectionScreen()
{
	std::vector<std::string> playerNames;
	for(auto playerName : inputNames)
	{
		if (playerName->getText().length())
			playerNames.push_back(playerName->getText());
	}

	Settings playerName = settings.write["general"]["playerName"];
	Settings multiPlayerNames = settings.write["general"]["multiPlayerNames"];
	multiPlayerNames->Vector().clear();
	if (!playerNames.empty())
	{
		playerName->String() = playerNames.front();
		for (auto playerNameIt = playerNames.begin()+1; playerNameIt != playerNames.end(); playerNameIt++)
		{
			multiPlayerNames->Vector().push_back(JsonNode(*playerNameIt));
		}
	}
	else
	{
		// Without the check the saving crashes directly.
		// When empty reset the player's name. This would translate to it being
		// the default for the next run. But enables deleting players, by just
		// deleting the names, otherwise some UI element should have been added.
		playerName->clear();
	}

	CMainMenu::openLobby(screenType, host, playerNames, loadMode);
}

CSimpleJoinScreen::CSimpleJoinScreen(bool host)
{
	OBJECT_CONSTRUCTION;
	background = std::make_shared<CPicture>(ImagePath::builtin("MUDIALOG.bmp")); // address background
	pos = background->center(); //center, window has size of bg graphic (x,y = 396,278 w=232 h=212)

	textTitle = std::make_shared<CTextBox>("", Rect(20, 20, 205, 50), 0, FONT_BIG, ETextAlignment::CENTER, Colors::WHITE);
	inputAddress = std::make_shared<CTextInput>(Rect(25, 68, 175, 16), background->getSurface());
	inputPort = std::make_shared<CTextInput>(Rect(25, 115, 175, 16), background->getSurface());
	buttonOk = std::make_shared<CButton>(Point(26, 142), AnimationPath::builtin("MUBCHCK.DEF"), CGI->generaltexth->zelp[560], std::bind(&CSimpleJoinScreen::connectToServer, this), EShortcut::GLOBAL_ACCEPT);
	if(host && !settings["session"]["donotstartserver"].Bool())
	{
		textTitle->setText(CGI->generaltexth->translate("vcmi.mainMenu.serverConnecting"));
		buttonOk->block(true);
		startConnection();
	}
	else
	{
		textTitle->setText(CGI->generaltexth->translate("vcmi.mainMenu.serverAddressEnter"));
		inputAddress->setCallback(std::bind(&CSimpleJoinScreen::onChange, this, _1));
		inputPort->setCallback(std::bind(&CSimpleJoinScreen::onChange, this, _1));
		inputPort->setFilterNumber(0, 65535);
		inputAddress->giveFocus();
	}
	inputAddress->setText(host ? CSH->getLocalHostname() : CSH->getRemoteHostname());
	inputPort->setText(std::to_string(host ? CSH->getLocalPort() : CSH->getRemotePort()));
	buttonOk->block(inputAddress->getText().empty() || inputPort->getText().empty());

	buttonCancel = std::make_shared<CButton>(Point(142, 142), AnimationPath::builtin("MUBCANC.DEF"), CGI->generaltexth->zelp[561], std::bind(&CSimpleJoinScreen::leaveScreen, this), EShortcut::GLOBAL_CANCEL);
	statusBar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(7, 186, 218, 18), 7, 186));
}

void CSimpleJoinScreen::connectToServer()
{
	textTitle->setText(CGI->generaltexth->translate("vcmi.mainMenu.serverConnecting"));
	buttonOk->block(true);
	GH.stopTextInput();

	startConnection(inputAddress->getText(), boost::lexical_cast<ui16>(inputPort->getText()));
}

void CSimpleJoinScreen::leaveScreen()
{
	textTitle->setText(CGI->generaltexth->translate("vcmi.mainMenu.serverClosing"));
	CSH->setState(EClientState::CONNECTION_CANCELLED);
}

void CSimpleJoinScreen::onChange(const std::string & newText)
{
	buttonOk->block(inputAddress->getText().empty() || inputPort->getText().empty());
}

void CSimpleJoinScreen::startConnection(const std::string & addr, ui16 port)
{
	if(addr.empty())
		CSH->startLocalServerAndConnect(false);
	else
		CSH->connectToServer(addr, port);
}

CLoadingScreen::CLoadingScreen()
	: CWindowObject(BORDERED, getBackground())
{
	OBJECT_CONSTRUCTION;
	
	addUsedEvents(TIME);
	
	CCS->musich->stopMusic(5000);
	
	const auto & conf = CMainMenuConfig::get().getConfig()["loading"];
	if(conf.isStruct())
	{
		const int posx = conf["x"].Integer();
		const int posy = conf["y"].Integer();
		const int blockSize = conf["size"].Integer();
		const int blocksAmount = conf["amount"].Integer();
		
		for(int i = 0; i < blocksAmount; ++i)
		{
			progressBlocks.push_back(std::make_shared<CAnimImage>(AnimationPath::fromJson(conf["name"]), i, 0, posx + i * blockSize, posy));
			progressBlocks.back()->deactivate();
			progressBlocks.back()->visible = false;
		}
	}
}

CLoadingScreen::~CLoadingScreen()
{
}

void CLoadingScreen::tick(uint32_t msPassed)
{
	if(!progressBlocks.empty())
	{
		int status = float(get()) / 255.f * progressBlocks.size();
		
		for(int i = 0; i < status; ++i)
		{
			progressBlocks.at(i)->activate();
			progressBlocks.at(i)->visible = true;
		}
	}
}

ImagePath CLoadingScreen::getBackground()
{
	ImagePath fname = ImagePath::builtin("loadbar");
	const auto & conf = CMainMenuConfig::get().getConfig()["loading"];

	if(conf.isStruct())
	{
		if(conf["background"].isVector())
			return ImagePath::fromJson(*RandomGeneratorUtil::nextItem(conf["background"].Vector(), CRandomGenerator::getDefault()));
		
		if(conf["background"].isString())
			return ImagePath::fromJson(conf["background"]);
		
		return fname;
	}
	
	if(conf.isVector() && !conf.Vector().empty())
		return ImagePath::fromJson(*RandomGeneratorUtil::nextItem(conf.Vector(), CRandomGenerator::getDefault()));
	
	return fname;
}
