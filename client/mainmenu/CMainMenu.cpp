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

#include "mainmenu/CCampaignScreen.h"
#include "lobby/CBonusSelection.h"
#include "lobby/CSelectionBase.h"
#include "lobby/CLobbyScreen.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/CCompressedStream.h"

#include "../lib/CStopWatch.h"
#include "gui/SDL_Extensions.h"
#include "CGameInfo.h"
#include "gui/CCursorHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/JsonNode.h"
#include "CMusicHandler.h"
#include "CVideoHandler.h"
#include "Graphics.h"
#include "../lib/serializer/Connection.h"
#include "../lib/serializer/CTypeList.h"
#include "../lib/VCMIDirs.h"
#include "../lib/mapping/CMap.h"
#include "windows/GUIClasses.h"
#include "CPlayerInterface.h"
#include "../CCallback.h"
#include "CMessage.h"
#include "CBitmapHandler.h"
#include "Client.h"
#include "../lib/NetPacks.h"
#include "../lib/CThreadHelper.h"
#include "../lib/CConfigHandler.h"
#include "../lib/GameConstants.h"
#include "gui/CGuiHandler.h"
#include "gui/CAnimation.h"
#include "widgets/CComponent.h"
#include "widgets/Buttons.h"
#include "widgets/MiscWidgets.h"
#include "widgets/ObjectLists.h"
#include "widgets/TextControls.h"
#include "windows/InfoWindows.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/CondSh.h"
#include "CServerHandler.h"

#include "mainmenu/CreditsScreen.h"

namespace fs = boost::filesystem;

CMainMenu * CMM = nullptr;
ISelectionScreenInfo * SEL;

static void do_quit()
{
	SDL_Event event;
	event.quit.type = SDL_QUIT;
	SDL_PushEvent(&event);
}

CMenuScreen::CMenuScreen(const JsonNode & configNode)
	: config(configNode)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	background = new CPicture(config["background"].String());
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

	tabs = new CTabbedInt(std::bind(&CMenuScreen::createTab, this, _1), CTabbedInt::DestroyFunc());
	tabs->type |= REDRAW_PARENT;
}

CIntObject * CMenuScreen::createTab(size_t index)
{
	if(config["items"].Vector().size() == index)
		return new CreditsScreen();

	return new CMenuEntry(this, config["items"].Vector()[index]);
}

void CMenuScreen::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	if(pos.h != to->h || pos.w != to->w)
		CMessage::drawBorder(PlayerColor(1), to, pos.w + 28, pos.h + 30, pos.x - 14, pos.y - 15);

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
					return std::bind(&CMenuScreen::switchToTab, menu, index2);
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
					return []() { GH.pushInt(new CMultiMode(ESelectionScreen::newGame)); };
				case 2:
					return std::bind(CMainMenu::openLobby, ESelectionScreen::campaignList, true, nullptr, ELoadMode::NONE);
				case 3:
					return std::bind(CInfoWindow::showInfoDialog, "Sorry, tutorial is not implemented yet\n", (const std::vector<CComponent *> *)nullptr, false, PlayerColor(1));
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
					return []()     { GH.pushInt(new CMultiMode(ESelectionScreen::loadGame)); };
				case 2:
					return std::bind(CMainMenu::openLobby, ESelectionScreen::loadGame, true, nullptr, ELoadMode::CAMPAIGN);
				case 3:
					return std::bind(CInfoWindow::showInfoDialog, "Sorry, tutorial is not implemented yet\n", (const std::vector<CComponent *> *)nullptr, false, PlayerColor(1));
				}
			}
			break;
			case 4: //exit
			{
				return std::bind(CInfoWindow::showYesNoDialog, std::ref(CGI->generaltexth->allTexts[69]), (const std::vector<CComponent *> *)nullptr, do_quit, 0, false, PlayerColor(1));
			}
			break;
			case 5: //highscores
			{
				return std::bind(CInfoWindow::showInfoDialog, "Sorry, high scores menu is not implemented yet\n", (const std::vector<CComponent *> *)nullptr, false, PlayerColor(1));
			}
			}
		}
	}
	logGlobal->error("Failed to parse command: %s", string);
	return std::function<void()>();
}

CButton * CMenuEntry::createButton(CMenuScreen * parent, const JsonNode & button)
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

	return new CButton(Point(posx, posy), button["name"].String(), help, command, button["hotkey"].Float());
}

CMenuEntry::CMenuEntry(CMenuScreen * parent, const JsonNode & config)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
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
	CMM = this;
	menu = new CMenuScreen(CMainMenuConfig::get().getConfig()["window"]);
	loadGraphics();
}

CMainMenu::~CMainMenu()
{
	boost::unique_lock<boost::recursive_mutex> lock(*CPlayerInterface::pim);
	disposeGraphics();
	if(CMM == this)
		CMM = nullptr;

	if(GH.curInt == this)
		GH.curInt = nullptr;
}

void CMainMenu::update()
{
	if(CMM != this) //don't update if you are not a main interface
		return;

	if(GH.listInt.empty())
	{
		GH.pushInt(this);
		GH.pushInt(menu);
		menu->switchToTab(0);
	}

	CSH->processIncomingPacks();

	// Handles mouse and key input
	GH.updateTime();
	GH.handleEvents();

	// check for null othervice crash on finishing a campaign
	// /FIXME: find out why GH.listInt is empty to begin with
	if(GH.topInt() != nullptr)
		GH.topInt()->show(screen);
}

void CMainMenu::openLobby(ESelectionScreen screenType, bool host, const std::vector<std::string> * names, ELoadMode loadMode)
{
	CSH->resetStateForLobby(screenType == ESelectionScreen::newGame ? StartInfo::NEW_GAME : StartInfo::LOAD_GAME, names);
	CSH->screenType = screenType;
	CSH->loadMode = loadMode;

	GH.pushInt(new CSimpleJoinScreen(host));
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
	CSH->campaignState = campaign;
	CSH->campaignPassed = true;
	GH.pushInt(new CSimpleJoinScreen());
}

void CMainMenu::openCampaignScreen(std::string name)
{
	if(vstd::contains(CMainMenuConfig::get().getCampaigns().Struct(), name))
	{
		GH.pushInt(new CCampaignScreen(CMainMenuConfig::get().getCampaigns()[name]));
		return;
	}
	logGlobal->error("Unknown campaign set: %s", name);
}

CMainMenu * CMainMenu::create()
{
	if(!CMM)
		CMM = new CMainMenu();

	GH.terminate_cond->set(false);
	return CMM;
}

void CMainMenu::removeFromGui()
{
	//remove everything but main menu and background
	GH.popInts(GH.listInt.size() - 2);
	GH.popInt(GH.topInt()); //remove main menu
	GH.popInt(GH.topInt()); //remove background
}

void CMainMenu::showLoadingScreen(std::function<void()> loader)
{
	if(GH.listInt.size() && GH.listInt.front() == CMM)
		CMM->removeFromGui();
	GH.pushInt(new CLoadingScreen(loader));
}

void CMainMenu::loadGraphics()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CFilledTexture("DIBOXBCK", pos);

	victoryIcons = std::make_shared<CAnimation>("SCNRVICT.DEF");
	victoryIcons->load();
	lossIcons = std::make_shared<CAnimation>("SCNRLOSS.DEF");
	lossIcons->load();
}

void CMainMenu::disposeGraphics()
{
	victoryIcons->unload();
	lossIcons->unload();
}

CPicture * CMainMenu::createPicture(const JsonNode & config)
{
	return new CPicture(config["name"].String(), config["x"].Float(), config["y"].Float());
}

CMultiMode::CMultiMode(ESelectionScreen ScreenType)
	: screenType(ScreenType)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bg = new CPicture("MUPOPUP.bmp");
	bg->convertToScreenBPP(); //so we could draw without problems
	blitAt(CPicture("MUMAP.bmp"), 16, 77, *bg); //blit img
	pos = bg->center(); //center, window has size of bg graphic

	bar = new CGStatusBar(new CPicture(Rect(7, 465, 440, 18), 0)); //226, 472
	txt = new CTextInput(Rect(19, 436, 334, 16), *bg);
	txt->setText(settings["general"]["playerName"].String()); //Player
	txt->cb += std::bind(&CMultiMode::onNameChange, this, _1);

	btns[0] = new CButton(Point(373, 78), "MUBHOT.DEF", CGI->generaltexth->zelp[266], std::bind(&CMultiMode::hostTCP, this));
	btns[1] = new CButton(Point(373, 78 + 57 * 1), "MUBHOST.DEF", CButton::tooltip("Host TCP/IP game", ""), std::bind(&CMultiMode::hostTCP, this));
	btns[2] = new CButton(Point(373, 78 + 57 * 2), "MUBJOIN.DEF", CButton::tooltip("Join TCP/IP game", ""), std::bind(&CMultiMode::joinTCP, this));
	btns[6] = new CButton(Point(373, 424), "MUBCANC.DEF", CGI->generaltexth->zelp[288], [&]() { GH.popIntTotally(this);}, SDLK_ESCAPE);

}

void CMultiMode::hostTCP()
{
	GH.popIntTotally(this);
	GH.pushInt(new CMultiPlayers(settings["general"]["playerName"].String(), screenType, true, ELoadMode::MULTI));
}

void CMultiMode::joinTCP()
{
	GH.popIntTotally(this);
	GH.pushInt(new CMultiPlayers(settings["general"]["playerName"].String(), screenType, false, ELoadMode::MULTI));
}

void CMultiMode::onNameChange(std::string newText)
{
	Settings name = settings.write["general"]["playerName"];
	name->String() = newText;
}

CMultiPlayers::CMultiPlayers(const std::string & firstPlayer, ESelectionScreen ScreenType, bool Host, ELoadMode LoadMode)
	: loadMode(LoadMode), screenType(ScreenType), host(Host)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bg = new CPicture("MUHOTSEA.bmp");
	pos = bg->center(); //center, window has size of bg graphic

	std::string text = CGI->generaltexth->allTexts[446];
	boost::replace_all(text, "\t", "\n");
	Rect boxRect(25, 20, 315, 50);
	title = new CTextBox(text, boxRect, 0, FONT_BIG, CENTER, Colors::WHITE); //HOTSEAT	Please enter names

	for(int i = 0; i < ARRAY_COUNT(txt); i++)
	{
		txt[i] = new CTextInput(Rect(60, 85 + i * 30, 280, 16), *bg);
		txt[i]->cb += std::bind(&CMultiPlayers::onChange, this, _1);
	}

	ok = new CButton(Point(95, 338), "MUBCHCK.DEF", CGI->generaltexth->zelp[560], std::bind(&CMultiPlayers::enterSelectionScreen, this), SDLK_RETURN);
	cancel = new CButton(Point(205, 338), "MUBCANC.DEF", CGI->generaltexth->zelp[561], std::bind(&CGuiHandler::popIntTotally, std::ref(GH), this), SDLK_ESCAPE);
	bar = new CGStatusBar(new CPicture(Rect(7, 381, 348, 18), 0)); //226, 472

	txt[0]->setText(firstPlayer, true);
	txt[0]->giveFocus();
}

void CMultiPlayers::onChange(std::string newText)
{
	size_t namesCount = 0;

	for(auto & elem : txt)
		if(!elem->text.empty())
			namesCount++;
}

void CMultiPlayers::enterSelectionScreen()
{
	std::vector<std::string> names;
	for(int i = 0; i < ARRAY_COUNT(txt); i++)
		if(txt[i]->text.length())
			names.push_back(txt[i]->text);

	Settings name = settings.write["general"]["playerName"];
	name->String() = names[0];

	CMainMenu::openLobby(screenType, host, &names, loadMode);
}

CSimpleJoinScreen::CSimpleJoinScreen(bool host)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bg = new CPicture("MUDIALOG.bmp"); // address background
	pos = bg->center(); //center, window has size of bg graphic (x,y = 396,278 w=232 h=212)

	Rect boxRect(20, 20, 205, 50);
	title = new CTextBox("", boxRect, 0, FONT_BIG, CENTER, Colors::WHITE);
	if(host && !settings["session"]["donotstartserver"].Bool())
	{
		title->setText("Connecting...");
		boost::thread(&CSimpleJoinScreen::connectThread, this, "", 0);
	}
	else
	{
		title->setText("Enter address:");
		address = new CTextInput(Rect(25, 68, 175, 16), *bg);
		address->cb += std::bind(&CSimpleJoinScreen::onChange, this, _1);

		port = new CTextInput(Rect(25, 115, 175, 16), *bg);
		port->cb += std::bind(&CSimpleJoinScreen::onChange, this, _1);
		port->filters += std::bind(&CTextInput::numberFilter, _1, _2, 0, 65535);
		ok = new CButton(Point(26, 142), "MUBCHCK.DEF", CGI->generaltexth->zelp[560], std::bind(&CSimpleJoinScreen::connectToServer, this), SDLK_RETURN);

		port->setText(CServerHandler::getDefaultPortStr(), true);
		address->setText(settings["server"]["server"].String(), true);
		address->giveFocus();
	}

	cancel = new CButton(Point(142, 142), "MUBCANC.DEF", CGI->generaltexth->zelp[561], std::bind(&CSimpleJoinScreen::leaveScreen, this), SDLK_ESCAPE);
	bar = new CGStatusBar(new CPicture(Rect(7, 186, 218, 18), 0));
}

void CSimpleJoinScreen::connectToServer()
{
	title->setText("Connecting...");
	ok->block(true);

	boost::thread(&CSimpleJoinScreen::connectThread, this, address->text, boost::lexical_cast<ui16>(port->text));
}

void CSimpleJoinScreen::leaveScreen()
{
	if(CSH->state == EClientState::CONNECTING)
	{
		title->setText("Closing...");
		CSH->state = EClientState::CONNECTION_CANCELLED;
	}
	else if(GH.listInt.size() && GH.listInt.front() == this)
	{
		GH.popIntTotally(this);
	}
}

void CSimpleJoinScreen::onChange(const std::string & newText)
{
	ok->block(address->text.empty() || port->text.empty());
}

void CSimpleJoinScreen::connectThread(const std::string addr, const ui16 port)
{
	setThreadName("CSimpleJoinScreen::connectThread");
	if(!addr.length())
		CSH->startLocalServerAndConnect();
	else
		CSH->justConnectToServer(addr, port);

	if(GH.listInt.size() && GH.listInt.front() == this)
	{
		GH.popIntTotally(this);
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
