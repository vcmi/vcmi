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

#include "../../lib/filesystem/Filesystem.h"
#include "../../lib/filesystem/CCompressedStream.h"

#include "../gui/SDL_Extensions.h"
#include "../gui/CCursorHandler.h"

#include "../CGameInfo.h"
#include "../../lib/CGeneralTextHandler.h"
#include "../../lib/JsonNode.h"
#include "../CMusicHandler.h"
#include "../CVideoHandler.h"
#include "../Graphics.h"
#include "../../lib/serializer/Connection.h"
#include "../../lib/serializer/CTypeList.h"
#include "../../lib/VCMIDirs.h"
#include "../../lib/mapping/CMap.h"
#include "../windows/GUIClasses.h"
#include "../CPlayerInterface.h"
#include "../../CCallback.h"
#include "../CMessage.h"
#include "../CBitmapHandler.h"
#include "../Client.h"
#include "../gui/CGuiHandler.h"
#include "../gui/CAnimation.h"
#include "../widgets/CComponent.h"
#include "../widgets/Buttons.h"
#include "../widgets/MiscWidgets.h"
#include "../widgets/ObjectLists.h"
#include "../widgets/TextControls.h"
#include "../windows/InfoWindows.h"
#include "../CServerHandler.h"
#include "../../lib/CStopWatch.h"
#include "../../lib/NetPacksLobby.h"
#include "../../lib/CThreadHelper.h"
#include "../../lib/CConfigHandler.h"
#include "../../lib/GameConstants.h"
#include "../../lib/CRandomGenerator.h"
#include "../../lib/CondSh.h"
#include "../../lib/mapping/CCampaignHandler.h"


namespace fs = boost::filesystem;

std::shared_ptr<CMainMenu> CMM;
ISelectionScreenInfo * SEL;

static void do_quit()
{
	SDL_Event event;
	event.quit.type = SDL_QUIT;
	SDL_PushEvent(&event);
}

CMenuScreen::CMenuScreen(const JsonNode & configNode)
	: CWindowObject(BORDERED), config(configNode)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;

	background = std::make_shared<CPicture>(config["background"].String());
	if(config["scalable"].Bool())
	{
		if(background->bg->format->palette)
			background->convertToScreenBPP();
		background->scaleTo(Point(screen->w, screen->h));
	}

	pos = background->center();

	for(const JsonNode & node : config["items"].Vector())
		menuNameToEntry.push_back(node["name"].String());

	for(const JsonNode & node : config["images"].Vector())
		images.push_back(CMainMenu::createPicture(node));

	//Hardcoded entry
	menuNameToEntry.push_back("credits");

	tabs = std::make_shared<CTabbedInt>(std::bind(&CMenuScreen::createTab, this, _1));
	tabs->type |= REDRAW_PARENT;
}

std::shared_ptr<CIntObject> CMenuScreen::createTab(size_t index)
{
	if(config["items"].Vector().size() == index)
		return std::make_shared<CreditsScreen>(this->pos);
	else
		return std::make_shared<CMenuEntry>(this, config["items"].Vector()[index]);
}

void CMenuScreen::show(SDL_Surface * to)
{
	if(!config["video"].isNull())
		CCS->videoh->update(config["video"]["x"].Float() + pos.x, config["video"]["y"].Float() + pos.y, to, true, false);
	CIntObject::show(to);
}

void CMenuScreen::activate()
{
	CCS->musich->playMusic("Music/MainMenu", true);
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
					return []() { GH.pushIntT<CMultiMode>(ESelectionScreen::newGame); };
				case 2:
					return std::bind(CMainMenu::openLobby, ESelectionScreen::campaignList, true, nullptr, ELoadMode::NONE);
				case 3:
					return std::bind(CInfoWindow::showInfoDialog, "Sorry, tutorial is not implemented yet\n", std::vector<std::shared_ptr<CComponent>>(), PlayerColor(1));
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
					return []() { GH.pushIntT<CMultiMode>(ESelectionScreen::loadGame); };
				case 2:
					return std::bind(CMainMenu::openLobby, ESelectionScreen::loadGame, true, nullptr, ELoadMode::CAMPAIGN);
				case 3:
					return std::bind(CInfoWindow::showInfoDialog, "Sorry, tutorial is not implemented yet\n", std::vector<std::shared_ptr<CComponent>>(), PlayerColor(1));
				}
			}
			break;
			case 4: //exit
			{
				return std::bind(CInfoWindow::showYesNoDialog, std::ref(CGI->generaltexth->allTexts[69]), std::vector<std::shared_ptr<CComponent>>(), do_quit, 0, PlayerColor(1));
			}
			break;
			case 5: //highscores
			{
				return std::bind(CInfoWindow::showInfoDialog, "Sorry, high scores menu is not implemented yet\n", std::vector<std::shared_ptr<CComponent>>(), PlayerColor(1));
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
		help = CGI->generaltexth->zelp[button["help"].Float()];

	int posx = button["x"].Float();
	if(posx < 0)
		posx = pos.w + posx;

	int posy = button["y"].Float();
	if(posy < 0)
		posy = pos.h + posy;

	return std::make_shared<CButton>(Point(posx, posy), button["name"].String(), help, command, button["hotkey"].Float());
}

CMenuEntry::CMenuEntry(CMenuScreen * parent, const JsonNode & config)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	type |= REDRAW_PARENT;
	pos = parent->pos;

	for(const JsonNode & node : config["images"].Vector())
		images.push_back(CMainMenu::createPicture(node));

	for(const JsonNode & node : config["buttons"].Vector())
	{
		buttons.push_back(createButton(parent, node));
		buttons.back()->hoverable = true;
		buttons.back()->type |= REDRAW_PARENT;
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
	pos.w = screen->w;
	pos.h = screen->h;

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

void CMainMenu::update()
{
	if(CMM != this->shared_from_this()) //don't update if you are not a main interface
		return;

	if(GH.listInt.empty())
	{
		GH.pushInt(CMM);
		GH.pushInt(menu);
		menu->switchToTab(0);
	}

	// Handles mouse and key input
	GH.updateTime();
	GH.handleEvents();

	// check for null othervice crash on finishing a campaign
	// /FIXME: find out why GH.listInt is empty to begin with
	if(GH.topInt())
		GH.topInt()->show(screen);
}

void CMainMenu::openLobby(ESelectionScreen screenType, bool host, const std::vector<std::string> * names, ELoadMode loadMode)
{
	CSH->resetStateForLobby(screenType == ESelectionScreen::newGame ? StartInfo::NEW_GAME : StartInfo::LOAD_GAME, names);
	CSH->screenType = screenType;
	CSH->loadMode = loadMode;

	GH.pushIntT<CSimpleJoinScreen>(host);
}

void CMainMenu::openCampaignLobby(const std::string & campaignFileName)
{
	auto ourCampaign = std::make_shared<CCampaignState>(CCampaignHandler::getCampaign(campaignFileName));
	openCampaignLobby(ourCampaign);
}

void CMainMenu::openCampaignLobby(std::shared_ptr<CCampaignState> campaign)
{
	CSH->resetStateForLobby(StartInfo::CAMPAIGN);
	CSH->screenType = ESelectionScreen::campaignList;
	CSH->campaignStateToSend = campaign;
	GH.pushIntT<CSimpleJoinScreen>();
}

void CMainMenu::openCampaignScreen(std::string name)
{
	if(vstd::contains(CMainMenuConfig::get().getCampaigns().Struct(), name))
	{
		GH.pushIntT<CCampaignScreen>(CMainMenuConfig::get().getCampaigns()[name]);
		return;
	}
	logGlobal->error("Unknown campaign set: %s", name);
}

std::shared_ptr<CMainMenu> CMainMenu::create()
{
	if(!CMM)
		CMM = std::shared_ptr<CMainMenu>(new CMainMenu());

	GH.terminate_cond->set(false);
	return CMM;
}

std::shared_ptr<CPicture> CMainMenu::createPicture(const JsonNode & config)
{
	return std::make_shared<CPicture>(config["name"].String(), config["x"].Float(), config["y"].Float());
}

CMultiMode::CMultiMode(ESelectionScreen ScreenType)
	: screenType(ScreenType)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL_NO_DISPOSE;
	background = std::make_shared<CPicture>("MUPOPUP.bmp");
	background->convertToScreenBPP(); //so we could draw without problems
	blitAt(CPicture("MUMAP.bmp"), 16, 77, *background);
	pos = background->center(); //center, window has size of bg graphic

	statusBar = std::make_shared<CGStatusBar>(std::make_shared<CPicture>(Rect(7, 465, 440, 18), 0)); //226, 472
	playerName = std::make_shared<CTextInput>(Rect(19, 436, 334, 16), *background);
	playerName->setText(settings["general"]["playerName"].String());
	playerName->cb += std::bind(&CMultiMode::onNameChange, this, _1);

	buttonHotseat = std::make_shared<CButton>(Point(373, 78), "MUBHOT.DEF", CGI->generaltexth->zelp[266], std::bind(&CMultiMode::hostTCP, this));
	buttonHost = std::make_shared<CButton>(Point(373, 78 + 57 * 1), "MUBHOST.DEF", CButton::tooltip("Host TCP/IP game", ""), std::bind(&CMultiMode::hostTCP, this));
	buttonJoin = std::make_shared<CButton>(Point(373, 78 + 57 * 2), "MUBJOIN.DEF", CButton::tooltip("Join TCP/IP game", ""), std::bind(&CMultiMode::joinTCP, this));
	buttonCancel = std::make_shared<CButton>(Point(373, 424), "MUBCANC.DEF", CGI->generaltexth->zelp[288], [=](){ close();}, SDLK_ESCAPE);
}

void CMultiMode::hostTCP()
{
	auto savedScreenType = screenType;
	close();
	GH.pushIntT<CMultiPlayers>(settings["general"]["playerName"].String(), savedScreenType, true, ELoadMode::MULTI);
}

void CMultiMode::joinTCP()
{
	auto savedScreenType = screenType;
	close();
	GH.pushIntT<CMultiPlayers>(settings["general"]["playerName"].String(), savedScreenType, false, ELoadMode::MULTI);
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
	textTitle = std::make_shared<CTextBox>(text, Rect(25, 20, 315, 50), 0, FONT_BIG, CENTER, Colors::WHITE); //HOTSEAT	Please enter names

	for(int i = 0; i < inputNames.size(); i++)
	{
		inputNames[i] = std::make_shared<CTextInput>(Rect(60, 85 + i * 30, 280, 16), *background);
		inputNames[i]->cb += std::bind(&CMultiPlayers::onChange, this, _1);
	}

	buttonOk = std::make_shared<CButton>(Point(95, 338), "MUBCHCK.DEF", CGI->generaltexth->zelp[560], std::bind(&CMultiPlayers::enterSelectionScreen, this), SDLK_RETURN);
	buttonCancel = std::make_shared<CButton>(Point(205, 338), "MUBCANC.DEF", CGI->generaltexth->zelp[561], [=](){ close();}, SDLK_ESCAPE);
	statusBar = std::make_shared<CGStatusBar>(std::make_shared<CPicture>(Rect(7, 381, 348, 18), 0)); //226, 472

	inputNames[0]->setText(firstPlayer, true);
	inputNames[0]->giveFocus();
}

void CMultiPlayers::onChange(std::string newText)
{
	size_t namesCount = 0;

	for(auto & elem : inputNames)
		if(!elem->text.empty())
			namesCount++;
}

void CMultiPlayers::enterSelectionScreen()
{
	std::vector<std::string> names;
	for(auto name : inputNames)
	{
		if(name->text.length())
			names.push_back(name->text);
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

	textTitle = std::make_shared<CTextBox>("", Rect(20, 20, 205, 50), 0, FONT_BIG, CENTER, Colors::WHITE);
	inputAddress = std::make_shared<CTextInput>(Rect(25, 68, 175, 16), *background.get());
	inputPort = std::make_shared<CTextInput>(Rect(25, 115, 175, 16), *background.get());
	if(host && !settings["session"]["donotstartserver"].Bool())
	{
		textTitle->setText("Connecting...");
		boost::thread(&CSimpleJoinScreen::connectThread, this, "", 0);
	}
	else
	{
		textTitle->setText("Enter address:");
		inputAddress->cb += std::bind(&CSimpleJoinScreen::onChange, this, _1);
		inputPort->cb += std::bind(&CSimpleJoinScreen::onChange, this, _1);
		inputPort->filters += std::bind(&CTextInput::numberFilter, _1, _2, 0, 65535);
		buttonOk = std::make_shared<CButton>(Point(26, 142), "MUBCHCK.DEF", CGI->generaltexth->zelp[560], std::bind(&CSimpleJoinScreen::connectToServer, this), SDLK_RETURN);

		inputAddress->giveFocus();
	}
	inputAddress->setText(settings["server"]["server"].String(), true);
	inputPort->setText(CServerHandler::getDefaultPortStr(), true);

	buttonCancel = std::make_shared<CButton>(Point(142, 142), "MUBCANC.DEF", CGI->generaltexth->zelp[561], std::bind(&CSimpleJoinScreen::leaveScreen, this), SDLK_ESCAPE);
	statusBar = std::make_shared<CGStatusBar>(std::make_shared<CPicture>(Rect(7, 186, 218, 18), 0));
}

void CSimpleJoinScreen::connectToServer()
{
	textTitle->setText("Connecting...");
	buttonOk->block(true);

	boost::thread(&CSimpleJoinScreen::connectThread, this, inputAddress->text, boost::lexical_cast<ui16>(inputPort->text));
}

void CSimpleJoinScreen::leaveScreen()
{
	if(CSH->state == EClientState::CONNECTING)
	{
		textTitle->setText("Closing...");
		CSH->state = EClientState::CONNECTION_CANCELLED;
	}
	else if(GH.listInt.size() && GH.listInt.front().get() == this)
	{
		close();
	}
}

void CSimpleJoinScreen::onChange(const std::string & newText)
{
	buttonOk->block(inputAddress->text.empty() || inputPort->text.empty());
}

void CSimpleJoinScreen::connectThread(const std::string addr, const ui16 port)
{
	setThreadName("CSimpleJoinScreen::connectThread");
	if(!addr.length())
		CSH->startLocalServerAndConnect();
	else
		CSH->justConnectToServer(addr, port);

	if(GH.listInt.size() && GH.listInt.front().get() == this)
	{
		close();
	}
}

CLoadingScreen::CLoadingScreen(std::function<void()> loader)
	: CWindowObject(BORDERED, getBackground()), loadingThread(loader)
{
	CCS->musich->stopMusic(5000);
}

CLoadingScreen::~CLoadingScreen()
{
	loadingThread.join();
}

void CLoadingScreen::showAll(SDL_Surface * to)
{
	Rect rect(0, 0, to->w, to->h);
	SDL_FillRect(to, &rect, 0);

	CWindowObject::showAll(to);
}

std::string CLoadingScreen::getBackground()
{
	const auto & conf = CMainMenuConfig::get().getConfig()["loading"].Vector();

	if(conf.empty())
	{
		return "loadbar";
	}
	else
	{
		return RandomGeneratorUtil::nextItem(conf, CRandomGenerator::getDefault())->String();
	}
}
