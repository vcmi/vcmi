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

#include "../lobby/CBonusSelection.h"
#include "../lobby/CSelectionBase.h"
#include "../lobby/CLobbyScreen.h"
#include "../gui/CursorHandler.h"
#include "../windows/GUIClasses.h"
#include "../gui/CGuiHandler.h"
#include "../gui/ShortcutHandler.h"
#include "../gui/Shortcut.h"
#include "../gui/WindowHandler.h"
#include "../render/Canvas.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../windows/InfoWindows.h"
#include "../CServerHandler.h"

#include "../CGameInfo.h"
#include "../CMusicHandler.h"
#include "../CVideoHandler.h"
#include "../CPlayerInterface.h"
#include "../Client.h"
#include "../CMT.h"

#include "../../CCallback.h"

#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/JsonNode.h"
#include "../../lib/campaign/CampaignHandler.h"
#include "../../lib/serializer/Connection.h"
#include "../../lib/serializer/CTypeList.h"
#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/CCompressedStream.h"
#include "../../lib/mapping/CMapInfo.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/CStopWatch.h"
#include "../../lib/NetPacksLobby.h"
#include "../../lib/ThreadUtilities.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/GameConstants.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/CondSh.h"

#if defined(SINGLE_PROCESS_APP) && defined(VCMI_ANDROID)
#include "../../server/CVCMIServer.h"
#include <SDL.h>
#endif

std::shared_ptr<CMainMenu> CMM;
ISelectionScreenInfo * SEL;

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
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	background = std::make_shared<CPicture>(config["background"].String());
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
	if(!config["video"].isNull())
		CCS->videoh->update((int)config["video"]["x"].Float() + pos.x, (int)config["video"]["y"].Float() + pos.y, to.getInternalSurface(), true, false);
	CIntObject::show(to);
}

void CMenuScreen::activate()
{
	CCS->musich->playMusic("Music/MainMenu", true, true);
	if(!config["video"].isNull())
		CCS->videoh->open(config["video"]["name"].String());
	CIntObject::activate();
}

void CMenuScreen::deactivate()
{
	if(!config["video"].isNull())
		CCS->videoh->close();

	CIntObject::deactivate();
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

//funciton for std::string -> std::function conversion for main menu
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
					return std::bind(CMainMenu::openLobby, ESelectionScreen::newGame, true, nullptr, ELoadMode::NONE);
				case 1:
					return []() { GH.windows().createAndPushWindow<CMultiMode>(ESelectionScreen::newGame); };
				case 2:
					return std::bind(CMainMenu::openLobby, ESelectionScreen::campaignList, true, nullptr, ELoadMode::NONE);
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
					return std::bind(CMainMenu::openLobby, ESelectionScreen::loadGame, true, nullptr, ELoadMode::SINGLE);
				case 1:
					return []() { GH.windows().createAndPushWindow<CMultiMode>(ESelectionScreen::loadGame); };
				case 2:
					return std::bind(CMainMenu::openLobby, ESelectionScreen::loadGame, true, nullptr, ELoadMode::CAMPAIGN);
				case 3:
					return std::bind(CMainMenu::openLobby, ESelectionScreen::loadGame, true, nullptr, ELoadMode::TUTORIAL);
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
				return std::bind(CInfoWindow::showInfoDialog, CGI->generaltexth->translate("vcmi.mainMenu.highscoresNotImplemented"), std::vector<std::shared_ptr<CComponent>>(), PlayerColor(1));
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

	auto result = std::make_shared<CButton>(Point(posx, posy), button["name"].String(), help, command, shortcut);

	if (button["center"].Bool())
		result->moveBy(Point(-result->pos.w/2, -result->pos.h/2));
	return result;
}

CMenuEntry::CMenuEntry(CMenuScreen * parent, const JsonNode & config)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	setRedrawParent(true);
	pos = parent->pos;

	for(const JsonNode & node : config["images"].Vector())
		images.push_back(CMainMenu::createPicture(node));

	for(const JsonNode & node : config["buttons"].Vector())
	{
		buttons.push_back(createButton(parent, node));
		buttons.back()->hoverable = true;
		buttons.back()->setRedrawParent(true);
	}
}

CMainMenuConfig::CMainMenuConfig()
	: campaignSets(JsonNode(ResourceID("config/campaignSets.json"))), config(JsonNode(ResourceID("config/mainmenu.json")))
{

}

CMainMenuConfig & CMainMenuConfig::get()
{
	static CMainMenuConfig config;
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

	GH.defActionsDef = 63;
	menu = std::make_shared<CMenuScreen>(CMainMenuConfig::get().getConfig()["window"]);
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	backgroundAroundMenu = std::make_shared<CFilledTexture>("DIBOXBCK", pos);
}

CMainMenu::~CMainMenu()
{
	boost::unique_lock<boost::recursive_mutex> lock(*CPlayerInterface::pim);

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

	// Handles mouse and key input
	GH.handleEvents();
	GH.windows().simpleRedraw();
}

void CMainMenu::openLobby(ESelectionScreen screenType, bool host, const std::vector<std::string> * names, ELoadMode loadMode)
{
	CSH->resetStateForLobby(screenType == ESelectionScreen::newGame ? StartInfo::NEW_GAME : StartInfo::LOAD_GAME, names);
	CSH->screenType = screenType;
	CSH->loadMode = loadMode;

	GH.windows().createAndPushWindow<CSimpleJoinScreen>(host);
}

void CMainMenu::openCampaignLobby(const std::string & campaignFileName)
{
	auto ourCampaign = CampaignHandler::getCampaign(campaignFileName);
	openCampaignLobby(ourCampaign);
}

void CMainMenu::openCampaignLobby(std::shared_ptr<CampaignState> campaign)
{
	CSH->resetStateForLobby(StartInfo::CAMPAIGN);
	CSH->screenType = ESelectionScreen::campaignList;
	CSH->campaignStateToSend = campaign;
	GH.windows().createAndPushWindow<CSimpleJoinScreen>();
}

void CMainMenu::openCampaignScreen(std::string name)
{
	if(vstd::contains(CMainMenuConfig::get().getCampaigns().Struct(), name))
	{
		GH.windows().createAndPushWindow<CCampaignScreen>(CMainMenuConfig::get().getCampaigns()[name]);
		return;
	}
	logGlobal->error("Unknown campaign set: %s", name);
}

void CMainMenu::startTutorial()
{
	ResourceID tutorialMap("Maps/Tutorial.tut", EResType::MAP);
	if(!CResourceHandler::get()->existsResource(tutorialMap))
	{
		CInfoWindow::showInfoDialog(CGI->generaltexth->translate("core.genrltxt.742"), std::vector<std::shared_ptr<CComponent>>(), PlayerColor(1));
		return;
	}
		
	auto mapInfo = std::make_shared<CMapInfo>();
	mapInfo->mapInit(tutorialMap.getName());
	CMainMenu::openLobby(ESelectionScreen::newGame, true, nullptr, ELoadMode::NONE);
	CSH->startMapAfterConnection(mapInfo);
}

std::shared_ptr<CMainMenu> CMainMenu::create()
{
	if(!CMM)
		CMM = std::shared_ptr<CMainMenu>(new CMainMenu());

	return CMM;
}

std::shared_ptr<CPicture> CMainMenu::createPicture(const JsonNode & config)
{
	return std::make_shared<CPicture>(config["name"].String(), (int)config["x"].Float(), (int)config["y"].Float());
}

CMultiMode::CMultiMode(ESelectionScreen ScreenType)
	: screenType(ScreenType)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	background = std::make_shared<CPicture>("MUPOPUP.bmp");
	pos = background->center(); //center, window has size of bg graphic

	picture = std::make_shared<CPicture>("MUMAP.bmp", 16, 77);

	statusBar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(7, 465, 440, 18), 7, 465));
	playerName = std::make_shared<CTextInput>(Rect(19, 436, 334, 16), background->getSurface());
	playerName->setText(getPlayerName());
	playerName->cb += std::bind(&CMultiMode::onNameChange, this, _1);

	buttonHotseat = std::make_shared<CButton>(Point(373, 78), "MUBHOT.DEF", CGI->generaltexth->zelp[266], std::bind(&CMultiMode::hostTCP, this));
	buttonHost = std::make_shared<CButton>(Point(373, 78 + 57 * 1), "MUBHOST.DEF", CButton::tooltip(CGI->generaltexth->translate("vcmi.mainMenu.hostTCP"), ""), std::bind(&CMultiMode::hostTCP, this));
	buttonJoin = std::make_shared<CButton>(Point(373, 78 + 57 * 2), "MUBJOIN.DEF", CButton::tooltip(CGI->generaltexth->translate("vcmi.mainMenu.joinTCP"), ""), std::bind(&CMultiMode::joinTCP, this));
	buttonCancel = std::make_shared<CButton>(Point(373, 424), "MUBCANC.DEF", CGI->generaltexth->zelp[288], [=](){ close();}, EShortcut::GLOBAL_CANCEL);
}

void CMultiMode::hostTCP()
{
	auto savedScreenType = screenType;
	close();
	GH.windows().createAndPushWindow<CMultiPlayers>(getPlayerName(), savedScreenType, true, ELoadMode::MULTI);
}

void CMultiMode::joinTCP()
{
	auto savedScreenType = screenType;
	close();
	GH.windows().createAndPushWindow<CMultiPlayers>(getPlayerName(), savedScreenType, false, ELoadMode::MULTI);
}

std::string CMultiMode::getPlayerName()
{
	std::string name = settings["general"]["playerName"].String();
	if(name == "Player")
		name = CGI->generaltexth->translate("vcmi.mainMenu.playerName");
	return name;
}

void CMultiMode::onNameChange(std::string newText)
{
	Settings name = settings.write["general"]["playerName"];
	name->String() = newText;
}

CMultiPlayers::CMultiPlayers(const std::string & firstPlayer, ESelectionScreen ScreenType, bool Host, ELoadMode LoadMode)
	: loadMode(LoadMode), screenType(ScreenType), host(Host)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	background = std::make_shared<CPicture>("MUHOTSEA.bmp");
	pos = background->center(); //center, window has size of bg graphic

	std::string text = CGI->generaltexth->allTexts[446];
	boost::replace_all(text, "\t", "\n");
	textTitle = std::make_shared<CTextBox>(text, Rect(25, 20, 315, 50), 0, FONT_BIG, ETextAlignment::CENTER, Colors::WHITE); //HOTSEAT	Please enter names

	for(int i = 0; i < inputNames.size(); i++)
	{
		inputNames[i] = std::make_shared<CTextInput>(Rect(60, 85 + i * 30, 280, 16), background->getSurface());
		inputNames[i]->cb += std::bind(&CMultiPlayers::onChange, this, _1);
	}

	buttonOk = std::make_shared<CButton>(Point(95, 338), "MUBCHCK.DEF", CGI->generaltexth->zelp[560], std::bind(&CMultiPlayers::enterSelectionScreen, this), EShortcut::GLOBAL_ACCEPT);
	buttonCancel = std::make_shared<CButton>(Point(205, 338), "MUBCANC.DEF", CGI->generaltexth->zelp[561], [=](){ close();}, EShortcut::GLOBAL_CANCEL);
	statusBar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(7, 381, 348, 18), 7, 381));

	inputNames[0]->setText(firstPlayer, true);
#ifndef VCMI_IOS
	inputNames[0]->giveFocus();
#endif
}

void CMultiPlayers::onChange(std::string newText)
{
}

void CMultiPlayers::enterSelectionScreen()
{
	std::vector<std::string> names;
	for(auto name : inputNames)
	{
		if(name->getText().length())
			names.push_back(name->getText());
	}

	Settings name = settings.write["general"]["playerName"];
	name->String() = names[0];

	CMainMenu::openLobby(screenType, host, &names, loadMode);
}

CSimpleJoinScreen::CSimpleJoinScreen(bool host)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	background = std::make_shared<CPicture>("MUDIALOG.bmp"); // address background
	pos = background->center(); //center, window has size of bg graphic (x,y = 396,278 w=232 h=212)

	textTitle = std::make_shared<CTextBox>("", Rect(20, 20, 205, 50), 0, FONT_BIG, ETextAlignment::CENTER, Colors::WHITE);
	inputAddress = std::make_shared<CTextInput>(Rect(25, 68, 175, 16), background->getSurface());
	inputPort = std::make_shared<CTextInput>(Rect(25, 115, 175, 16), background->getSurface());
	if(host && !settings["session"]["donotstartserver"].Bool())
	{
		textTitle->setText(CGI->generaltexth->translate("vcmi.mainMenu.serverConnecting"));
		startConnectThread();
	}
	else
	{
		textTitle->setText(CGI->generaltexth->translate("vcmi.mainMenu.serverAddressEnter"));
		inputAddress->cb += std::bind(&CSimpleJoinScreen::onChange, this, _1);
		inputPort->cb += std::bind(&CSimpleJoinScreen::onChange, this, _1);
		inputPort->filters += std::bind(&CTextInput::numberFilter, _1, _2, 0, 65535);
		buttonOk = std::make_shared<CButton>(Point(26, 142), "MUBCHCK.DEF", CGI->generaltexth->zelp[560], std::bind(&CSimpleJoinScreen::connectToServer, this), EShortcut::GLOBAL_ACCEPT);

		inputAddress->giveFocus();
	}
	inputAddress->setText(host ? CServerHandler::localhostAddress : CSH->getHostAddress(), true);
	inputPort->setText(std::to_string(CSH->getHostPort()), true);

	buttonCancel = std::make_shared<CButton>(Point(142, 142), "MUBCANC.DEF", CGI->generaltexth->zelp[561], std::bind(&CSimpleJoinScreen::leaveScreen, this), EShortcut::GLOBAL_CANCEL);
	statusBar = CGStatusBar::create(std::make_shared<CPicture>(background->getSurface(), Rect(7, 186, 218, 18), 7, 186));
}

void CSimpleJoinScreen::connectToServer()
{
	textTitle->setText(CGI->generaltexth->translate("vcmi.mainMenu.serverConnecting"));
	buttonOk->block(true);
	GH.stopTextInput();

	startConnectThread(inputAddress->getText(), boost::lexical_cast<ui16>(inputPort->getText()));
}

void CSimpleJoinScreen::leaveScreen()
{
	if(CSH->state == EClientState::CONNECTING)
	{
		textTitle->setText(CGI->generaltexth->translate("vcmi.mainMenu.serverClosing"));
		CSH->state = EClientState::CONNECTION_CANCELLED;
	}
	else if(GH.windows().isTopWindow(this))
	{
		close();
	}
}

void CSimpleJoinScreen::onChange(const std::string & newText)
{
	buttonOk->block(inputAddress->getText().empty() || inputPort->getText().empty());
}

void CSimpleJoinScreen::startConnectThread(const std::string & addr, ui16 port)
{
#if defined(SINGLE_PROCESS_APP) && defined(VCMI_ANDROID)
	// in single process build server must use same JNIEnv as client
	// as server runs in a separate thread, it must not attempt to search for Java classes (and they're already cached anyway)
	// https://github.com/libsdl-org/SDL/blob/main/docs/README-android.md#threads-and-the-java-vm
	CVCMIServer::reuseClientJNIEnv(SDL_AndroidGetJNIEnv());
#endif
	boost::thread connector(&CSimpleJoinScreen::connectThread, this, addr, port);

	connector.detach();
}

void CSimpleJoinScreen::connectThread(const std::string & addr, ui16 port)
{
	setThreadName("connectThread");
	if(!addr.length())
		CSH->startLocalServerAndConnect();
	else
		CSH->justConnectToServer(addr, port);

	// async call to prevent thread race
	GH.dispatchMainThread([this](){
		if(GH.windows().isTopWindow(this))
		{
			close();
		}
	});
}

CLoadingScreen::CLoadingScreen()
	: CWindowObject(BORDERED, getBackground())
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	
	addUsedEvents(TIME);
	
	CCS->musich->stopMusic(5000);
	
	const auto & conf = CMainMenuConfig::get().getConfig()["loading"];
	if(conf.isStruct())
	{
		const int posx = conf["x"].Integer(), posy = conf["y"].Integer();
		const int blockSize = conf["size"].Integer();
		const int blocksAmount = conf["amount"].Integer();
		
		for(int i = 0; i < blocksAmount; ++i)
		{
			progressBlocks.push_back(std::make_shared<CAnimImage>(conf["name"].String(), i, 0, posx + i * blockSize, posy));
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

std::string CLoadingScreen::getBackground()
{
	std::string fname = "loadbar";
	const auto & conf = CMainMenuConfig::get().getConfig()["loading"];

	if(conf.isStruct())
	{
		if(conf["background"].isVector())
			return RandomGeneratorUtil::nextItem(conf["background"].Vector(), CRandomGenerator::getDefault())->String();
		
		if(conf["background"].isString())
			return conf["background"].String();
		
		return fname;
	}
	
	if(conf.isVector() && !conf.Vector().empty())
		return RandomGeneratorUtil::nextItem(conf.Vector(), CRandomGenerator::getDefault())->String();
	
	return fname;
}
