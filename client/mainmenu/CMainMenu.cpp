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
#include "../media/IVideoPlayer.h"
#include "../gui/CursorHandler.h"
#include "../windows/GUIClasses.h"
#include "../GameEngine.h"
#include "../GameInstance.h"
#include "../eventsSDL/InputHandler.h"
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

#include "../CPlayerInterface.h"
#include "../Client.h"
#include "../CMT.h"

#include "../../lib/texts/CGeneralTextHandler.h"
#include "../../lib/campaign/CampaignHandler.h"
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
#include "../../lib/GameLibrary.h"
#include "../../lib/json/JsonUtils.h"

#include <boost/lexical_cast.hpp>

ISelectionScreenInfo * SEL = nullptr;

CMenuScreen::CMenuScreen(const JsonNode & configNode)
	: CWindowObject(BORDERED), config(configNode)
{
	OBJECT_CONSTRUCTION;

	const auto& bgConfig = config["background"];
	if (bgConfig.isVector())
		background = std::make_shared<CPicture>(ImagePath::fromJson(*RandomGeneratorUtil::nextItem(bgConfig.Vector(), CRandomGenerator::getDefault())));

	if (bgConfig.isString())
		background = std::make_shared<CPicture>(ImagePath::fromJson(bgConfig));

	if(config["scalable"].Bool())
		background->scaleTo(ENGINE->screenDimensions());

	pos = background->center();

	for (const JsonNode& node : config["images"].Vector())
	{
		auto image = std::make_shared<CPicture>(ImagePath::fromJson(*RandomGeneratorUtil::nextItem(node["name"].Vector(), CRandomGenerator::getDefault())), Point(node["x"].Integer(), node["y"].Integer()));
		images.push_back(image);
	}

	if(!config["video"].isNull())
	{
		Point videoPosition(config["video"]["x"].Integer(), config["video"]["y"].Integer());
		videoPlayer = std::make_shared<VideoWidget>(videoPosition, VideoPath::fromJson(config["video"]["name"]), false);
	}

	for(const JsonNode & node : config["items"].Vector())
		menuNameToEntry.push_back(node["name"].String());

	//Hardcoded entry
	menuNameToEntry.push_back("credits");

	tabs = std::make_shared<CTabbedInt>(std::bind(&CMenuScreen::createTab, this, _1));
	if(config["video"].isNull())
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
				return std::bind(&CMainMenu::openCampaignScreen, GAME->mainmenu(), commands.front());
				break;
			}
			case 2: //start
			{
				switch(std::find(gameType.begin(), gameType.end(), commands.front()) - gameType.begin())
				{
				case 0:
					return []() { CMainMenu::openLobby(ESelectionScreen::newGame, true, {}, ELoadMode::NONE); };
				case 1:
					return []() { ENGINE->windows().createAndPushWindow<CMultiMode>(ESelectionScreen::newGame); };
				case 2:
					return []() { CMainMenu::openLobby(ESelectionScreen::campaignList, true, {}, ELoadMode::NONE); };
				case 3:
					return []() { CMainMenu::startTutorial(); };
				}
				break;
			}
			case 3: //load
			{
				switch(std::find(gameType.begin(), gameType.end(), commands.front()) - gameType.begin())
				{
				case 0:
					return []() { CMainMenu::openLobby(ESelectionScreen::loadGame, true, {}, ELoadMode::SINGLE); };
				case 1:
					return []() { ENGINE->windows().createAndPushWindow<CMultiMode>(ESelectionScreen::loadGame); };
				case 2:
					return []() { CMainMenu::openLobby(ESelectionScreen::loadGame, true, {}, ELoadMode::CAMPAIGN); };
				case 3:
					return []() { CMainMenu::openLobby(ESelectionScreen::loadGame, true, {}, ELoadMode::TUTORIAL); };

				}
			}
			break;
			case 4: //exit
			{
				return []() { CInfoWindow::showYesNoDialog(LIBRARY->generaltexth->allTexts[69], std::vector<std::shared_ptr<CComponent>>(), [](){GAME->onShutdownRequested(false);}, 0, PlayerColor(1)); };
			}
			break;
			case 5: //highscores
			{
				return []() { CMainMenu::openHighScoreScreen(); };
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
		help = LIBRARY->generaltexth->zelp[(size_t)button["help"].Float()];

	int posx = static_cast<int>(button["x"].Float());
	if(posx < 0)
		posx = pos.w + posx;

	int posy = static_cast<int>(button["y"].Float());
	if(posy < 0)
		posy = pos.h + posy;

	EShortcut shortcut = ENGINE->shortcuts().findShortcut(button["shortcut"].String());

	if (shortcut == EShortcut::NONE && !button["shortcut"].String().empty())
		logGlobal->warn("Unknown shortcut '%s' found when loading main menu config!", button["shortcut"].String());

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

	for (const JsonNode& node : config["buttons"].Vector())
	{
		auto tokens = node["command"].String().find(' ');
		std::pair<std::string, std::string> commandParts = {
			node["command"].String().substr(0, tokens),
			(tokens == std::string::npos) ? "" : node["command"].String().substr(tokens + 1)
		};

		if (commandParts.first == "campaigns")
		{
			const auto& campaign = CMainMenuConfig::get().getCampaigns()[commandParts.second];

			if (!campaign.isStruct())
			{
				logGlobal->warn("Campaign set %s not found", commandParts.second);
				continue;
			}

			bool fileExists = false;
			for (const auto& item : campaign["items"].Vector())
			{
				std::string filename = item["file"].String();

				if (CResourceHandler::get()->existsResource(ResourcePath(filename, EResType::CAMPAIGN)))
				{
					fileExists = true;
					break; 
				}
			}

			if (!fileExists)
			{
				logGlobal->warn("No valid files found for campaign set %s", commandParts.second);
				continue;
			}
		}

		buttons.push_back(createButton(parent, node));
		buttons.back()->setHoverable(true);
		buttons.back()->setRedrawParent(true);
	}
}

CMainMenuConfig::CMainMenuConfig()
	: campaignSets(JsonUtils::assembleFromFiles("config/campaignSets.json"))
	, config(JsonPath::builtin("config/mainmenu.json"))
{
	if (!config["scenario-selection"].isStruct())
		// Fallback for 1.6 mods
		if (config["game-select"].Vector().empty())
			handleFatalError("The main menu configuration file mainmenu.json is invalid or corrupted. Please check the file for errors, verify your mod setup, or reinstall VCMI to resolve the issue.", false);
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
	pos.w = ENGINE->screenDimensions().x;
	pos.h = ENGINE->screenDimensions().y;

	menu = std::make_shared<CMenuScreen>(CMainMenuConfig::get().getConfig()["window"]);
	OBJECT_CONSTRUCTION;
	backgroundAroundMenu = std::make_shared<CFilledTexture>(ImagePath::builtin("DIBOXBCK"), pos);
}

CMainMenu::~CMainMenu() = default;

void CMainMenu::playIntroVideos()
{
	auto playVideo = [](std::string video, bool rim, float scaleFactor, std::function<void(bool)> cb){
		if(ENGINE->video().open(VideoPath::builtin(video), scaleFactor))
			ENGINE->windows().createAndPushWindow<VideoWindow>(VideoPath::builtin(video), rim ? ImagePath::builtin("INTRORIM") : ImagePath::builtin(""), true, scaleFactor, [cb](bool skipped){ cb(skipped); });
		else
			cb(true);
	};

	playVideo("3DOLOGO.SMK", false, 1.25, [playVideo, this](bool skipped){
		if(!skipped)
			playVideo("NWCLOGO.SMK", false, 2, [playVideo, this](bool skipped){
				if(!skipped)
					playVideo("H3INTRO.SMK", true, 1, [this](bool skipped){
						playMusic();
					});
				else
					playMusic();
			});
		else
			playMusic();
	});
}

void CMainMenu::playMusic()
{
	ENGINE->music().playMusic(AudioPath::builtin("Music/MainMenu"), true, true);
}

void CMainMenu::activate()
{
	// check if screen was resized while main menu was inactive - e.g. in gameplay mode
	if (pos.dimensions() != ENGINE->screenDimensions())
		onScreenResize();

	CIntObject::activate();
}

void CMainMenu::onScreenResize()
{
	pos.w = ENGINE->screenDimensions().x;
	pos.h = ENGINE->screenDimensions().y;

	menu = nullptr;
	menu = std::make_shared<CMenuScreen>(CMainMenuConfig::get().getConfig()["window"]);

	backgroundAroundMenu->pos = pos;
}

void CMainMenu::makeActiveInterface()
{
	ENGINE->windows().pushWindow(GAME->mainmenu());
	ENGINE->windows().pushWindow(menu);
	menu->switchToTab(menu->getActiveTab());
}

void CMainMenu::openLobby(ESelectionScreen screenType, bool host, const std::vector<std::string> & names, ELoadMode loadMode)
{
	GAME->server().resetStateForLobby(screenType == ESelectionScreen::newGame ? EStartMode::NEW_GAME : EStartMode::LOAD_GAME, screenType, EServerMode::LOCAL, names);
	GAME->server().loadMode = loadMode;

	ENGINE->windows().createAndPushWindow<CSimpleJoinScreen>(host);
}

void CMainMenu::openCampaignLobby(const std::string & campaignFileName, std::string campaignSet)
{
	auto ourCampaign = CampaignHandler::getCampaign(campaignFileName);
	ourCampaign->campaignSet = campaignSet;
	openCampaignLobby(ourCampaign);
}

void CMainMenu::openCampaignLobby(std::shared_ptr<CampaignState> campaign)
{
	GAME->server().resetStateForLobby(EStartMode::CAMPAIGN, ESelectionScreen::campaignList, EServerMode::LOCAL, {});
	GAME->server().campaignStateToSend = campaign;
	ENGINE->windows().createAndPushWindow<CSimpleJoinScreen>();
}

void CMainMenu::openCampaignScreen(std::string name)
{
	auto const & config = CMainMenuConfig::get().getCampaigns();

	if(!vstd::contains(config.Struct(), name))
	{
		logGlobal->error("Unknown campaign set: %s", name);
		return;
	}

	ENGINE->windows().createAndPushWindow<CCampaignScreen>(config, name);
}

void CMainMenu::startTutorial()
{
	ResourcePath tutorialMap("Maps/Tutorial.tut", EResType::MAP);
	if(!CResourceHandler::get()->existsResource(tutorialMap))
	{
		CInfoWindow::showInfoDialog(LIBRARY->generaltexth->translate("core.genrltxt.742"), std::vector<std::shared_ptr<CComponent>>(), PlayerColor(1));
		return;
	}
		
	auto mapInfo = std::make_shared<CMapInfo>();
	mapInfo->mapInit(tutorialMap.getName());
	CMainMenu::openLobby(ESelectionScreen::newGame, true, {}, ELoadMode::NONE);
	GAME->server().startMapAfterConnection(mapInfo);
}

void CMainMenu::openHighScoreScreen()
{
	ENGINE->windows().createAndPushWindow<CHighScoreScreen>(CHighScoreScreen::HighScorePage::SCENARIO);
	return;
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

	const auto& multiplayerConfig = CMainMenuConfig::get().getConfig()["multiplayer"];
	if (multiplayerConfig.isVector() && !multiplayerConfig.Vector().empty())
		picture = std::make_shared<CPicture>(ImagePath::fromJson(*RandomGeneratorUtil::nextItem(multiplayerConfig.Vector(), CRandomGenerator::getDefault())), 16, 77);

	textTitle = std::make_shared<CTextBox>("", Rect(7, 18, 440, 50), 0, FONT_BIG, ETextAlignment::CENTER, Colors::WHITE);
	textTitle->setText(LIBRARY->generaltexth->zelp[263].second);

	statusBar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(7, 465, 440, 18), 7, 465));
	playerName = std::make_shared<CTextInput>(Rect(19, 436, 334, 16), background->getSurface());
	playerName->setText(getPlayersNames()[0]);
	playerName->setCallback(std::bind(&CMultiMode::onNameChange, this, _1));

	buttonHotseat = std::make_shared<CButton>(Point(373, 78 + 57 * 0), AnimationPath::builtin("MUBHOT.DEF"), LIBRARY->generaltexth->zelp[266], std::bind(&CMultiMode::hostTCP, this, EShortcut::MAIN_MENU_HOTSEAT), EShortcut::MAIN_MENU_HOTSEAT);
	buttonLobby = std::make_shared<CButton>(Point(373, 78 + 57 * 1), AnimationPath::builtin("MUBONL.DEF"), LIBRARY->generaltexth->zelp[265], std::bind(&CMultiMode::openLobby, this), EShortcut::MAIN_MENU_LOBBY);

	buttonHost = std::make_shared<CButton>(Point(373, 78 + 57 * 3), AnimationPath::builtin("MUBHOST.DEF"), CButton::tooltip(LIBRARY->generaltexth->translate("vcmi.mainMenu.hostTCP"), ""), std::bind(&CMultiMode::hostTCP, this, EShortcut::MAIN_MENU_HOST_GAME), EShortcut::MAIN_MENU_HOST_GAME);
	buttonJoin = std::make_shared<CButton>(Point(373, 78 + 57 * 4), AnimationPath::builtin("MUBJOIN.DEF"), CButton::tooltip(LIBRARY->generaltexth->translate("vcmi.mainMenu.joinTCP"), ""), std::bind(&CMultiMode::joinTCP, this, EShortcut::MAIN_MENU_JOIN_GAME), EShortcut::MAIN_MENU_JOIN_GAME);

	buttonCancel = std::make_shared<CButton>(Point(373, 424), AnimationPath::builtin("MUBCANC.DEF"), LIBRARY->generaltexth->zelp[288], [this](){ close();}, EShortcut::GLOBAL_CANCEL);
}

void CMultiMode::openLobby()
{
	close();
	GAME->server().getGlobalLobby().activateInterface();
}

void CMultiMode::hostTCP(EShortcut shortcut)
{
	auto savedScreenType = screenType;
	close();
	ENGINE->windows().createAndPushWindow<CMultiPlayers>(getPlayersNames(), savedScreenType, true, ELoadMode::MULTI, shortcut);
}

void CMultiMode::joinTCP(EShortcut shortcut)
{
	auto savedScreenType = screenType;
	close();
	ENGINE->windows().createAndPushWindow<CMultiPlayers>(getPlayersNames(), savedScreenType, false, ELoadMode::MULTI, shortcut);
}

std::vector<std::string> CMultiMode::getPlayersNames()
{
	std::vector<std::string> playerNames;

	std::string playerNameStr = settings["general"]["playerName"].String();
	if (playerNameStr == "Player")
		playerNameStr = LIBRARY->generaltexth->translate("core.genrltxt.434");
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

CMultiPlayers::CMultiPlayers(const std::vector<std::string>& playerNames, ESelectionScreen ScreenType, bool Host, ELoadMode LoadMode, EShortcut shortcut)
	: loadMode(LoadMode), screenType(ScreenType), host(Host)
{
	OBJECT_CONSTRUCTION;
	background = std::make_shared<CPicture>(ImagePath::builtin("MUHOTSEA.bmp"));
	pos = background->center(); //center, window has size of bg graphic

	std::string text;
	switch (shortcut)
	{
	case EShortcut::MAIN_MENU_HOTSEAT:
		text = LIBRARY->generaltexth->allTexts[446];
		boost::replace_all(text, "\t", "\n");
		break;
	case EShortcut::MAIN_MENU_HOST_GAME:
		text = LIBRARY->generaltexth->translate("vcmi.mainMenu.hostTCP");
		break;
	case EShortcut::MAIN_MENU_JOIN_GAME:
		text = LIBRARY->generaltexth->translate("vcmi.mainMenu.joinTCP");
		break;
	}

	textTitle = std::make_shared<CTextBox>(text, Rect(25, 10, 315, 60), 0, FONT_BIG, ETextAlignment::CENTER, Colors::WHITE);

	for(int i = 0; i < inputNames.size(); i++)
	{
		inputNames[i] = std::make_shared<CTextInput>(Rect(60, 85 + i * 30, 280, 16), background->getSurface());
		inputNames[i]->setCallback(std::bind(&CMultiPlayers::onChange, this, _1));
	}

	buttonOk = std::make_shared<CButton>(Point(95, 338), AnimationPath::builtin("MUBCHCK.DEF"), LIBRARY->generaltexth->zelp[560], std::bind(&CMultiPlayers::enterSelectionScreen, this), EShortcut::GLOBAL_ACCEPT);
	buttonCancel = std::make_shared<CButton>(Point(205, 338), AnimationPath::builtin("MUBCANC.DEF"), LIBRARY->generaltexth->zelp[561], [this](){ close();}, EShortcut::GLOBAL_CANCEL);
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

	textTitle = std::make_shared<CTextBox>("", Rect(20, 10, 205, 50), 0, FONT_BIG, ETextAlignment::CENTER, Colors::WHITE);
	inputAddress = std::make_shared<CTextInput>(Rect(25, 68, 175, 16), background->getSurface());
	inputPort = std::make_shared<CTextInput>(Rect(25, 115, 175, 16), background->getSurface());
	buttonOk = std::make_shared<CButton>(Point(26, 142), AnimationPath::builtin("MUBCHCK.DEF"), LIBRARY->generaltexth->zelp[560], std::bind(&CSimpleJoinScreen::connectToServer, this), EShortcut::GLOBAL_ACCEPT);
	if(host && !settings["session"]["donotstartserver"].Bool())
	{
		textTitle->setText(LIBRARY->generaltexth->translate("vcmi.mainMenu.serverConnecting"));
		buttonOk->block(true);
		startConnection();
	}
	else
	{
		textTitle->setText(LIBRARY->generaltexth->translate("vcmi.mainMenu.serverAddressEnter"));
		inputAddress->setCallback(std::bind(&CSimpleJoinScreen::onChange, this, _1));
		inputPort->setCallback(std::bind(&CSimpleJoinScreen::onChange, this, _1));
		inputPort->setFilterNumber(0, 65535);
		inputAddress->giveFocus();
	}
	inputAddress->setText(host ? GAME->server().getLocalHostname() : GAME->server().getRemoteHostname());
	inputPort->setText(std::to_string(host ? GAME->server().getLocalPort() : GAME->server().getRemotePort()));
	buttonOk->block(inputAddress->getText().empty() || inputPort->getText().empty());

	buttonCancel = std::make_shared<CButton>(Point(142, 142), AnimationPath::builtin("MUBCANC.DEF"), LIBRARY->generaltexth->zelp[561], std::bind(&CSimpleJoinScreen::leaveScreen, this), EShortcut::GLOBAL_CANCEL);
	statusBar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(7, 186, 218, 18), 7, 186));
}

void CSimpleJoinScreen::connectToServer()
{
	textTitle->setText(LIBRARY->generaltexth->translate("vcmi.mainMenu.serverConnecting"));
	buttonOk->block(true);
	ENGINE->input().stopTextInput();

	startConnection(inputAddress->getText(), boost::lexical_cast<ui16>(inputPort->getText()));
}

void CSimpleJoinScreen::leaveScreen()
{
	textTitle->setText(LIBRARY->generaltexth->translate("vcmi.mainMenu.serverClosing"));
	GAME->server().setState(EClientState::CONNECTION_CANCELLED);
}

void CSimpleJoinScreen::onChange(const std::string & newText)
{
	buttonOk->block(inputAddress->getText().empty() || inputPort->getText().empty());
}

void CSimpleJoinScreen::startConnection(const std::string & addr, ui16 port)
{
	if(addr.empty())
		GAME->server().startLocalServerAndConnect(false);
	else
		GAME->server().connectToServer(addr, port);
}

CLoadingScreen::CLoadingScreen()
	: CLoadingScreen(getBackground())
{
}

CLoadingScreen::CLoadingScreen(ImagePath background)
	: CWindowObject(BORDERED, background)
{
	OBJECT_CONSTRUCTION;

	addUsedEvents(TIME);

	ENGINE->music().stopMusic(5000);

	const auto& conf = CMainMenuConfig::get().getConfig()["loading"];

	const auto& backgroundConfig = conf["background"];
	if (!backgroundConfig.Vector().empty())
		backimg = std::make_shared<CPicture>(ImagePath::fromJson(*RandomGeneratorUtil::nextItem(backgroundConfig.Vector(), CRandomGenerator::getDefault())));

	for (const JsonNode& node : conf["images"].Vector())
	{
		auto image = std::make_shared<CPicture>(ImagePath::fromJson(*RandomGeneratorUtil::nextItem(node["name"].Vector(), CRandomGenerator::getDefault())), Point(node["x"].Integer(), node["y"].Integer()));
		images.push_back(image);
	}

	const auto& loadframeConfig = conf["loadframe"];
	if (loadframeConfig.isStruct())
	{
		loadFrame = std::make_shared<CPicture>(
			ImagePath::fromJson(*RandomGeneratorUtil::nextItem(loadframeConfig["name"].Vector(), CRandomGenerator::getDefault())),
			loadframeConfig["x"].Integer(),
			loadframeConfig["y"].Integer()
			);
	}

	const auto& loadbarConfig = conf["loadbar"];
	if (loadbarConfig.isStruct())
	{
		AnimationPath loadbarPath = AnimationPath::fromJson(*RandomGeneratorUtil::nextItem(loadbarConfig["name"].Vector(), CRandomGenerator::getDefault()));
		const int posx = loadbarConfig["x"].Integer();
		const int posy = loadbarConfig["y"].Integer();
		const int blockSize = loadbarConfig["size"].Integer();
		const int blocksAmount = loadbarConfig["amount"].Integer();
		for (int i = 0; i < blocksAmount; ++i) 
		{
			progressBlocks.push_back(std::make_shared<CAnimImage>(loadbarPath, i, 0, posx + i * blockSize, posy));
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
