#include "StdInc.h"
#include "CPreGame.h"

#include "../lib/filesystem/Filesystem.h"
#include "../lib/filesystem/CFileInfo.h"
#include "../lib/filesystem/CCompressedStream.h"

#include "../lib/CStopWatch.h"
#include "gui/SDL_Extensions.h"
#include "CGameInfo.h"
#include "gui/CCursorHandler.h"
#include "CDefHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/mapping/CCampaignHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/JsonNode.h"
#include "CMusicHandler.h"
#include "CVideoHandler.h"
#include "Graphics.h"
#include "../lib/Connection.h"
#include "../lib/VCMIDirs.h"
#include "../lib/mapping/CMap.h"
#include "windows/GUIClasses.h"
#include "CPlayerInterface.h"
#include "../CCallback.h"
#include "CMessage.h"
#include "../lib/spells/CSpellHandler.h" /*for campaign bonuses*/
#include "../lib/CArtHandler.h" /*for campaign bonuses*/
#include "../lib/CBuildingHandler.h" /*for campaign bonuses*/
#include "CBitmapHandler.h"
#include "Client.h"
#include "../lib/NetPacks.h"
#include "../lib/registerTypes//RegisterTypes.h"
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
#include "../lib/mapping/CMapService.h"
#include "../lib/mapping/CMap.h"
#include "../lib/CRandomGenerator.h"
#include "../lib/CondSh.h"

/*
 * CPreGame.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
namespace fs = boost::filesystem;

void startGame(StartInfo * options, CConnection *serv = nullptr);
void endGame();

CGPreGame * CGP = nullptr;
ISelectionScreenInfo *SEL;

static PlayerColor playerColor; //if more than one player - applies to the first

/**
 * Stores the current name of the savegame.
 *
 * TODO better solution for auto-selection when saving already saved games.
 * -> CSelectionScreen should be divided into CLoadGameScreen, CSaveGameScreen,...
 * The name of the savegame can then be stored non-statically in CGameState and
 * passed separately to CSaveGameScreen.
 */
static std::string saveGameName;

struct EvilHlpStruct
{
	CConnection *serv;
	StartInfo *sInfo;

	void reset(bool strong = true)
	{
		if(strong)
		{
			vstd::clear_pointer(serv);
			vstd::clear_pointer(sInfo);
		}
		else
		{
			serv = nullptr;
			sInfo = nullptr;
		}
	}

} startingInfo;

static void do_quit()
{
	SDL_Event event;
	event.quit.type = SDL_QUIT;
	SDL_PushEvent(&event);
}

static CMapInfo *mapInfoFromGame()
{
	auto   ret = new CMapInfo();
	ret->mapHeader = std::unique_ptr<CMapHeader>(new CMapHeader(*LOCPLINT->cb->getMapHeader()));
	return ret;
}

static void setPlayersFromGame()
{
	playerColor = LOCPLINT->playerID;
}

static void swapPlayers(PlayerSettings &a, PlayerSettings &b)
{
	std::swap(a.playerID, b.playerID);
	std::swap(a.name, b.name);

	if(a.playerID == 1)
		playerColor = a.color;
	else if(b.playerID == 1)
		playerColor = b.color;
}

void setPlayer(PlayerSettings &pset, ui8 player, const std::map<ui8, std::string> &playerNames)
{
	if(vstd::contains(playerNames, player))
		pset.name = playerNames.find(player)->second;
	else
		pset.name = CGI->generaltexth->allTexts[468];//Computer

	pset.playerID = player;
	if(player == playerNames.begin()->first)
		playerColor = pset.color;
}

void updateStartInfo(std::string filename, StartInfo & sInfo, const CMapHeader * mapHeader, const std::map<ui8, std::string> &playerNames)
{
	sInfo.playerInfos.clear();
	if(!mapHeader)
	{
		return;
	}

	sInfo.mapname = filename;
	playerColor = PlayerColor::NEUTRAL;

	auto namesIt = playerNames.cbegin();

	for (int i = 0; i < mapHeader->players.size(); i++)
	{
		const PlayerInfo &pinfo = mapHeader->players[i];

		//neither computer nor human can play - no player
		if (!(pinfo.canHumanPlay || pinfo.canComputerPlay))
			continue;

		PlayerSettings &pset = sInfo.playerInfos[PlayerColor(i)];
		pset.color = PlayerColor(i);
		if(pinfo.canHumanPlay && namesIt != playerNames.cend())
		{
			setPlayer(pset, namesIt++->first, playerNames);
		}
		else
		{
			setPlayer(pset, 0, playerNames);
			if(!pinfo.canHumanPlay)
			{
				pset.compOnly = true;
			}
		}

		pset.castle = pinfo.defaultCastle();
		pset.hero = pinfo.defaultHero();

		if(pset.hero != PlayerSettings::RANDOM && pinfo.hasCustomMainHero())
		{
			pset.hero = pinfo.mainCustomHeroId;
			pset.heroName = pinfo.mainCustomHeroName;
			pset.heroPortrait = pinfo.mainCustomHeroPortrait;
		}

		pset.handicap = PlayerSettings::NO_HANDICAP;
	}
}

template <typename T> class CApplyOnPG;

class CBaseForPGApply
{
public:
	virtual void applyOnPG(CSelectionScreen *selScr, void *pack) const =0;
	virtual ~CBaseForPGApply(){};
	template<typename U> static CBaseForPGApply *getApplier(const U * t=nullptr)
	{
		return new CApplyOnPG<U>;
	}
};

template <typename T> class CApplyOnPG : public CBaseForPGApply
{
public:
	void applyOnPG(CSelectionScreen *selScr, void *pack) const override
	{
		T *ptr = static_cast<T*>(pack);
		ptr->apply(selScr);
	}
};

template <> class CApplyOnPG<CPack> : public CBaseForPGApply
{
public:
	void applyOnPG(CSelectionScreen *selScr, void *pack) const override
	{
			logGlobal->errorStream() << "Cannot apply on PG plain CPack!";
			assert(0);
	}
};

static CApplier<CBaseForPGApply> *applier = nullptr;

static CPicture* createPicture(const JsonNode& config)
{
	return new CPicture(config["name"].String(), config["x"].Float(), config["y"].Float());
}

CMenuScreen::CMenuScreen(const JsonNode& configNode):
	config(configNode)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	background = new CPicture(config["background"].String());
	if (config["scalable"].Bool())
	{
		if (background->bg->format->palette)
			background->convertToScreenBPP();
		background->scaleTo(Point(screen->w, screen->h));
	}

	pos = background->center();

	for(const JsonNode& node : config["items"].Vector())
		menuNameToEntry.push_back(node["name"].String());

	for(const JsonNode& node : config["images"].Vector())
		images.push_back(createPicture(node));

	//Hardcoded entry
	menuNameToEntry.push_back("credits");

    tabs = new CTabbedInt(std::bind(&CMenuScreen::createTab, this, _1), CTabbedInt::DestroyFunc());
	tabs->type |= REDRAW_PARENT;
}

CIntObject * CMenuScreen::createTab(size_t index)
{
	if (config["items"].Vector().size() == index)
		return new CreditsScreen();

	return new CMenuEntry(this, config["items"].Vector()[index]);
}

void CMenuScreen::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	if (pos.h != to->h || pos.w != to->w)
		CMessage::drawBorder(PlayerColor(1), to, pos.w+28, pos.h+30, pos.x-14, pos.y-15);

}

void CMenuScreen::show(SDL_Surface * to)
{
	if (!config["video"].isNull())
		CCS->videoh->update(config["video"]["x"].Float() + pos.x, config["video"]["y"].Float() + pos.y, to, true, false);
	CIntObject::show(to);
}

void CMenuScreen::activate()
{
	CCS->musich->playMusic("Music/MainMenu", true);
	if (!config["video"].isNull())
		CCS->videoh->open(config["video"]["name"].String());
	CIntObject::activate();
}

void CMenuScreen::deactivate()
{
	if (!config["video"].isNull())
		CCS->videoh->close();

	CIntObject::deactivate();
}

void CMenuScreen::switchToTab(size_t index)
{
	tabs->setActive(index);
}

//funciton for std::string -> std::function conversion for main menu
static std::function<void()> genCommand(CMenuScreen* menu, std::vector<std::string> menuType, const std::string &string)
{
	static const std::vector<std::string> commandType  =
		{"to", "campaigns", "start", "load", "exit", "highscores"};

	static const std::vector<std::string> gameType =
		{"single", "multi", "campaign", "tutorial"};

	std::list<std::string> commands;
	boost::split(commands, string, boost::is_any_of("\t "));

	if (!commands.empty())
	{
		size_t index = std::find(commandType.begin(), commandType.end(), commands.front()) - commandType.begin();
		commands.pop_front();
		if (index > 3 || !commands.empty())
		{
			switch (index)
			{
				break; case 0://to - switch to another tab, if such tab exists
				{
					size_t index2 = std::find(menuType.begin(), menuType.end(), commands.front()) - menuType.begin();
					if ( index2 != menuType.size())
                        return std::bind(&CMenuScreen::switchToTab, menu, index2);
				}
				break; case 1://open campaign selection window
				{
                    return std::bind(&CGPreGame::openCampaignScreen, CGP, commands.front());
				}
				break; case 2://start
				{
					switch (std::find(gameType.begin(), gameType.end(), commands.front()) - gameType.begin())
					{
                        case 0: return std::bind(&CGPreGame::openSel, CGP, CMenuScreen::newGame, CMenuScreen::SINGLE_PLAYER);
						case 1: return &pushIntT<CMultiMode>;
                        case 2: return std::bind(&CGPreGame::openSel, CGP, CMenuScreen::campaignList, CMenuScreen::SINGLE_PLAYER);
						//TODO: start tutorial
                        case 3: return std::bind(CInfoWindow::showInfoDialog, "Sorry, tutorial is not implemented yet\n", (const std::vector<CComponent*>*)nullptr, false, PlayerColor(1));
					}
				}
				break; case 3://load
				{
					switch (std::find(gameType.begin(), gameType.end(), commands.front()) - gameType.begin())
					{
                        case 0: return std::bind(&CGPreGame::openSel, CGP, CMenuScreen::loadGame, CMenuScreen::SINGLE_PLAYER);
                        case 1: return std::bind(&CGPreGame::openSel, CGP, CMenuScreen::loadGame, CMenuScreen::MULTI_HOT_SEAT);
						//TODO: load campaign
                        case 2: return std::bind(CInfoWindow::showInfoDialog, "This function is not implemented yet. Campaign saves can be loaded from \"Single Player\" menu", (const std::vector<CComponent*>*)nullptr, false, PlayerColor(1));
						//TODO: load tutorial
                        case 3: return std::bind(CInfoWindow::showInfoDialog, "Sorry, tutorial is not implemented yet\n", (const std::vector<CComponent*>*)nullptr, false, PlayerColor(1));
					}
				}
				break; case 4://exit
				{
                    return std::bind(CInfoWindow::showYesNoDialog, std::ref(CGI->generaltexth->allTexts[69]), (const std::vector<CComponent*>*)nullptr, do_quit, 0, false, PlayerColor(1));
				}
				break; case 5://highscores
				{
					//TODO: high scores
                    return std::bind(CInfoWindow::showInfoDialog, "Sorry, high scores menu is not implemented yet\n", (const std::vector<CComponent*>*)nullptr, false, PlayerColor(1));
				}
			}
		}
	}
    logGlobal->errorStream()<<"Failed to parse command: "<<string;
	return std::function<void()>();
}

CButton* CMenuEntry::createButton(CMenuScreen* parent, const JsonNode& button)
{
	std::function<void()> command = genCommand(parent, parent->menuNameToEntry, button["command"].String());

	std::pair<std::string, std::string> help;
	if (!button["help"].isNull() && button["help"].Float() > 0)
		help = CGI->generaltexth->zelp[button["help"].Float()];

	int posx = button["x"].Float();
	if (posx < 0)
		posx = pos.w + posx;

	int posy = button["y"].Float();
	if (posy < 0)
		posy = pos.h + posy;

	return new CButton(Point(posx, posy), button["name"].String(), help, command, button["hotkey"].Float());
}

CMenuEntry::CMenuEntry(CMenuScreen* parent, const JsonNode &config)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	type |= REDRAW_PARENT;
	pos = parent->pos;

	for(const JsonNode& node : config["images"].Vector())
		images.push_back(createPicture(node));

	for(const JsonNode& node : config["buttons"].Vector())
	{
		buttons.push_back(createButton(parent, node));
		buttons.back()->hoverable = true;
		buttons.back()->type |= REDRAW_PARENT;
	}
}

CreditsScreen::CreditsScreen():
    positionCounter(0)
{
	addUsedEvents(LCLICK | RCLICK);
	type |= REDRAW_PARENT;
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos.w = CGP->menu->pos.w;
	pos.h = CGP->menu->pos.h;
	auto textFile = CResourceHandler::get()->load(ResourceID("DATA/CREDITS.TXT"))->readAll();
	std::string text((char*)textFile.first.get(), textFile.second);
	size_t firstQuote = text.find('\"')+1;
	text = text.substr(firstQuote, text.find('\"', firstQuote) - firstQuote );
	credits = new CMultiLineLabel(Rect(pos.w - 350, 0, 350, 600), FONT_CREDITS, CENTER, Colors::WHITE, text);
	credits->scrollTextTo(-600); // move all text below the screen
}

void CreditsScreen::show(SDL_Surface * to)
{
	CIntObject::show(to);
	positionCounter++;
	if (positionCounter % 2 == 0)
		credits->scrollTextBy(1);

	//end of credits, close this screen
	if (credits->textSize.y + 600 < positionCounter / 2)
		clickRight(false, false);
}

void CreditsScreen::clickLeft(tribool down, bool previousState)
{
	clickRight(down, previousState);
}

void CreditsScreen::clickRight(tribool down, bool previousState)
{
	CTabbedInt* menu = dynamic_cast<CTabbedInt*>(parent);
	assert(menu);
	menu->setActive(0);
}

CGPreGameConfig & CGPreGameConfig::get()
{
	static CGPreGameConfig config;
	return config;
}

const JsonNode & CGPreGameConfig::getConfig() const
{
	return config;
}

const JsonNode & CGPreGameConfig::getCampaigns() const
{
	return campaignSets;
}


CGPreGameConfig::CGPreGameConfig() :
	campaignSets(JsonNode(ResourceID("config/campaignSets.json"))),
	config(JsonNode(ResourceID("config/mainmenu.json")))
{

}

CGPreGame::CGPreGame()
{
	pos.w = screen->w;
	pos.h = screen->h;

	GH.defActionsDef = 63;
	CGP = this;
	menu = new CMenuScreen(CGPreGameConfig::get().getConfig()["window"]);
	loadGraphics();
}

CGPreGame::~CGPreGame()
{
	boost::unique_lock<boost::recursive_mutex> lock(*CPlayerInterface::pim);
	disposeGraphics();
	if(CGP == this)
		CGP = nullptr;

	if(GH.curInt == this)
		GH.curInt = nullptr;
}

void CGPreGame::openSel(CMenuScreen::EState screenType, CMenuScreen::EMultiMode multi /*= CMenuScreen::SINGLE_PLAYER*/)
{
	GH.pushInt(new CSelectionScreen(screenType, multi));
}

void CGPreGame::loadGraphics()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	new CFilledTexture("DIBOXBCK", pos);

	victory = CDefHandler::giveDef("SCNRVICT.DEF");
	loss = CDefHandler::giveDef("SCNRLOSS.DEF");
}

void CGPreGame::disposeGraphics()
{
	delete victory;
	delete loss;
}

void CGPreGame::update()
{
	if(CGP != this) //don't update if you are not a main interface
		return;

	if (GH.listInt.empty())
	{
		GH.pushInt(this);
		GH.pushInt(menu);
		menu->switchToTab(0);
	}

	if(SEL)
		SEL->update();

	// Handles mouse and key input
	GH.updateTime();
	GH.handleEvents();

	// check for null othervice crash on finishing a campaign
	// /FIXME: find out why GH.listInt is empty to begin with
	if (GH.topInt() != nullptr)
		GH.topInt()->show(screen);
}

void CGPreGame::openCampaignScreen(std::string name)
{
	if (vstd::contains(CGPreGameConfig::get().getCampaigns().Struct(), name))
	{
		GH.pushInt(new CCampaignScreen(CGPreGameConfig::get().getCampaigns()[name]));
		return;
	}
	logGlobal->errorStream()<<"Unknown campaign set: "<<name;
}

CGPreGame *CGPreGame::create()
{
	if(!CGP)
		CGP = new CGPreGame();

	GH.terminate_cond.set(false);
	return CGP;
}

void CGPreGame::removeFromGui()
{
	//remove everything but main menu and background
	GH.popInts(GH.listInt.size() - 2);
	GH.popInt(GH.topInt()); //remove main menu
	GH.popInt(GH.topInt()); //remove background
}

CSelectionScreen::CSelectionScreen(CMenuScreen::EState Type, CMenuScreen::EMultiMode MultiPlayer /*= CMenuScreen::SINGLE_PLAYER*/, const std::map<ui8, std::string> * Names /*= nullptr*/, const std::string & Address /*=""*/, const std::string & Port /*= ""*/)
	: ISelectionScreenInfo(Names), serverHandlingThread(nullptr), mx(new boost::recursive_mutex),
	  serv(nullptr), ongoingClosing(false), myNameID(255)
{
	CGPreGame::create(); //we depend on its graphics
	screenType = Type;
	multiPlayer = MultiPlayer;

	OBJ_CONSTRUCTION_CAPTURING_ALL;

	bool network = (isGuest() || isHost());

	CServerHandler *sh = nullptr;
	if(isHost())
	{
		sh = new CServerHandler;
		sh->startServer();
	}

	IShowActivatable::type = BLOCK_ADV_HOTKEYS;
	pos.w = 762;
	pos.h = 584;
	if(Type == CMenuScreen::saveGame)
	{
		bordered = false;
		center(pos);
	}
	else if(Type == CMenuScreen::campaignList)
	{
		bordered = false;
		bg = new CPicture("CamCust.bmp", 0, 0);
		pos = bg->center();
	}
	else
	{
		bordered = true;
		//load random background
		const JsonVector & bgNames = CGPreGameConfig::get().getConfig()["game-select"].Vector();
		bg = new CPicture(RandomGeneratorUtil::nextItem(bgNames, CRandomGenerator::getDefault())->String(), 0, 0);
		pos = bg->center();
	}

	sInfo.difficulty = 1;
	current = nullptr;

	sInfo.mode = (Type == CMenuScreen::newGame ? StartInfo::NEW_GAME : StartInfo::LOAD_GAME);
	sInfo.turnTime = 0;
	curTab = nullptr;

	card = new InfoCard(network); //right info card
	if (screenType == CMenuScreen::campaignList)
	{
		opt = nullptr;
	}
	else
	{
		opt = new OptionsTab(); //scenario options tab
		opt->recActions = DISPOSE;

		randMapTab = new CRandomMapTab();
        randMapTab->getMapInfoChanged() += std::bind(&CSelectionScreen::changeSelection, this, _1);
		randMapTab->recActions = DISPOSE;
	}
    sel = new SelectionTab(screenType, std::bind(&CSelectionScreen::changeSelection, this, _1), multiPlayer); //scenario selection tab
	sel->recActions = DISPOSE;

	switch(screenType)
	{
	case CMenuScreen::newGame:
		{
			SDL_Color orange = {232, 184, 32, 0};
			SDL_Color overlayColor = isGuest() ? orange : Colors::WHITE;

			card->difficulty->addCallback(std::bind(&CSelectionScreen::difficultyChange, this, _1));
			card->difficulty->setSelected(1);
			CButton * select = new CButton(Point(411, 80), "GSPBUTT.DEF", CGI->generaltexth->zelp[45], 0, SDLK_s);
			select->addCallback([&]()
			{
				toggleTab(sel);
				changeSelection(sel->getSelectedMapInfo());
			});
			select->addTextOverlay(CGI->generaltexth->allTexts[500], FONT_SMALL, overlayColor);

			CButton *opts = new CButton(Point(411, 510), "GSPBUTT.DEF", CGI->generaltexth->zelp[46], std::bind(&CSelectionScreen::toggleTab, this, opt), SDLK_a);
			opts->addTextOverlay(CGI->generaltexth->allTexts[501], FONT_SMALL, overlayColor);

			CButton * randomBtn = new CButton(Point(411, 105), "GSPBUTT.DEF", CGI->generaltexth->zelp[47], 0, SDLK_r);
			randomBtn->addTextOverlay(CGI->generaltexth->allTexts[740], FONT_SMALL, overlayColor);
			randomBtn->addCallback([&]()
			{
				toggleTab(randMapTab);
				changeSelection(randMapTab->getMapInfo());
			});

			start  = new CButton(Point(411, 535), "SCNRBEG.DEF", CGI->generaltexth->zelp[103], std::bind(&CSelectionScreen::startScenario, this), SDLK_b);

			if(network)
			{
				CButton *hideChat = new CButton(Point(619, 83), "GSPBUT2.DEF", CGI->generaltexth->zelp[48], std::bind(&InfoCard::toggleChat, card), SDLK_h);
				hideChat->addTextOverlay(CGI->generaltexth->allTexts[531], FONT_SMALL);

				if(isGuest())
				{
					select->block(true);
					opts->block(true);
					randomBtn->block(true);
					start->block(true);
				}
			}
		}
		break;
	case CMenuScreen::loadGame:
		sel->recActions = 255;
		start  = new CButton(Point(411, 535), "SCNRLOD.DEF", CGI->generaltexth->zelp[103], std::bind(&CSelectionScreen::startScenario, this), SDLK_l);
		break;
	case CMenuScreen::saveGame:
		sel->recActions = 255;
		start  = new CButton(Point(411, 535), "SCNRSAV.DEF", CGI->generaltexth->zelp[103], std::bind(&CSelectionScreen::startScenario, this), SDLK_s);
		break;
	case CMenuScreen::campaignList:
		sel->recActions = 255;
		start  = new CButton(Point(411, 535), "SCNRLOD.DEF", CButton::tooltip(), std::bind(&CSelectionScreen::startCampaign, this), SDLK_b);
		break;
	}

	start->assignedKeys.insert(SDLK_RETURN);

	back = new CButton(Point(581, 535), "SCNRBACK.DEF", CGI->generaltexth->zelp[105], std::bind(&CGuiHandler::popIntTotally, &GH, this), SDLK_ESCAPE);

	if(network)
	{
		if(isHost())
		{
			assert(playerNames.size() == 1  &&  vstd::contains(playerNames, 1)); //TODO hot-seat/network combo
			serv = sh->connectToServer();
			*serv << (ui8) 4;
			myNameID = 1;
		}
		else
			serv = CServerHandler::justConnectToServer(Address, Port);

		serv->enterPregameConnectionMode();
		*serv << playerNames.begin()->second;

		if(isGuest())
		{
			const CMapInfo *map;
			*serv >> myNameID >> map;
			serv->connectionID = myNameID;
			changeSelection(map);
		}
		else if(current)
		{
			SelectMap sm(*current);
			*serv << &sm;

			UpdateStartOptions uso(sInfo);
			*serv << &uso;
		}

		applier = new CApplier<CBaseForPGApply>;
		registerTypesPregamePacks(*applier);
		serverHandlingThread = new boost::thread(&CSelectionScreen::handleConnection, this);
	}
	delete sh;
}

CSelectionScreen::~CSelectionScreen()
{
	ongoingClosing = true;
	if(serv)
	{
		assert(serverHandlingThread);
		QuitMenuWithoutStarting qmws;
		*serv << &qmws;
// 		while(!serverHandlingThread->timed_join(boost::posix_time::milliseconds(50)))
// 			processPacks();
		serverHandlingThread->join();
		delete serverHandlingThread;
	}
	playerColor = PlayerColor::CANNOT_DETERMINE;
	playerNames.clear();

	assert(!serv);
	vstd::clear_pointer(applier);
	delete mx;
}

void CSelectionScreen::toggleTab(CIntObject *tab)
{
	if(isHost() && serv)
	{
		PregameGuiAction pga;
		if(tab == curTab)
			pga.action = PregameGuiAction::NO_TAB;
		else if(tab == opt)
			pga.action = PregameGuiAction::OPEN_OPTIONS;
		else if(tab == sel)
			pga.action = PregameGuiAction::OPEN_SCENARIO_LIST;
		else if(tab == randMapTab)
			pga.action = PregameGuiAction::OPEN_RANDOM_MAP_OPTIONS;

		*serv << &pga;
	}

	if(curTab && curTab->active)
	{
		curTab->deactivate();
		curTab->recActions = DISPOSE;
	}
	if(curTab != tab)
	{
		tab->recActions = 255;
		tab->activate();
		curTab = tab;
	}
	else
	{
		curTab = nullptr;
	};
	GH.totalRedraw();
}

void CSelectionScreen::changeSelection(const CMapInfo * to)
{
	if(current == to) return;
	if(isGuest())
		vstd::clear_pointer(current);

	current = to;

	if(to && (screenType == CMenuScreen::loadGame ||
			  screenType == CMenuScreen::saveGame))
	   SEL->sInfo.difficulty = to->scenarioOpts->difficulty;
	if(screenType != CMenuScreen::campaignList)
	{
		updateStartInfo(to ? to->fileURI : "", sInfo, to ? to->mapHeader.get() : nullptr);
		if(screenType == CMenuScreen::newGame)
		{
			if(to && to->isRandomMap)
			{
				sInfo.mapGenOptions = std::shared_ptr<CMapGenOptions>(new CMapGenOptions(randMapTab->getMapGenOptions()));
			}
			else
			{
				sInfo.mapGenOptions.reset();
			}
		}
	}
	card->changeSelection(to);
	if(screenType != CMenuScreen::campaignList)
	{
		opt->recreate();
	}

	if(isHost() && serv)
	{
		SelectMap sm(*to);
		*serv << &sm;

		UpdateStartOptions uso(sInfo);
		*serv << &uso;
	}
}

void CSelectionScreen::startCampaign()
{
	if (SEL->current)
		GH.pushInt(new CBonusSelection(SEL->current->fileURI));
}

void CSelectionScreen::startScenario()
{
	if(screenType == CMenuScreen::newGame)
	{
		//there must be at least one human player before game can be started
		std::map<PlayerColor, PlayerSettings>::const_iterator i;
		for(i = SEL->sInfo.playerInfos.cbegin(); i != SEL->sInfo.playerInfos.cend(); i++)
			if(i->second.playerID != PlayerSettings::PLAYER_AI)
				break;

		if(i == SEL->sInfo.playerInfos.cend())
		{
			GH.pushInt(CInfoWindow::create(CGI->generaltexth->allTexts[530])); //You must position yourself prior to starting the game.
			return;
		}
	}

	if(isHost())
	{
		start->block(true);
		StartWithCurrentSettings swcs;
		*serv << &swcs;
		ongoingClosing = true;
		return;
	}

	if(screenType != CMenuScreen::saveGame)
	{
		if(!current)
			return;

		if(sInfo.mapGenOptions)
		{
			//copy settings from interface to actual options. TODO: refactor, it used to have no effect at all -.-
			sInfo.mapGenOptions = std::shared_ptr<CMapGenOptions>(new CMapGenOptions(randMapTab->getMapGenOptions()));

			// Update player settings for RMG
			for(const auto & psetPair : sInfo.playerInfos)
			{
				const auto & pset = psetPair.second;
				sInfo.mapGenOptions->setStartingTownForPlayer(pset.color, pset.castle);
				if(pset.playerID != PlayerSettings::PLAYER_AI)
				{
					sInfo.mapGenOptions->setPlayerTypeForStandardPlayer(pset.color, EPlayerType::HUMAN);
				}
			}

			if(!sInfo.mapGenOptions->checkOptions())
			{
				GH.pushInt(CInfoWindow::create(CGI->generaltexth->allTexts[751]));
				return;
			}
		}

		saveGameName.clear();
		if(screenType == CMenuScreen::loadGame)
		{
			saveGameName = sInfo.mapname;
		}

		auto   si = new StartInfo(sInfo);
		CGP->removeFromGui();
        CGP->showLoadingScreen(std::bind(&startGame, si, (CConnection *)nullptr));
	}
	else
	{
		if(!(sel && sel->txt && sel->txt->text.size()))
			return;

		saveGameName = "Saves/" + sel->txt->text;

		CFunctionList<void()> overWrite;
        overWrite += std::bind(&CCallback::save, LOCPLINT->cb.get(), saveGameName);
        overWrite += std::bind(&CGuiHandler::popIntTotally, &GH, this);

		if(CResourceHandler::get("local")->existsResource(ResourceID(saveGameName, EResType::CLIENT_SAVEGAME)))
		{
			std::string hlp = CGI->generaltexth->allTexts[493]; //%s exists. Overwrite?
			boost::algorithm::replace_first(hlp, "%s", sel->txt->text);
			LOCPLINT->showYesNoDialog(hlp, overWrite, 0, false);
		}
		else
			overWrite();
	}
}

void CSelectionScreen::difficultyChange( int to )
{
	assert(screenType == CMenuScreen::newGame);
	sInfo.difficulty = to;
	propagateOptions();
	redraw();
}

void CSelectionScreen::handleConnection()
{
	setThreadName("CSelectionScreen::handleConnection");
	try
	{
		assert(serv);
		while(serv)
		{
			CPackForSelectionScreen *pack = nullptr;
			*serv >> pack;
            logNetwork->traceStream() << "Received a pack of type " << typeid(*pack).name();
			assert(pack);
			if(QuitMenuWithoutStarting *endingPack = dynamic_cast<QuitMenuWithoutStarting *>(pack))
			{
				endingPack->apply(this);
			}
			else if(StartWithCurrentSettings *endingPack = dynamic_cast<StartWithCurrentSettings *>(pack))
			{
				endingPack->apply(this);
			}
			else
			{
				boost::unique_lock<boost::recursive_mutex> lll(*mx);
				upcomingPacks.push_back(pack);
			}
		}
	}
	catch(int i)
	{
		if(i != 666)
			throw;
	}
	catch(...)
	{
		handleException();
		throw;
	}
}

void CSelectionScreen::setSInfo(const StartInfo &si)
{
	std::map<PlayerColor, PlayerSettings>::const_iterator i;
	for(i = si.playerInfos.cbegin(); i != si.playerInfos.cend(); i++)
	{
		if(i->second.playerID == myNameID)
		{
			playerColor = i->first;
			break;
		}
	}

	if(i == si.playerInfos.cend()) //not found
		playerColor = PlayerColor::CANNOT_DETERMINE;

	sInfo = si;
	if(current)
		opt->recreate(); //will force to recreate using current sInfo

	card->difficulty->setSelected(si.difficulty);

	if(curTab == randMapTab)
		randMapTab->setMapGenOptions(si.mapGenOptions);

	GH.totalRedraw();
}

void CSelectionScreen::processPacks()
{
	boost::unique_lock<boost::recursive_mutex> lll(*mx);
	while(!upcomingPacks.empty())
	{
		CPackForSelectionScreen *pack = upcomingPacks.front();
		upcomingPacks.pop_front();
		CBaseForPGApply *apply = applier->apps[typeList.getTypeID(pack)]; //find the applier
		apply->applyOnPG(this, pack);
		delete pack;
	}
}

void CSelectionScreen::update()
{
	if(serverHandlingThread)
		processPacks();
}

void CSelectionScreen::propagateOptions()
{
	if(isHost() && serv)
	{
		UpdateStartOptions ups(sInfo);
		*serv << &ups;
	}
}

void CSelectionScreen::postRequest(ui8 what, ui8 dir)
{
	if(!isGuest() || !serv)
		return;

	RequestOptionsChange roc(what, dir, myNameID);
	*serv << &roc;
}

void CSelectionScreen::postChatMessage(const std::string &txt)
{
	assert(serv);
	ChatMessage cm;
	cm.message = txt;
	cm.playerName = sInfo.getPlayersSettings(myNameID)->name;
	*serv << &cm;
}

void CSelectionScreen::propagateNames()
{
	PlayersNames pn;
	pn.playerNames = playerNames;
	*serv << &pn;
}

void CSelectionScreen::showAll(SDL_Surface *to)
{
	CIntObject::showAll(to);
	if (bordered && (pos.h != to->h || pos.w != to->w))
		CMessage::drawBorder(PlayerColor(1), to, pos.w+28, pos.h+30, pos.x-14, pos.y-15);
}

// A new size filter (Small, Medium, ...) has been selected. Populate
// selMaps with the relevant data.
void SelectionTab::filter( int size, bool selectFirst )
{
	curItems.clear();

	if(tabType == CMenuScreen::campaignList)
	{
		for (auto & elem : allItems)
			curItems.push_back(&elem);
	}
	else
	{
		for (auto & elem : allItems)
			if( elem.mapHeader && elem.mapHeader->version  &&  (!size || elem.mapHeader->width == size))
				curItems.push_back(&elem);
	}

	if(curItems.size())
	{
		slider->block(false);
		slider->setAmount(curItems.size());
		sort();
		if(selectFirst)
		{
			slider->moveTo(0);
			onSelect(curItems[0]);
		}
		selectAbs(0);
	}
	else
	{
		slider->block(true);
		onSelect(nullptr);
	}
}

std::unordered_set<ResourceID> SelectionTab::getFiles(std::string dirURI, int resType)
{
	boost::to_upper(dirURI);

	std::unordered_set<ResourceID> ret = CResourceHandler::get()->getFilteredFiles([&](const ResourceID & ident)
	{
		return ident.getType() == resType
			&& boost::algorithm::starts_with(ident.getName(), dirURI);
	});

	return ret;
}

void SelectionTab::parseMaps(const std::unordered_set<ResourceID> &files)
{
	allItems.clear();
	for(auto & file : files)
	{
		try
		{
			CMapInfo mapInfo;
			mapInfo.mapInit(file.getName());

			// ignore unsupported map versions (e.g. WoG maps without WoG
			if (mapInfo.mapHeader->version <= CGI->modh->settings.data["textData"]["mapVersion"].Float())
				allItems.push_back(std::move(mapInfo));
		}
		catch(std::exception & e)
		{
            logGlobal->errorStream() << "Map " << file.getName() << " is invalid. Message: " << e.what();
		}
	}
}

void SelectionTab::parseGames(const std::unordered_set<ResourceID> &files, bool multi)
{
	for(auto & file : files)
	{
		try
		{
			CLoadFile lf(*CResourceHandler::get()->getResourceName(file), minSupportedVersion);
			lf.checkMagicBytes(SAVEGAME_MAGIC);
// 			ui8 sign[8];
// 			lf >> sign;
// 			if(std::memcmp(sign,"VCMISVG",7))
// 			{
// 				throw std::runtime_error("not a correct savefile!");
// 			}

			// Create the map info object
			CMapInfo mapInfo;
			mapInfo.mapHeader = make_unique<CMapHeader>();
			mapInfo.scenarioOpts = new StartInfo;
			lf >> *(mapInfo.mapHeader.get()) >> mapInfo.scenarioOpts;
			mapInfo.fileURI = file.getName();
			mapInfo.countPlayers();
			std::time_t time = boost::filesystem::last_write_time(*CResourceHandler::get()->getResourceName(file));
			mapInfo.date = std::asctime(std::localtime(&time));

			// If multi mode then only multi games, otherwise single
			if((mapInfo.actualHumanPlayers > 1) != multi)
			{
				mapInfo.mapHeader.reset();
			}

			allItems.push_back(std::move(mapInfo));
		}
		catch(const std::exception & e)
		{
            logGlobal->errorStream() << "Error: Failed to process " << file.getName() <<": " << e.what();
		}
	}
}

void SelectionTab::parseCampaigns(const std::unordered_set<ResourceID> &files )
{
	allItems.reserve(files.size());
	for (auto & file : files)
	{
		CMapInfo info;
		//allItems[i].date = std::asctime(std::localtime(&files[i].date));
		info.fileURI = file.getName();
		info.campaignInit();
		allItems.push_back(std::move(info));
	}
}

SelectionTab::SelectionTab(CMenuScreen::EState Type, const std::function<void(CMapInfo *)> &OnSelect, CMenuScreen::EMultiMode MultiPlayer /*= CMenuScreen::SINGLE_PLAYER*/)
	:bg(nullptr), onSelect(OnSelect)
{
	OBJ_CONSTRUCTION;
	selectionPos = 0;
	addUsedEvents(LCLICK | WHEEL | KEYBOARD | DOUBLECLICK);
	slider = nullptr;
	txt = nullptr;
	tabType = Type;

	if (Type != CMenuScreen::campaignList)
	{
		bg = new CPicture("SCSELBCK.bmp", 0, 6);
		pos = bg->pos;
	}
	else
	{
		bg = nullptr; //use background from parent
		type |= REDRAW_PARENT; // we use parent background so we need to make sure it's will be redrawn too
		pos.w = parent->pos.w;
		pos.h = parent->pos.h;
		pos.x += 3; pos.y += 6;
	}

	if(MultiPlayer == CMenuScreen::MULTI_NETWORK_GUEST)
	{
		positions = 18;
	}
	else
	{
		switch(tabType)
		{
		case CMenuScreen::newGame:
			parseMaps(getFiles("Maps/", EResType::MAP));
			positions = 18;
			break;

		case CMenuScreen::loadGame:
		case CMenuScreen::saveGame:
			parseGames(getFiles("Saves/", EResType::CLIENT_SAVEGAME), MultiPlayer);
			if(tabType == CMenuScreen::loadGame)
			{
				positions = 18;
			}
			else
			{
				positions = 16;
			}
			if(tabType == CMenuScreen::saveGame)
			{
				txt = new CTextInput(Rect(32, 539, 350, 20), Point(-32, -25), "GSSTRIP.bmp", 0);
				txt->filters += CTextInput::filenameFilter;
			}
			break;

		case CMenuScreen::campaignList:
			parseCampaigns(getFiles("Maps/", EResType::CAMPAIGN));
			positions = 18;
			break;

		default:
			assert(0);
			break;
		}
	}

	generalSortingBy = (tabType == CMenuScreen::loadGame || tabType == CMenuScreen::saveGame) ? _fileName : _name;

	if (tabType != CMenuScreen::campaignList)
	{
		//size filter buttons
		{
			int sizes[] = {36, 72, 108, 144, 0};
			const char * names[] = {"SCSMBUT.DEF", "SCMDBUT.DEF", "SCLGBUT.DEF", "SCXLBUT.DEF", "SCALBUT.DEF"};
			for(int i = 0; i < 5; i++)
				new CButton(Point(158 + 47*i, 46), names[i], CGI->generaltexth->zelp[54+i], std::bind(&SelectionTab::filter, this, sizes[i], true));
		}

		//sort buttons buttons
		{
			int xpos[] = {23, 55, 88, 121, 306, 339};
			const char * names[] = {"SCBUTT1.DEF", "SCBUTT2.DEF", "SCBUTCP.DEF", "SCBUTT3.DEF", "SCBUTT4.DEF", "SCBUTT5.DEF"};
			for(int i = 0; i < 6; i++)
			{
				ESortBy criteria = (ESortBy)i;

				if(criteria == _name)
					criteria = generalSortingBy;

				new CButton(Point(xpos[i], 86), names[i], CGI->generaltexth->zelp[107+i], std::bind(&SelectionTab::sortBy, this, criteria));
			}
		}
	}
	else
	{
		//sort by buttons
		new CButton(Point(23, 86), "CamCusM.DEF", CButton::tooltip(), std::bind(&SelectionTab::sortBy, this, _numOfMaps)); //by num of maps
		new CButton(Point(55, 86), "CamCusL.DEF", CButton::tooltip(), std::bind(&SelectionTab::sortBy, this, _name)); //by name
	}

	slider = new CSlider(Point(372, 86), tabType != CMenuScreen::saveGame ? 480 : 430, std::bind(&SelectionTab::sliderMove, this, _1), positions, curItems.size(), 0, false, CSlider::BLUE);
	slider->addUsedEvents(WHEEL);
	format =  CDefHandler::giveDef("SCSELC.DEF");

	sortingBy = _format;
	ascending = true;
	filter(0);
	//select(0);
	switch(tabType)
	{
	case CMenuScreen::newGame:
		selectFName("Maps/Arrogance");
		break;
	case CMenuScreen::loadGame:
	case CMenuScreen::campaignList:
		select(0);
		break;
	case CMenuScreen::saveGame:;
		if(saveGameName.empty())
		{
			txt->setText("NEWGAME");
		}
		else
		{
			selectFName(saveGameName);
		}
	}
}

SelectionTab::~SelectionTab()
{
	delete format;
}

void SelectionTab::sortBy( int criteria )
{
	if(criteria == sortingBy)
	{
		ascending = !ascending;
	}
	else
	{
		sortingBy = (ESortBy)criteria;
		ascending = true;
	}
	sort();

	selectAbs(0);
}

void SelectionTab::sort()
{
	if(sortingBy != generalSortingBy)
		std::stable_sort(curItems.begin(), curItems.end(), mapSorter(generalSortingBy));
	std::stable_sort(curItems.begin(), curItems.end(), mapSorter(sortingBy));

	if(!ascending)
		std::reverse(curItems.begin(), curItems.end());

	redraw();
}

void SelectionTab::select( int position )
{
	if(!curItems.size()) return;

	// New selection. py is the index in curItems.
	int py = position + slider->getValue();
	vstd::amax(py, 0);
	vstd::amin(py, curItems.size()-1);

	selectionPos = py;

	if(position < 0)
		slider->moveBy(position);
	else if(position >= positions)
		slider->moveBy(position - positions + 1);

	if(txt)
	{
		auto filename = *CResourceHandler::get("local")->getResourceName(
								   ResourceID(curItems[py]->fileURI, EResType::CLIENT_SAVEGAME));
		txt->setText(filename.stem().string());
	}

	onSelect(curItems[py]);
}

void SelectionTab::selectAbs( int position )
{
	select(position - slider->getValue());
}

int SelectionTab::getPosition( int x, int y )
{
	return -1;
}

void SelectionTab::sliderMove( int slidPos )
{
	if(!slider) return; //ignore spurious call when slider is being created
	redraw();
}


// Display the tab with the scenario names
//
// elemIdx is the index of the maps or saved game to display on line 0
// slider->capacity contains the number of available screen lines
// slider->positionsAmnt is the number of elements after filtering
void SelectionTab::printMaps(SDL_Surface *to)
{

	int elemIdx = slider->getValue();

	// Display all elements if there's enough space
	//if(slider->amount < slider->capacity)
	//	elemIdx = 0;


	SDL_Color itemColor;
	for (int line = 0; line < positions  &&  elemIdx < curItems.size(); elemIdx++, line++)
	{
		CMapInfo *currentItem = curItems[elemIdx];

		if (elemIdx == selectionPos)
			itemColor=Colors::YELLOW;
		else
			itemColor=Colors::WHITE;

		if(tabType != CMenuScreen::campaignList)
		{
			//amount of players
			std::ostringstream ostr(std::ostringstream::out);
			ostr << currentItem->playerAmnt << "/" << currentItem->humanPlayers;
			printAtLoc(ostr.str(), 29, 120 + line * 25, FONT_SMALL, itemColor, to);

			//map size
			std::string temp2 = "C";
			switch (currentItem->mapHeader->width)
			{
			case 36:
				temp2="S";
				break;
			case 72:
				temp2="M";
				break;
			case 108:
				temp2="L";
				break;
			case 144:
				temp2="XL";
				break;
			}
			printAtMiddleLoc(temp2, 70, 128 + line * 25, FONT_SMALL, itemColor, to);

			int temp=-1;
			switch (currentItem->mapHeader->version)
			{
			case EMapFormat::ROE:
				temp=0;
				break;
			case EMapFormat::AB:
				temp=1;
				break;
			case EMapFormat::SOD:
				temp=2;
				break;
			case EMapFormat::WOG:
				temp=3;
				break;
			default:
				// Unknown version. Be safe and ignore that map
                logGlobal->warnStream() << "Warning: " << currentItem->fileURI << " has wrong version!";
				continue;
			}
			blitAtLoc(format->ourImages[temp].bitmap, 88, 117 + line * 25, to);

			//victory conditions
			blitAtLoc(CGP->victory->ourImages[currentItem->mapHeader->victoryIconIndex].bitmap, 306, 117 + line * 25, to);

			//loss conditions
			blitAtLoc(CGP->loss->ourImages[currentItem->mapHeader->defeatIconIndex].bitmap, 339, 117 + line * 25, to);
		}
		else //if campaign
		{
			//number of maps in campaign
			std::ostringstream ostr(std::ostringstream::out);
			ostr << CGI->generaltexth->campaignRegionNames[ currentItem->campaignHeader->mapVersion ].size();
			printAtLoc(ostr.str(), 29, 120 + line * 25, FONT_SMALL, itemColor, to);
		}

		std::string name;
		if(tabType == CMenuScreen::newGame)
		{
			if (!currentItem->mapHeader->name.length())
				currentItem->mapHeader->name = "Unnamed";
			name = currentItem->mapHeader->name;
		}
		else if(tabType == CMenuScreen::campaignList)
		{
			name = currentItem->campaignHeader->name;
		}
		else
		{
			name = CResourceHandler::get("local")->getResourceName(
								 ResourceID(currentItem->fileURI, EResType::CLIENT_SAVEGAME))->stem().string();
		}

		//print name
		printAtMiddleLoc(name, 213, 128 + line * 25, FONT_SMALL, itemColor, to);
	}
}

void SelectionTab::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	printMaps(to);

	std::string title;
	switch(tabType) {
	case CMenuScreen::newGame:
		title = CGI->generaltexth->arraytxt[229];
		break;
	case CMenuScreen::loadGame:
		title = CGI->generaltexth->arraytxt[230];
		break;
	case CMenuScreen::saveGame:
		title = CGI->generaltexth->arraytxt[231];
		break;
	case CMenuScreen::campaignList:
		title = CGI->generaltexth->allTexts[726];
		break;
	}

	printAtMiddleLoc(title, 205, 28, FONT_MEDIUM, Colors::YELLOW, to); //Select a Scenario to Play
	if(tabType != CMenuScreen::campaignList)
	{
		printAtMiddleLoc(CGI->generaltexth->allTexts[510], 87, 62, FONT_SMALL, Colors::YELLOW, to); //Map sizes
	}
}

void SelectionTab::clickLeft( tribool down, bool previousState )
{
	if(down)
	{
		int line = getLine();
		if(line != -1)
			select(line);
	}
}
void SelectionTab::keyPressed( const SDL_KeyboardEvent & key )
{
	if(key.state != SDL_PRESSED) return;

	int moveBy = 0;
	switch(key.keysym.sym)
	{
	case SDLK_UP:
		moveBy = -1;
		break;
	case SDLK_DOWN:
		moveBy = +1;
		break;
	case SDLK_PAGEUP:
		moveBy = -positions+1;
		break;
	case SDLK_PAGEDOWN:
		moveBy = +positions-1;
		break;
	case SDLK_HOME:
		select(-slider->getValue());
		return;
	case SDLK_END:
		select(curItems.size() - slider->getValue());
		return;
	default:
		return;
	}
	select(selectionPos - slider->getValue() + moveBy);
}

void SelectionTab::onDoubleClick()
{
	if(getLine() != -1) //double clicked scenarios list
	{
		//act as if start button was pressed
		(static_cast<CSelectionScreen*>(parent))->start->clickLeft(false, true);
	}
}

int SelectionTab::getLine()
{
	int line = -1;
	Point clickPos(GH.current->button.x, GH.current->button.y);
	clickPos = clickPos - pos.topLeft();

	// Ignore clicks on save name area
	int maxPosY;
	if(tabType == CMenuScreen::saveGame)
		maxPosY = 516;
	else
		maxPosY = 564;

    	if(clickPos.y > 115  &&  clickPos.y < maxPosY  &&  clickPos.x > 22  &&  clickPos.x < 371)
	{
		line = (clickPos.y-115) / 25; //which line
	}

	return line;
}

void SelectionTab::selectFName( std::string fname )
{
	boost::to_upper(fname);
	for(int i = curItems.size() - 1; i >= 0; i--)
	{
		if(curItems[i]->fileURI == fname)
		{
			slider->moveTo(i);
			selectAbs(i);
			return;
		}
	}

	selectAbs(0);
}

const CMapInfo * SelectionTab::getSelectedMapInfo() const
{
	return curItems.empty() ? nullptr : curItems[selectionPos];
}

CRandomMapTab::CRandomMapTab()
{
	OBJ_CONSTRUCTION;
	bg = new CPicture("RANMAPBK", 0, 6);

	// Map Size
	mapSizeBtnGroup = new CToggleGroup(0);
	mapSizeBtnGroup->pos.y += 81;
	mapSizeBtnGroup->pos.x += 158;
	const std::vector<std::string> mapSizeBtns = {"RANSIZS", "RANSIZM","RANSIZL","RANSIZX"};
	addButtonsToGroup(mapSizeBtnGroup, mapSizeBtns, 0, 3, 47, 198);
	mapSizeBtnGroup->setSelected(1);
	mapSizeBtnGroup->addCallback([&](int btnId)
	{
		auto mapSizeVal = getPossibleMapSizes();
		mapGenOptions.setWidth(mapSizeVal[btnId]);
		mapGenOptions.setHeight(mapSizeVal[btnId]);
		if(!SEL->isGuest())
			updateMapInfo();
	});

	// Two levels
	twoLevelsBtn = new CToggleButton(Point(346, 81), "RANUNDR", CGI->generaltexth->zelp[202]);
	//twoLevelsBtn->select(true); for now, deactivated
	twoLevelsBtn->addCallback([&](bool on)
	{
		mapGenOptions.setHasTwoLevels(on);
		if(!SEL->isGuest())
			updateMapInfo();
	});

	// Create number defs list
	std::vector<std::string> numberDefs;
	for(int i = 0; i <= 8; ++i)
	{
		numberDefs.push_back("RANNUM" + boost::lexical_cast<std::string>(i));
	}

	const int NUMBERS_WIDTH = 32;
	const int BTNS_GROUP_LEFT_MARGIN = 67;
	// Amount of players
	playersCntGroup = new CToggleGroup(0);
	playersCntGroup->pos.y += 153;
	playersCntGroup->pos.x += BTNS_GROUP_LEFT_MARGIN;
	addButtonsWithRandToGroup(playersCntGroup, numberDefs, 1, 8, NUMBERS_WIDTH, 204, 212);
	playersCntGroup->addCallback([&](int btnId)
	{
		mapGenOptions.setPlayerCount(btnId);
		deactivateButtonsFrom(teamsCntGroup, btnId);
		deactivateButtonsFrom(compOnlyPlayersCntGroup, btnId);
		validatePlayersCnt(btnId);
		if(!SEL->isGuest())
			updateMapInfo();
	});

	// Amount of teams
	teamsCntGroup = new CToggleGroup(0);
	teamsCntGroup->pos.y += 219;
	teamsCntGroup->pos.x += BTNS_GROUP_LEFT_MARGIN;
	addButtonsWithRandToGroup(teamsCntGroup, numberDefs, 0, 7, NUMBERS_WIDTH, 214, 222);
	teamsCntGroup->addCallback([&](int btnId)
	{
		mapGenOptions.setTeamCount(btnId);
		if(!SEL->isGuest())
			updateMapInfo();
	});

	// Computer only players
	compOnlyPlayersCntGroup = new CToggleGroup(0);
	compOnlyPlayersCntGroup->pos.y += 285;
	compOnlyPlayersCntGroup->pos.x += BTNS_GROUP_LEFT_MARGIN;
	addButtonsWithRandToGroup(compOnlyPlayersCntGroup, numberDefs, 0, 7, NUMBERS_WIDTH, 224, 232);
	compOnlyPlayersCntGroup->addCallback([&](int btnId)
	{
		mapGenOptions.setCompOnlyPlayerCount(btnId);
		deactivateButtonsFrom(compOnlyTeamsCntGroup, (btnId == 0 ? 1 : btnId));
		validateCompOnlyPlayersCnt(btnId);
		if(!SEL->isGuest())
			updateMapInfo();
	});

	// Computer only teams
	compOnlyTeamsCntGroup = new CToggleGroup(0);
	compOnlyTeamsCntGroup->pos.y += 351;
	compOnlyTeamsCntGroup->pos.x += BTNS_GROUP_LEFT_MARGIN;
	addButtonsWithRandToGroup(compOnlyTeamsCntGroup, numberDefs, 0, 6, NUMBERS_WIDTH, 234, 241);
	deactivateButtonsFrom(compOnlyTeamsCntGroup, 1);
	compOnlyTeamsCntGroup->addCallback([&](int btnId)
	{
		mapGenOptions.setCompOnlyTeamCount(btnId);
		if(!SEL->isGuest())
			updateMapInfo();
	});

	const int WIDE_BTN_WIDTH = 85;
	// Water content
	waterContentGroup = new CToggleGroup(0);
	waterContentGroup->pos.y += 419;
	waterContentGroup->pos.x += BTNS_GROUP_LEFT_MARGIN;
	const std::vector<std::string> waterContentBtns = {"RANNONE","RANNORM","RANISLD"};
	addButtonsWithRandToGroup(waterContentGroup, waterContentBtns, 0, 2, WIDE_BTN_WIDTH, 243, 246);
	waterContentGroup->addCallback([&](int btnId)
	{
		mapGenOptions.setWaterContent(static_cast<EWaterContent::EWaterContent>(btnId));
	});

	// Monster strength
	monsterStrengthGroup = new CToggleGroup(0);
	monsterStrengthGroup->pos.y += 485;
	monsterStrengthGroup->pos.x += BTNS_GROUP_LEFT_MARGIN;
	const std::vector<std::string> monsterStrengthBtns = {"RANWEAK","RANNORM","RANSTRG"};
	addButtonsWithRandToGroup(monsterStrengthGroup, monsterStrengthBtns, 0, 2, WIDE_BTN_WIDTH, 248, 251);
	monsterStrengthGroup->addCallback([&](int btnId)
	{
		if (btnId < 0)
			mapGenOptions.setMonsterStrength(EMonsterStrength::RANDOM);
		else
			mapGenOptions.setMonsterStrength(static_cast<EMonsterStrength::EMonsterStrength>(btnId + EMonsterStrength::GLOBAL_WEAK)); //value 2 to 4
	});

	// Show random maps btn
	showRandMaps = new CButton(Point(54, 535), "RANSHOW", CGI->generaltexth->zelp[252]);

	// Initialize map info object
	if(!SEL->isGuest())
		updateMapInfo();
}

void CRandomMapTab::addButtonsWithRandToGroup(CToggleGroup * group, const std::vector<std::string> & defs, int nStart, int nEnd, int btnWidth, int helpStartIndex, int helpRandIndex) const
{
	addButtonsToGroup(group, defs, nStart, nEnd, btnWidth, helpStartIndex);

	// Buttons are relative to button group, TODO better solution?
	SObjectConstruction obj__i(group);
	const std::string RANDOM_DEF = "RANRAND";
	group->addToggle(CMapGenOptions::RANDOM_SIZE, new CToggleButton(Point(256, 0), RANDOM_DEF, CGI->generaltexth->zelp[helpRandIndex]));
	group->setSelected(CMapGenOptions::RANDOM_SIZE);
}

void CRandomMapTab::addButtonsToGroup(CToggleGroup * group, const std::vector<std::string> & defs, int nStart, int nEnd, int btnWidth, int helpStartIndex) const
{
	// Buttons are relative to button group, TODO better solution?
	SObjectConstruction obj__i(group);
	int cnt = nEnd - nStart + 1;
	for(int i = 0; i < cnt; ++i)
	{
		auto button = new CToggleButton(Point(i * btnWidth, 0), defs[i + nStart], CGI->generaltexth->zelp[helpStartIndex + i]);
		// For blocked state we should use pressed image actually
		button->setImageOrder(0, 1, 1, 3);

		group->addToggle(i + nStart, button);
	}
}

void CRandomMapTab::deactivateButtonsFrom(CToggleGroup * group, int startId)
{
	logGlobal->infoStream() << "Blocking buttons from " << startId;
	for(auto toggle : group->buttons)
	{
		if (auto button = dynamic_cast<CToggleButton*>(toggle.second))
		{
			if(startId == CMapGenOptions::RANDOM_SIZE || toggle.first < startId)
			{
				button->block(false);
			}
			else
			{
				button->block(true);
			}
		}
	}
}

void CRandomMapTab::validatePlayersCnt(int playersCnt)
{
	if(playersCnt == CMapGenOptions::RANDOM_SIZE)
	{
		return;
	}

	if(mapGenOptions.getTeamCount() >= playersCnt)
	{
		mapGenOptions.setTeamCount(playersCnt - 1);
		teamsCntGroup->setSelected(mapGenOptions.getTeamCount());
	}
	if(mapGenOptions.getCompOnlyPlayerCount() >= playersCnt)
	{
		mapGenOptions.setCompOnlyPlayerCount(playersCnt - 1);
		compOnlyPlayersCntGroup->setSelected(mapGenOptions.getCompOnlyPlayerCount());
	}

	validateCompOnlyPlayersCnt(mapGenOptions.getCompOnlyPlayerCount());
}

void CRandomMapTab::validateCompOnlyPlayersCnt(int compOnlyPlayersCnt)
{
	if(compOnlyPlayersCnt == CMapGenOptions::RANDOM_SIZE)
	{
		return;
	}

	if(mapGenOptions.getCompOnlyTeamCount() >= compOnlyPlayersCnt)
	{
		int compOnlyTeamCount = compOnlyPlayersCnt == 0 ? 0 : compOnlyPlayersCnt - 1;
		mapGenOptions.setCompOnlyTeamCount(compOnlyTeamCount);
		compOnlyTeamsCntGroup->setSelected(compOnlyTeamCount);
	}
}

std::vector<int> CRandomMapTab::getPossibleMapSizes()
{
	return {CMapHeader::MAP_SIZE_SMALL,CMapHeader::MAP_SIZE_MIDDLE,CMapHeader::MAP_SIZE_LARGE,CMapHeader::MAP_SIZE_XLARGE};
}

void CRandomMapTab::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	// Headline
	printAtMiddleLoc(CGI->generaltexth->allTexts[738], 222, 36, FONT_BIG, Colors::YELLOW, to);
	printAtMiddleLoc(CGI->generaltexth->allTexts[739], 222, 56, FONT_SMALL, Colors::WHITE, to);

	// Map size
	printAtMiddleLoc(CGI->generaltexth->allTexts[752], 104, 97, FONT_SMALL, Colors::WHITE, to);

	// Players cnt
	printAtLoc(CGI->generaltexth->allTexts[753], 68, 133, FONT_SMALL, Colors::WHITE, to);

	// Teams cnt
	printAtLoc(CGI->generaltexth->allTexts[754], 68, 199, FONT_SMALL, Colors::WHITE, to);

	// Computer only players cnt
	printAtLoc(CGI->generaltexth->allTexts[755], 68, 265, FONT_SMALL, Colors::WHITE, to);

	// Computer only teams cnt
	printAtLoc(CGI->generaltexth->allTexts[756], 68, 331, FONT_SMALL, Colors::WHITE, to);

	// Water content
	printAtLoc(CGI->generaltexth->allTexts[757], 68, 398, FONT_SMALL, Colors::WHITE, to);

	// Monster strength
	printAtLoc(CGI->generaltexth->allTexts[758], 68, 465, FONT_SMALL, Colors::WHITE, to);
}

void CRandomMapTab::updateMapInfo()
{
	// Generate header info
	mapInfo = make_unique<CMapInfo>();
	mapInfo->isRandomMap = true;
	mapInfo->mapHeader = make_unique<CMapHeader>();
	mapInfo->mapHeader->version = EMapFormat::SOD;
	mapInfo->mapHeader->name = CGI->generaltexth->allTexts[740];
	mapInfo->mapHeader->description = CGI->generaltexth->allTexts[741];
	mapInfo->mapHeader->difficulty = 1; // Normal
	mapInfo->mapHeader->height = mapGenOptions.getHeight();
	mapInfo->mapHeader->width = mapGenOptions.getWidth();
	mapInfo->mapHeader->twoLevel = mapGenOptions.getHasTwoLevels();

	// Generate player information
	mapInfo->mapHeader->players.clear();
	int playersToGen = PlayerColor::PLAYER_LIMIT_I;
	if(mapGenOptions.getPlayerCount() != CMapGenOptions::RANDOM_SIZE)
		playersToGen = mapGenOptions.getPlayerCount();
	mapInfo->mapHeader->howManyTeams = playersToGen;

	for(int i = 0; i < playersToGen; ++i)
	{
		PlayerInfo player;
		player.isFactionRandom = true;
		player.canComputerPlay = true;
		if(mapGenOptions.getCompOnlyPlayerCount() != CMapGenOptions::RANDOM_SIZE &&
			i >= mapGenOptions.getHumanOnlyPlayerCount())
		{
			player.canHumanPlay = false;
		}
		else
		{
			player.canHumanPlay = true;
		}
		player.team = TeamID(i);
		player.hasMainTown = true;
		player.generateHeroAtMainTown = true;
		mapInfo->mapHeader->players.push_back(player);
	}

	mapInfoChanged(mapInfo.get());
}

CFunctionList<void(const CMapInfo *)> & CRandomMapTab::getMapInfoChanged()
{
	return mapInfoChanged;
}

const CMapInfo * CRandomMapTab::getMapInfo() const
{
	return mapInfo.get();
}

const CMapGenOptions & CRandomMapTab::getMapGenOptions() const
{
	return mapGenOptions;
}

void CRandomMapTab::setMapGenOptions(std::shared_ptr<CMapGenOptions> opts)
{
	mapSizeBtnGroup->setSelected(vstd::find_pos(getPossibleMapSizes(), opts->getWidth()));
	twoLevelsBtn->setSelected(opts->getHasTwoLevels());
	playersCntGroup->setSelected(opts->getPlayerCount());
	teamsCntGroup->setSelected(opts->getTeamCount());
	compOnlyPlayersCntGroup->setSelected(opts->getCompOnlyPlayerCount());
	compOnlyTeamsCntGroup->setSelected(opts->getCompOnlyTeamCount());
	waterContentGroup->setSelected(opts->getWaterContent());
	monsterStrengthGroup->setSelected(opts->getMonsterStrength());
}

CChatBox::CChatBox(const Rect &rect)
{
	OBJ_CONSTRUCTION;
	pos += rect;
	addUsedEvents(KEYBOARD | TEXTINPUT);
	captureAllKeys = true;
	type |= REDRAW_PARENT;

	const int height = graphics->fonts[FONT_SMALL]->getLineHeight();
	inputBox = new CTextInput(Rect(0, rect.h - height, rect.w, height));
	inputBox->removeUsedEvents(KEYBOARD);
	chatHistory = new CTextBox("", Rect(0, 0, rect.w, rect.h - height), 1);

	chatHistory->label->color = Colors::GREEN;
}

void CChatBox::keyPressed(const SDL_KeyboardEvent & key)
{
	if(key.keysym.sym == SDLK_RETURN  &&  key.state == SDL_PRESSED  &&  inputBox->text.size())
	{
		SEL->postChatMessage(inputBox->text);
		inputBox->setText("");
	}
	else
		inputBox->keyPressed(key);
}

void CChatBox::addNewMessage(const std::string &text)
{
	CCS->soundh->playSound("CHAT");
	chatHistory->setText(chatHistory->label->text + text + "\n");
	if(chatHistory->slider)
		chatHistory->slider->moveToMax();
}

InfoCard::InfoCard( bool Network )
  : bg(nullptr), network(Network), chatOn(false), chat(nullptr), playerListBg(nullptr),
	difficulty(nullptr), sizes(nullptr), sFlags(nullptr)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	CIntObject::type |= REDRAW_PARENT;
	pos.x += 393;
	pos.y += 6;
	addUsedEvents(RCLICK);
	mapDescription = nullptr;

	Rect descriptionRect(26, 149, 320, 115);
	mapDescription = new CTextBox("", descriptionRect, 1);

	if(SEL->screenType == CMenuScreen::campaignList)
	{
		CSelectionScreen *ss = static_cast<CSelectionScreen*>(parent);
		mapDescription->addChild(new CPicture(*ss->bg, descriptionRect + Point(-393, 0)), true); //move subpicture bg to our description control (by default it's our (Infocard) child)
	}
	else
	{
		bg = new CPicture("GSELPOP1.bmp", 0, 0);
		parent->addChild(bg);
		auto it = vstd::find(parent->children, this); //our position among parent children
		parent->children.insert(it, bg); //put BG before us
		parent->children.pop_back();
		pos.w = bg->pos.w;
		pos.h = bg->pos.h;
		sizes = CDefHandler::giveDef("SCNRMPSZ.DEF");
		sFlags = CDefHandler::giveDef("ITGFLAGS.DEF");
		difficulty = new CToggleGroup(0);
		{
			static const char *difButns[] = {"GSPBUT3.DEF", "GSPBUT4.DEF", "GSPBUT5.DEF", "GSPBUT6.DEF", "GSPBUT7.DEF"};

			for(int i = 0; i < 5; i++)
			{
				auto button = new CToggleButton(Point(110 + i*32, 450), difButns[i], CGI->generaltexth->zelp[24+i]);

				difficulty->addToggle(i, button);
				if(SEL->screenType != CMenuScreen::newGame)
					button->block(true);
			}
		}

		if(network)
		{
			playerListBg = new CPicture("CHATPLUG.bmp", 16, 276);
			chat = new CChatBox(Rect(26, 132, 340, 132));

			chatOn = true;
			mapDescription->disable();
		}
	}

}

InfoCard::~InfoCard()
{
	delete sizes;
	delete sFlags;
}

void InfoCard::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);

	//blit texts
	if(SEL->screenType != CMenuScreen::campaignList)
	{
		printAtLoc(CGI->generaltexth->allTexts[390] + ":", 24, 400, FONT_SMALL, Colors::WHITE, to); //Allies
		printAtLoc(CGI->generaltexth->allTexts[391] + ":", 190, 400, FONT_SMALL, Colors::WHITE, to); //Enemies
		printAtLoc(CGI->generaltexth->allTexts[494], 33, 430, FONT_SMALL, Colors::YELLOW, to);//"Map Diff:"
		printAtLoc(CGI->generaltexth->allTexts[492] + ":", 133,430, FONT_SMALL, Colors::YELLOW, to); //player difficulty
		printAtLoc(CGI->generaltexth->allTexts[218] + ":", 290,430, FONT_SMALL, Colors::YELLOW, to); //"Rating:"
		printAtLoc(CGI->generaltexth->allTexts[495], 26, 22, FONT_SMALL, Colors::YELLOW, to); //Scenario Name:
		if(!chatOn)
		{
			printAtLoc(CGI->generaltexth->allTexts[496], 26, 132, FONT_SMALL, Colors::YELLOW, to); //Scenario Description:
			printAtLoc(CGI->generaltexth->allTexts[497], 26, 283, FONT_SMALL, Colors::YELLOW, to); //Victory Condition:
			printAtLoc(CGI->generaltexth->allTexts[498], 26, 339, FONT_SMALL, Colors::YELLOW, to); //Loss Condition:
		}
		else //players list
		{
			std::map<ui8, std::string> playerNames = SEL->playerNames;
			int playerSoFar = 0;
			for (auto i = SEL->sInfo.playerInfos.cbegin(); i != SEL->sInfo.playerInfos.cend(); i++)
			{
				if(i->second.playerID != PlayerSettings::PLAYER_AI)
				{
					printAtLoc(i->second.name, 24, 285 + playerSoFar++ * graphics->fonts[FONT_SMALL]->getLineHeight(), FONT_SMALL, Colors::WHITE, to);
					playerNames.erase(i->second.playerID);
				}
			}

			playerSoFar = 0;
			for (auto i = playerNames.cbegin(); i != playerNames.cend(); i++)
			{
				printAtLoc(i->second, 193, 285 + playerSoFar++ * graphics->fonts[FONT_SMALL]->getLineHeight(), FONT_SMALL, Colors::WHITE, to);
			}

		}
	}

	if(SEL->current)
	{
		if(SEL->screenType != CMenuScreen::campaignList)
		{
			int temp = -1;

			if(!chatOn)
			{
				CDefHandler * loss    = CGP ? CGP->loss    : CDefHandler::giveDef("SCNRLOSS.DEF");
				CDefHandler * victory = CGP ? CGP->victory : CDefHandler::giveDef("SCNRVICT.DEF");

				CMapHeader * header = SEL->current->mapHeader.get();
				//victory conditions
				printAtLoc(header->victoryMessage, 60, 307, FONT_SMALL, Colors::WHITE, to);
				blitAtLoc(victory->ourImages[header->victoryIconIndex].bitmap, 24, 302, to); //victory cond descr

				//loss conditoins
				printAtLoc(header->defeatMessage, 60, 366, FONT_SMALL, Colors::WHITE, to);
				blitAtLoc(loss->ourImages[header->defeatIconIndex].bitmap, 24, 359, to); //loss cond

				if (!CGP)
				{
					delete loss;
					delete victory;
				}
			}

			//difficulty
			assert(SEL->current->mapHeader->difficulty <= 4);
			std::string &diff = CGI->generaltexth->arraytxt[142 + SEL->current->mapHeader->difficulty];
			printAtMiddleLoc(diff, 62, 472, FONT_SMALL, Colors::WHITE, to);

			//selecting size icon
			switch (SEL->current->mapHeader->width)
			{
			case 36:
				temp=0;
				break;
			case 72:
				temp=1;
				break;
			case 108:
				temp=2;
				break;
			case 144:
				temp=3;
				break;
			default:
				temp=4;
				break;
			}
			blitAtLoc(sizes->ourImages[temp].bitmap, 318, 22, to);


			if(SEL->screenType == CMenuScreen::loadGame)
				printToLoc((static_cast<const CMapInfo*>(SEL->current))->date,308,34, FONT_SMALL, Colors::WHITE, to);

			//print flags
			int fx = 34  + graphics->fonts[FONT_SMALL]->getStringWidth(CGI->generaltexth->allTexts[390]);
			int ex = 200 + graphics->fonts[FONT_SMALL]->getStringWidth(CGI->generaltexth->allTexts[391]);

			TeamID myT;

			if(playerColor < PlayerColor::PLAYER_LIMIT)
				myT = SEL->current->mapHeader->players[playerColor.getNum()].team;
			else
				myT = TeamID::NO_TEAM;

			for (auto i = SEL->sInfo.playerInfos.cbegin(); i != SEL->sInfo.playerInfos.cend(); i++)
			{
				int *myx = ((i->first == playerColor  ||  SEL->current->mapHeader->players[i->first.getNum()].team == myT) ? &fx : &ex);
				blitAtLoc(sFlags->ourImages[i->first.getNum()].bitmap, *myx, 399, to);
				*myx += sFlags->ourImages[i->first.getNum()].bitmap->w;
			}

			std::string tob;
			switch (SEL->sInfo.difficulty)
			{
			case 0:
				tob="80%";
				break;
			case 1:
				tob="100%";
				break;
			case 2:
				tob="130%";
				break;
			case 3:
				tob="160%";
				break;
			case 4:
				tob="200%";
				break;
			}
			printAtMiddleLoc(tob, 311, 472, FONT_SMALL, Colors::WHITE, to);
		}

		//blit description
		std::string name;

		if (SEL->screenType == CMenuScreen::campaignList)
		{
			name = SEL->current->campaignHeader->name;
		}
		else
		{
			name = SEL->current->mapHeader->name;
		}

		//name
		if (name.length())
			printAtLoc(name, 26, 39, FONT_BIG, Colors::YELLOW, to);
		else
			printAtLoc("Unnamed", 26, 39, FONT_BIG, Colors::YELLOW, to);
	}
}

void InfoCard::changeSelection( const CMapInfo *to )
{
	if(to && mapDescription)
	{
		if (SEL->screenType == CMenuScreen::campaignList)
			mapDescription->setText(to->campaignHeader->description);
		else
			mapDescription->setText(to->mapHeader->description);

		mapDescription->label->scrollTextTo(0);
		if (mapDescription->slider)
			mapDescription->slider->moveToMin();

		if(SEL->screenType != CMenuScreen::newGame && SEL->screenType != CMenuScreen::campaignList) {
			//difficulty->block(true);
			difficulty->setSelected(SEL->sInfo.difficulty);
		}
	}
	redraw();
}

void InfoCard::clickRight( tribool down, bool previousState )
{
	static const Rect flagArea(19, 397, 335, 23);
	if(down && SEL->current && isItInLoc(flagArea, GH.current->motion.x, GH.current->motion.y))
		showTeamsPopup();
}

void InfoCard::showTeamsPopup()
{
	SDL_Surface *bmp = CMessage::drawDialogBox(256, 90 + 50 * SEL->current->mapHeader->howManyTeams);

	graphics->fonts[FONT_MEDIUM]->renderTextCenter(bmp, CGI->generaltexth->allTexts[657], Colors::YELLOW, Point(128, 30));

	for(int i = 0; i < SEL->current->mapHeader->howManyTeams; i++)
	{
		std::vector<ui8> flags;
		std::string hlp = CGI->generaltexth->allTexts[656]; //Team %d
		hlp.replace(hlp.find("%d"), 2, boost::lexical_cast<std::string>(i+1));

		graphics->fonts[FONT_SMALL]->renderTextCenter(bmp, hlp, Colors::WHITE, Point(128, 65 + 50 * i));

		for(int j = 0; j < PlayerColor::PLAYER_LIMIT_I; j++)
			if((SEL->current->mapHeader->players[j].canHumanPlay || SEL->current->mapHeader->players[j].canComputerPlay)
				&& SEL->current->mapHeader->players[j].team == TeamID(i))
				flags.push_back(j);

		int curx = 128 - 9*flags.size();
		for(auto & flag : flags)
		{
			blitAt(sFlags->ourImages[flag].bitmap, curx, 75 + 50*i, bmp);
			curx += 18;
		}
	}

	GH.pushInt(new CInfoPopup(bmp, true));
}

void InfoCard::toggleChat()
{
	setChat(!chatOn);
}

void InfoCard::setChat(bool activateChat)
{
	if(chatOn == activateChat)
		return;

	assert(active);

	if(activateChat)
	{
		mapDescription->disable();
		chat->enable();
		playerListBg->enable();
	}
	else
	{
		mapDescription->enable();
		chat->disable();
		playerListBg->disable();
	}

	chatOn = activateChat;
	GH.totalRedraw();
}

OptionsTab::OptionsTab():
	turnDuration(nullptr)
{
	OBJ_CONSTRUCTION;
	bg = new CPicture("ADVOPTBK", 0, 6);
	pos = bg->pos;

	if(SEL->screenType == CMenuScreen::newGame)
		turnDuration = new CSlider(Point(55, 551), 194, std::bind(&OptionsTab::setTurnLength, this, _1), 1, 11, 11, true, CSlider::BLUE);
}

OptionsTab::~OptionsTab()
{

}

void OptionsTab::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	printAtMiddleLoc(CGI->generaltexth->allTexts[515], 222, 30, FONT_BIG, Colors::YELLOW, to);
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[516], 222, 68, FONT_SMALL, 300, Colors::WHITE, to); //Select starting options, handicap, and name for each player in the game.
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[517], 107, 110, FONT_SMALL, 100, Colors::YELLOW, to); //Player Name Handicap Type
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[518], 197, 110, FONT_SMALL, 70, Colors::YELLOW, to); //Starting Town
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[519], 273, 110, FONT_SMALL, 70, Colors::YELLOW, to); //Starting Hero
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[520], 349, 110, FONT_SMALL, 70, Colors::YELLOW, to); //Starting Bonus
	printAtMiddleLoc(CGI->generaltexth->allTexts[521], 222, 538, FONT_SMALL, Colors::YELLOW, to); // Player Turn Duration
	if (turnDuration)
		printAtMiddleLoc(CGI->generaltexth->turnDurations[turnDuration->getValue()], 319,559, FONT_SMALL, Colors::WHITE, to);//Turn duration value
}

void OptionsTab::nextCastle( PlayerColor player, int dir )
{
	if(SEL->isGuest())
	{
		SEL->postRequest(RequestOptionsChange::TOWN, dir);
		return;
	}

	PlayerSettings &s = SEL->sInfo.playerInfos[player];
	si16 &cur = s.castle;
	auto & allowed = SEL->current->mapHeader->players[s.color.getNum()].allowedFactions;

	if (cur == PlayerSettings::NONE) //no change
		return;

	if (cur == PlayerSettings::RANDOM) //first/last available
	{
		if (dir > 0)
			cur = *allowed.begin(); //id of first town
		else
			cur = *allowed.rbegin(); //id of last town

	}
	else // next/previous available
	{
		if ( (cur == *allowed.begin() && dir < 0 )
		  || (cur == *allowed.rbegin() && dir > 0) )
			cur = -1;
		else
		{
			assert(dir >= -1 && dir <= 1); //othervice std::advance may go out of range
			auto iter = allowed.find(cur);
			std::advance(iter, dir);
			cur = *iter;
		}
	}

	if(s.hero >= 0 && !SEL->current->mapHeader->players[s.color.getNum()].hasCustomMainHero()) // remove hero unless it set to fixed one in map editor
	{
		usedHeroes.erase(s.hero); // restore previously selected hero back to available pool
		s.hero =  PlayerSettings::RANDOM;
	}
	if(cur < 0  &&  s.bonus == PlayerSettings::RESOURCE)
		s.bonus = PlayerSettings::RANDOM;

	entries[player]->selectButtons();

	SEL->propagateOptions();
	entries[player]->update();
	redraw();
}

void OptionsTab::nextHero( PlayerColor player, int dir )
{
	if(SEL->isGuest())
	{
		SEL->postRequest(RequestOptionsChange::HERO, dir);
		return;
	}

	PlayerSettings &s = SEL->sInfo.playerInfos[player];
	int old = s.hero;
	if (s.castle < 0  ||  s.playerID == PlayerSettings::PLAYER_AI  ||  s.hero == PlayerSettings::NONE)
		return;

	if (s.hero == PlayerSettings::RANDOM) // first/last available
	{
		int max = CGI->heroh->heroes.size(),
			min = 0;
		s.hero = nextAllowedHero(player, min,max,0,dir);
	}
	else
	{
		if(dir > 0)
			s.hero = nextAllowedHero(player, s.hero, CGI->heroh->heroes.size(), 1, dir);
		else
			s.hero = nextAllowedHero(player, -1, s.hero, 1, dir); // min needs to be -1 -- hero at index 0 would be skipped otherwise
	}

	if(old != s.hero)
	{
		usedHeroes.erase(old);
		usedHeroes.insert(s.hero);
		entries[player]->update();
		redraw();
	}
	SEL->propagateOptions();
}

int OptionsTab::nextAllowedHero( PlayerColor player, int min, int max, int incl, int dir )
{
	if(dir>0)
	{
		for(int i=min+incl; i<=max-incl; i++)
			if(canUseThisHero(player, i))
				return i;
	}
	else
	{
		for(int i=max-incl; i>=min+incl; i--)
			if(canUseThisHero(player, i))
				return i;
	}
	return -1;
}

bool OptionsTab::canUseThisHero( PlayerColor player, int ID )
{
	return CGI->heroh->heroes.size() > ID
	        && SEL->sInfo.playerInfos[player].castle == CGI->heroh->heroes[ID]->heroClass->faction
	        && !vstd::contains(usedHeroes, ID)
	        && SEL->current->mapHeader->allowedHeroes[ID];
}

void OptionsTab::nextBonus( PlayerColor player, int dir )
{
	if(SEL->isGuest())
	{
		SEL->postRequest(RequestOptionsChange::BONUS, dir);
		return;
	}

	PlayerSettings &s = SEL->sInfo.playerInfos[player];
	PlayerSettings::Ebonus &ret = s.bonus = static_cast<PlayerSettings::Ebonus>(static_cast<int>(s.bonus) + dir);

	if (s.hero==PlayerSettings::NONE &&
		!SEL->current->mapHeader->players[s.color.getNum()].heroesNames.size() &&
		ret==PlayerSettings::ARTIFACT) //no hero - can't be artifact
	{
		if (dir<0)
			ret=PlayerSettings::RANDOM;
		else ret=PlayerSettings::GOLD;
	}

	if(ret > PlayerSettings::RESOURCE)
		ret = PlayerSettings::RANDOM;
	if(ret < PlayerSettings::RANDOM)
		ret = PlayerSettings::RESOURCE;

	if (s.castle==PlayerSettings::RANDOM && ret==PlayerSettings::RESOURCE) //random castle - can't be resource
	{
		if (dir<0)
			ret=PlayerSettings::GOLD;
		else ret=PlayerSettings::RANDOM;
	}

	SEL->propagateOptions();
	entries[player]->update();
	redraw();
}

void OptionsTab::recreate()
{
	for(auto & elem : entries)
	{
		delete elem.second;
	}
	entries.clear();
	usedHeroes.clear();

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	for(auto it = SEL->sInfo.playerInfos.begin(); it != SEL->sInfo.playerInfos.end(); ++it)
	{
		entries.insert(std::make_pair(it->first, new PlayerOptionsEntry(this, it->second)));
		const std::vector<SHeroName> &heroes = SEL->current->mapHeader->players[it->first.getNum()].heroesNames;
		for(auto & heroe : heroes)
			usedHeroes.insert(heroe.heroId);
	}

}

void OptionsTab::setTurnLength( int npos )
{
	static const int times[] = {1, 2, 4, 6, 8, 10, 15, 20, 25, 30, 0};
	vstd::amin(npos, ARRAY_COUNT(times) - 1);

	SEL->sInfo.turnTime = times[npos];
	redraw();
}

void OptionsTab::flagPressed( PlayerColor color )
{
	PlayerSettings &clicked =  SEL->sInfo.playerInfos[color];
	PlayerSettings *old = nullptr;

	if(SEL->playerNames.size() == 1) //single player -> just swap
	{
		if(color == playerColor) //that color is already selected, no action needed
			return;

		old = &SEL->sInfo.playerInfos[playerColor];
		swapPlayers(*old, clicked);
	}
	else
	{
		//identify clicked player
		int clickedNameID = clicked.playerID; //human is a number of player, zero means AI

		if(clickedNameID > 0  &&  playerToRestore.id == clickedNameID) //player to restore is about to being replaced -> put him back to the old place
		{
			PlayerSettings &restPos = SEL->sInfo.playerInfos[playerToRestore.color];
			SEL->setPlayer(restPos, playerToRestore.id);
			playerToRestore.reset();
		}

		int newPlayer; //which player will take clicked position

		//who will be put here?
		if(!clickedNameID) //AI player clicked -> if possible replace computer with unallocated player
		{
			newPlayer = SEL->getIdOfFirstUnallocatedPlayer();
			if(!newPlayer) //no "free" player -> get just first one
				newPlayer = SEL->playerNames.begin()->first;
		}
		else //human clicked -> take next
		{
			auto i = SEL->playerNames.find(clickedNameID); //clicked one
			i++; //player AFTER clicked one

			if(i != SEL->playerNames.end())
				newPlayer = i->first;
			else
				newPlayer = 0; //AI if we scrolled through all players
		}

		SEL->setPlayer(clicked, newPlayer); //put player

		//if that player was somewhere else, we need to replace him with computer
		if(newPlayer) //not AI
		{
			for(auto i = SEL->sInfo.playerInfos.begin(); i != SEL->sInfo.playerInfos.end(); i++)
			{
				int curNameID = i->second.playerID;
				if(i->first != color  &&  curNameID == newPlayer)
				{
					assert(i->second.playerID);
					playerToRestore.color = i->first;
					playerToRestore.id = newPlayer;
					SEL->setPlayer(i->second, 0); //set computer
					old = &i->second;
					break;
				}
			}
		}
	}

	entries[clicked.color]->selectButtons();
	if(old)
	{
		entries[old->color]->selectButtons();
		if(old->hero >= 0)
			usedHeroes.erase(old->hero);

		old->hero = entries[old->color]->pi.defaultHero();
		entries[old->color]->update(); // update previous frame images in case entries were auto-updated
	}

	SEL->propagateOptions();
	GH.totalRedraw();
}

OptionsTab::PlayerOptionsEntry::PlayerOptionsEntry( OptionsTab *owner, PlayerSettings &S)
 : pi(SEL->current->mapHeader->players[S.color.getNum()]), s(S)
{
	OBJ_CONSTRUCTION;
	defActions |= SHARE_POS;

	int serial = 0;
	for(int g=0; g < s.color.getNum(); ++g)
	{
		PlayerInfo &itred = SEL->current->mapHeader->players[g];
		if( itred.canComputerPlay || itred.canHumanPlay)
			serial++;
	}

	pos.x += 54;
	pos.y += 122 + serial*50;

	static const char *flags[] = {"AOFLGBR.DEF", "AOFLGBB.DEF", "AOFLGBY.DEF", "AOFLGBG.DEF",
		"AOFLGBO.DEF", "AOFLGBP.DEF", "AOFLGBT.DEF", "AOFLGBS.DEF"};
	static const char *bgs[] = {"ADOPRPNL.bmp", "ADOPBPNL.bmp", "ADOPYPNL.bmp", "ADOPGPNL.bmp",
		"ADOPOPNL.bmp", "ADOPPPNL.bmp", "ADOPTPNL.bmp", "ADOPSPNL.bmp"};

	bg = new CPicture(BitmapHandler::loadBitmap(bgs[s.color.getNum()]), 0, 0, true);
	if(SEL->screenType == CMenuScreen::newGame)
	{
		btns[0] = new CButton(Point(107, 5), "ADOPLFA.DEF", CGI->generaltexth->zelp[132], std::bind(&OptionsTab::nextCastle, owner, s.color, -1));
		btns[1] = new CButton(Point(168, 5), "ADOPRTA.DEF", CGI->generaltexth->zelp[133], std::bind(&OptionsTab::nextCastle, owner, s.color, +1));
		btns[2] = new CButton(Point(183, 5), "ADOPLFA.DEF", CGI->generaltexth->zelp[148], std::bind(&OptionsTab::nextHero, owner, s.color, -1));
		btns[3] = new CButton(Point(244, 5), "ADOPRTA.DEF", CGI->generaltexth->zelp[149], std::bind(&OptionsTab::nextHero, owner, s.color, +1));
		btns[4] = new CButton(Point(259, 5), "ADOPLFA.DEF", CGI->generaltexth->zelp[164], std::bind(&OptionsTab::nextBonus, owner, s.color, -1));
		btns[5] = new CButton(Point(320, 5), "ADOPRTA.DEF", CGI->generaltexth->zelp[165], std::bind(&OptionsTab::nextBonus, owner, s.color, +1));
	}
	else
		for(auto & elem : btns)
			elem = nullptr;

	selectButtons();

	assert(SEL->current && SEL->current->mapHeader);
	const PlayerInfo &p = SEL->current->mapHeader->players[s.color.getNum()];
	assert(p.canComputerPlay || p.canHumanPlay); //someone must be able to control this player
	if(p.canHumanPlay && p.canComputerPlay)
		whoCanPlay = HUMAN_OR_CPU;
	else if(p.canComputerPlay)
		whoCanPlay = CPU;
	else
		whoCanPlay = HUMAN;

	if(SEL->screenType != CMenuScreen::scenarioInfo
		&&  SEL->current->mapHeader->players[s.color.getNum()].canHumanPlay)
	{
		flag = new CButton(Point(-43, 2), flags[s.color.getNum()], CGI->generaltexth->zelp[180], std::bind(&OptionsTab::flagPressed, owner, s.color));
		flag->hoverable = true;
		flag->block(SEL->isGuest());
	}
	else
		flag = nullptr;

	town = new SelectedBox(Point(119, 2), s, TOWN);
	hero = new SelectedBox(Point(195, 2), s, HERO);
	bonus = new SelectedBox(Point(271, 2), s, BONUS);
}

void OptionsTab::PlayerOptionsEntry::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	printAtMiddleLoc(s.name, 55, 10, FONT_SMALL, Colors::WHITE, to);
	printAtMiddleWBLoc(CGI->generaltexth->arraytxt[206+whoCanPlay], 28, 39, FONT_TINY, 50, Colors::WHITE, to);
}

void OptionsTab::PlayerOptionsEntry::update()
{
	town->update();
	hero->update();
	bonus->update();
}

void OptionsTab::PlayerOptionsEntry::selectButtons()
{
	if(!btns[0])
		return;

	if( (pi.defaultCastle() != -1) //fixed tow
		|| (SEL->isGuest() && s.color != playerColor)) //or not our player
	{
		btns[0]->disable();
		btns[1]->disable();
	}
	else
	{
		btns[0]->enable();
		btns[1]->enable();
	}

	if( (pi.defaultHero() != -1  ||  !s.playerID  ||  s.castle < 0) //fixed hero
		|| (SEL->isGuest() && s.color != playerColor))//or not our player
	{
		btns[2]->disable();
		btns[3]->disable();
	}
	else
	{
		btns[2]->enable();
		btns[3]->enable();
	}

	if(SEL->isGuest() && s.color != playerColor)//or not our player
	{
		btns[4]->disable();
		btns[5]->disable();
	}
	else
	{
		btns[4]->enable();
		btns[5]->enable();
	}
}

size_t OptionsTab::CPlayerSettingsHelper::getImageIndex()
{
	enum EBonusSelection //frames of bonuses file
	{
		WOOD_ORE = 0,   CRYSTAL = 1,    GEM  = 2,
		MERCURY  = 3,   SULFUR  = 5,    GOLD = 8,
		ARTIFACT = 9,   RANDOM  = 10,
		WOOD = 0,       ORE     = 0,    MITHRIL = 10, // resources unavailable in bonuses file

		TOWN_RANDOM = 38,  TOWN_NONE = 39, // Special frames in ITPA
		HERO_RANDOM = 163, HERO_NONE = 164 // Special frames in PortraitsSmall
	};

	switch(type)
	{
	case TOWN:
		switch (settings.castle)
		{
		case PlayerSettings::NONE:   return TOWN_NONE;
		case PlayerSettings::RANDOM: return TOWN_RANDOM;
		default: return CGI->townh->factions[settings.castle]->town->clientInfo.icons[true][false] + 2;
		}

	case HERO:
		switch (settings.hero)
		{
		case PlayerSettings::NONE:   return HERO_NONE;
		case PlayerSettings::RANDOM: return HERO_RANDOM;
		default:
			{
				if(settings.heroPortrait >= 0)
					return settings.heroPortrait;
				return CGI->heroh->heroes[settings.hero]->imageIndex;
			}
		}

	case BONUS:
		{
			switch(settings.bonus)
			{
			case PlayerSettings::RANDOM:   return RANDOM;
			case PlayerSettings::ARTIFACT: return ARTIFACT;
			case PlayerSettings::GOLD:     return GOLD;
			case PlayerSettings::RESOURCE:
				{
					switch(CGI->townh->factions[settings.castle]->town->primaryRes)
					{
					case Res::WOOD_AND_ORE : return WOOD_ORE;
					case Res::WOOD    : return WOOD;
					case Res::MERCURY : return MERCURY;
					case Res::ORE     : return ORE;
					case Res::SULFUR  : return SULFUR;
					case Res::CRYSTAL : return CRYSTAL;
					case Res::GEMS    : return GEM;
					case Res::GOLD    : return GOLD;
					case Res::MITHRIL : return MITHRIL;
					}
				}
			}
		}
	}
	return 0;
}

std::string OptionsTab::CPlayerSettingsHelper::getImageName()
{
	switch(type)
	{
	case OptionsTab::TOWN:  return "ITPA";
	case OptionsTab::HERO:  return "PortraitsSmall";
	case OptionsTab::BONUS: return "SCNRSTAR";
	}
	return "";
}

std::string OptionsTab::CPlayerSettingsHelper::getTitle()
{
	switch(type)
	{
	case OptionsTab::TOWN: return (settings.castle < 0) ? CGI->generaltexth->allTexts[103] : CGI->generaltexth->allTexts[80];
	case OptionsTab::HERO: return (settings.hero   < 0) ? CGI->generaltexth->allTexts[101] : CGI->generaltexth->allTexts[77];
	case OptionsTab::BONUS:
		{
			switch(settings.bonus)
			{
			case PlayerSettings::RANDOM:   return CGI->generaltexth->allTexts[86]; //{Random Bonus}
			case PlayerSettings::ARTIFACT: return CGI->generaltexth->allTexts[83]; //{Artifact Bonus}
			case PlayerSettings::GOLD:     return CGI->generaltexth->allTexts[84]; //{Gold Bonus}
			case PlayerSettings::RESOURCE: return CGI->generaltexth->allTexts[85]; //{Resource Bonus}
			}
		}
	}
	return "";
}

std::string OptionsTab::CPlayerSettingsHelper::getName()
{
	switch(type)
	{
	case TOWN:
		{
			switch (settings.castle)
			{
			case PlayerSettings::NONE   : return CGI->generaltexth->allTexts[523];
			case PlayerSettings::RANDOM : return CGI->generaltexth->allTexts[522];
			default : return CGI->townh->factions[settings.castle]->name;
			}
		}
	case HERO:
		{
			switch (settings.hero)
			{
			case PlayerSettings::NONE   : return CGI->generaltexth->allTexts[523];
			case PlayerSettings::RANDOM : return CGI->generaltexth->allTexts[522];
			default :
				{
					if (!settings.heroName.empty())
						return settings.heroName;
					return CGI->heroh->heroes[settings.hero]->name;
				}
			}
		}
	case BONUS:
		{
			switch (settings.bonus)
			{
				case PlayerSettings::RANDOM : return CGI->generaltexth->allTexts[522];
				default: return CGI->generaltexth->arraytxt[214 + settings.bonus];
			}
		}
	}
	return "";
}

std::string OptionsTab::CPlayerSettingsHelper::getSubtitle()
{
	switch(type)
	{
	case TOWN: return getName();
	case HERO:
		{
			if (settings.hero >= 0)
				return getName() + " - " + CGI->heroh->heroes[settings.hero]->heroClass->name;
			return getName();
		}

	case BONUS:
		{
			switch(settings.bonus)
			{
			case PlayerSettings::GOLD:     return CGI->generaltexth->allTexts[87]; //500-1000
			case PlayerSettings::RESOURCE:
				{
					switch(CGI->townh->factions[settings.castle]->town->primaryRes)
					{
					case Res::MERCURY: return CGI->generaltexth->allTexts[694];
					case Res::SULFUR:  return CGI->generaltexth->allTexts[695];
					case Res::CRYSTAL: return CGI->generaltexth->allTexts[692];
					case Res::GEMS:    return CGI->generaltexth->allTexts[693];
					case Res::WOOD_AND_ORE: return CGI->generaltexth->allTexts[89]; //At the start of the game, 5-10 wood and 5-10 ore are added to your Kingdom's resource pool
					}
				}
			}
		}
	}
	return "";
}

std::string OptionsTab::CPlayerSettingsHelper::getDescription()
{
	switch(type)
	{
	case TOWN: return CGI->generaltexth->allTexts[104];
	case HERO: return CGI->generaltexth->allTexts[102];
	case BONUS:
		{
			switch(settings.bonus)
			{
			case PlayerSettings::RANDOM:   return CGI->generaltexth->allTexts[94]; //Gold, wood and ore, or an artifact is randomly chosen as your starting bonus
			case PlayerSettings::ARTIFACT: return CGI->generaltexth->allTexts[90]; //An artifact is randomly chosen and equipped to your starting hero
			case PlayerSettings::GOLD:     return CGI->generaltexth->allTexts[92]; //At the start of the game, 500-1000 gold is added to your Kingdom's resource pool
			case PlayerSettings::RESOURCE:
				{
					switch(CGI->townh->factions[settings.castle]->town->primaryRes)
					{
					case Res::MERCURY: return CGI->generaltexth->allTexts[690];
					case Res::SULFUR:  return CGI->generaltexth->allTexts[691];
					case Res::CRYSTAL: return CGI->generaltexth->allTexts[688];
					case Res::GEMS:    return CGI->generaltexth->allTexts[689];
					case Res::WOOD_AND_ORE: return CGI->generaltexth->allTexts[93]; //At the start of the game, 5-10 wood and 5-10 ore are added to your Kingdom's resource pool
					}
				}
			}
		}
	}
	return "";
}

OptionsTab::CPregameTooltipBox::CPregameTooltipBox(CPlayerSettingsHelper & helper):
	CWindowObject(BORDERED | RCLICK_POPUP),
	CPlayerSettingsHelper(helper)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	int value = PlayerSettings::NONE;

	switch(CPlayerSettingsHelper::type)
	{
	break; case TOWN:
			value = settings.castle;
	break; case HERO:
			value = settings.hero;
	break; case BONUS:
			value = settings.bonus;
	}

	if (value == PlayerSettings::RANDOM)
		genBonusWindow();
	else if (CPlayerSettingsHelper::type == BONUS)
		genBonusWindow();
	else if (CPlayerSettingsHelper::type == HERO)
		genHeroWindow();
	else if (CPlayerSettingsHelper::type == TOWN)
		genTownWindow();

	center();
}

void OptionsTab::CPregameTooltipBox::genHeader()
{
	new CFilledTexture("DIBOXBCK", pos);
	updateShadow();

	new CLabel(pos.w / 2 + 8, 21, FONT_MEDIUM, CENTER, Colors::YELLOW, getTitle());

	new CLabel(pos.w / 2, 88, FONT_SMALL, CENTER, Colors::WHITE, getSubtitle());

	new CAnimImage(getImageName(), getImageIndex(), 0, pos.w / 2 - 24, 45);
}

void OptionsTab::CPregameTooltipBox::genTownWindow()
{
	pos = Rect(0, 0, 228, 290);
	genHeader();

	new CLabel(pos.w / 2 + 8, 122, FONT_MEDIUM, CENTER, Colors::YELLOW, CGI->generaltexth->allTexts[79]);

	std::vector<CComponent *> components;
	const CTown * town = CGI->townh->factions[settings.castle]->town;

	for (auto & elem : town->creatures)
	{
		if (!elem.empty())
			components.push_back(new CComponent(CComponent::creature, elem.front(), 0, CComponent::tiny));
	}

	new CComponentBox(components, Rect(10, 140, pos.w - 20, 140));
}

void OptionsTab::CPregameTooltipBox::genHeroWindow()
{
	pos = Rect(0, 0, 292, 226);
	genHeader();

	// specialty
	new CAnimImage("UN44", CGI->heroh->heroes[settings.hero]->imageIndex, 0, pos.w / 2 - 22, 134);

	new CLabel(pos.w / 2 + 4, 117, FONT_MEDIUM, CENTER,  Colors::YELLOW, CGI->generaltexth->allTexts[78]);
	new CLabel(pos.w / 2,     188, FONT_SMALL,  CENTER, Colors::WHITE, CGI->heroh->heroes[settings.hero]->specName);
}

void OptionsTab::CPregameTooltipBox::genBonusWindow()
{
	pos = Rect(0, 0, 228, 162);
	genHeader();

	new CTextBox(getDescription(), Rect(10, 100, pos.w - 20, 70), 0, FONT_SMALL, CENTER, Colors::WHITE );
}

OptionsTab::SelectedBox::SelectedBox(Point position, PlayerSettings & settings, SelType type)
	:CIntObject(RCLICK, position),
	CPlayerSettingsHelper(settings, type)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	image = new CAnimImage(getImageName(), getImageIndex());
	subtitle = new CLabel(23, 39, FONT_TINY, CENTER, Colors::WHITE, getName());

	pos = image->pos;
}

void OptionsTab::SelectedBox::update()
{
	image->setFrame(getImageIndex());
	subtitle->setText(getName());
}

void OptionsTab::SelectedBox::clickRight( tribool down, bool previousState )
{
	if (down)
	{
		// cases when we do not need to display a message
		if (settings.castle == -2 && CPlayerSettingsHelper::type == TOWN )
			return;
		if (settings.hero == -2 && !SEL->current->mapHeader->players[settings.color.getNum()].hasCustomMainHero() && CPlayerSettingsHelper::type == HERO)
			return;

		GH.pushInt(new CPregameTooltipBox(*this));
	}
}

CScenarioInfo::CScenarioInfo(const CMapHeader *mapHeader, const StartInfo *startInfo)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	for(auto it = startInfo->playerInfos.cbegin(); it != startInfo->playerInfos.cend(); ++it)
	{
		if(it->second.playerID)
		{
			playerColor = it->first;
		}
	}

	pos.w = 762;
	pos.h = 584;
	center(pos);

	assert(LOCPLINT);
	sInfo = *LOCPLINT->cb->getStartInfo();
	assert(!SEL->current);
	current = mapInfoFromGame();
	setPlayersFromGame();

	screenType = CMenuScreen::scenarioInfo;

	card = new InfoCard();
	opt = new OptionsTab();
	opt->recreate();
	card->changeSelection(current);

	card->difficulty->setSelected(startInfo->difficulty);
	back = new CButton(Point(584, 535), "SCNRBACK.DEF", CGI->generaltexth->zelp[105], std::bind(&CGuiHandler::popIntTotally, &GH, this), SDLK_ESCAPE);
}

CScenarioInfo::~CScenarioInfo()
{
	delete current;
}

bool mapSorter::operator()(const CMapInfo *aaa, const CMapInfo *bbb)
{
	const CMapHeader * a = aaa->mapHeader.get(),
			* b = bbb->mapHeader.get();
	if(a && b) //if we are sorting scenarios
	{
		switch (sortBy)
		{
		case _format: //by map format (RoE, WoG, etc)
			return (a->version<b->version);
			break;
		case _loscon: //by loss conditions
			return (a->defeatMessage < b->defeatMessage);
			break;
		case _playerAm: //by player amount
			int playerAmntB,humenPlayersB,playerAmntA,humenPlayersA;
			playerAmntB=humenPlayersB=playerAmntA=humenPlayersA=0;
			for (int i=0;i<8;i++)
			{
				if (a->players[i].canHumanPlay) {playerAmntA++;humenPlayersA++;}
				else if (a->players[i].canComputerPlay) {playerAmntA++;}
				if (b->players[i].canHumanPlay) {playerAmntB++;humenPlayersB++;}
				else if (b->players[i].canComputerPlay) {playerAmntB++;}
			}
			if (playerAmntB!=playerAmntA)
				return (playerAmntA<playerAmntB);
			else
				return (humenPlayersA<humenPlayersB);
			break;
		case _size: //by size of map
			return (a->width<b->width);
			break;
		case _viccon: //by victory conditions
			return (a->victoryMessage < b->victoryMessage);
			break;
		case _name: //by name
			return boost::ilexicographical_compare(a->name, b->name);
		case _fileName: //by filename
			return boost::ilexicographical_compare(aaa->fileURI, bbb->fileURI);
		default:
			return boost::ilexicographical_compare(a->name, b->name);
		}
	}
	else //if we are sorting campaigns
	{
		switch(sortBy)
		{
			case _numOfMaps: //by number of maps in campaign
				return CGI->generaltexth->campaignRegionNames[ aaa->campaignHeader->mapVersion ].size() <
					CGI->generaltexth->campaignRegionNames[ bbb->campaignHeader->mapVersion ].size();
				break;
			case _name: //by name
				return boost::ilexicographical_compare(aaa->campaignHeader->name, bbb->campaignHeader->name);
			default:
				return boost::ilexicographical_compare(aaa->campaignHeader->name, bbb->campaignHeader->name);
		}
	}
}

CMultiMode::CMultiMode()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bg = new CPicture("MUPOPUP.bmp");
	bg->convertToScreenBPP(); //so we could draw without problems
	blitAt(CPicture("MUMAP.bmp"), 16, 77, *bg); //blit img
	pos = bg->center(); //center, window has size of bg graphic

	bar = new CGStatusBar(new CPicture(Rect(7, 465, 440, 18), 0));//226, 472
	txt = new CTextInput(Rect(19, 436, 334, 16), *bg);
	txt->setText(settings["general"]["playerName"].String()); //Player

	btns[0] = new CButton(Point(373, 78),        "MUBHOT.DEF",  CGI->generaltexth->zelp[266], std::bind(&CMultiMode::openHotseat, this));
	btns[1] = new CButton(Point(373, 78 + 57*1), "MUBHOST.DEF", CButton::tooltip("Host TCP/IP game", ""), std::bind(&CMultiMode::hostTCP, this));
	btns[2] = new CButton(Point(373, 78 + 57*2), "MUBJOIN.DEF", CButton::tooltip("Join TCP/IP game", ""), std::bind(&CMultiMode::joinTCP, this));
	btns[6] = new CButton(Point(373, 424),       "MUBCANC.DEF", CGI->generaltexth->zelp[288], [&] { GH.popIntTotally(this);}, SDLK_ESCAPE);
}

void CMultiMode::openHotseat()
{
	GH.pushInt(new CHotSeatPlayers(txt->text));
}

void CMultiMode::hostTCP()
{
	Settings name = settings.write["general"]["playerName"];
	name->String() = txt->text;
	GH.popIntTotally(this);
	GH.pushInt(new CSelectionScreen(CMenuScreen::newGame, CMenuScreen::MULTI_NETWORK_HOST));
}

void CMultiMode::joinTCP()
{
	Settings name = settings.write["general"]["playerName"];
	name->String() = txt->text;
	GH.pushInt(new CSimpleJoinScreen);
}

CHotSeatPlayers::CHotSeatPlayers(const std::string &firstPlayer)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bg = new CPicture("MUHOTSEA.bmp");
	pos = bg->center(); //center, window has size of bg graphic

	std::string text = CGI->generaltexth->allTexts[446];
	boost::replace_all(text, "\t","\n");
	Rect boxRect(25, 20, 315, 50);
	title = new CTextBox(text, boxRect, 0, FONT_BIG, CENTER, Colors::WHITE);//HOTSEAT	Please enter names

	for(int i = 0; i < ARRAY_COUNT(txt); i++)
	{
		txt[i] = new CTextInput(Rect(60, 85 + i*30, 280, 16), *bg);
        txt[i]->cb += std::bind(&CHotSeatPlayers::onChange, this, _1);
	}

	ok = new CButton(Point(95, 338), "MUBCHCK.DEF", CGI->generaltexth->zelp[560], std::bind(&CHotSeatPlayers::enterSelectionScreen, this), SDLK_RETURN);
	cancel = new CButton(Point(205, 338), "MUBCANC.DEF", CGI->generaltexth->zelp[561], std::bind(&CGuiHandler::popIntTotally, std::ref(GH), this), SDLK_ESCAPE);
	bar = new CGStatusBar(new CPicture(Rect(7, 381, 348, 18), 0));//226, 472

	txt[0]->setText(firstPlayer, true);
	txt[0]->giveFocus();
}

void CHotSeatPlayers::onChange(std::string newText)
{
	size_t namesCount = 0;

	for(auto & elem : txt)
		if(!elem->text.empty())
			namesCount++;

	ok->block(namesCount < 2);
}

void CHotSeatPlayers::enterSelectionScreen()
{
	std::map<ui8, std::string> names;
	for(int i = 0, j = 1; i < ARRAY_COUNT(txt); i++)
		if(txt[i]->text.length())
			names[j++] = txt[i]->text;

	Settings name = settings.write["general"]["playerName"];
	name->String() = names.begin()->second;

	GH.popInts(2); //pop MP mode window and this
	GH.pushInt(new CSelectionScreen(CMenuScreen::newGame, CMenuScreen::MULTI_HOT_SEAT, &names));
}

void CBonusSelection::init()
{
	highlightedRegion = nullptr;
	ourHeader = nullptr;
	diffLb = nullptr;
	diffRb = nullptr;
	bonuses = nullptr;
	selectedMap = 0;

	// Initialize start info
	startInfo.mapname = ourCampaign->camp->header.filename;
	startInfo.mode = StartInfo::CAMPAIGN;
	startInfo.campState = ourCampaign;
	startInfo.turnTime = 0;

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	static const std::string bgNames [] = {"E1_BG.BMP", "G2_BG.BMP", "E2_BG.BMP", "G1_BG.BMP", "G3_BG.BMP", "N1_BG.BMP",
		"S1_BG.BMP", "BR_BG.BMP", "IS_BG.BMP", "KR_BG.BMP", "NI_BG.BMP", "TA_BG.BMP", "AR_BG.BMP", "HS_BG.BMP",
		"BB_BG.BMP", "NB_BG.BMP", "EL_BG.BMP", "RN_BG.BMP", "UA_BG.BMP", "SP_BG.BMP"};

	loadPositionsOfGraphics();

	background = BitmapHandler::loadBitmap(bgNames[ourCampaign->camp->header.mapVersion]);
	pos.h = background->h;
	pos.w = background->w;
	center();

	SDL_Surface * panel = BitmapHandler::loadBitmap("CAMPBRF.BMP");

	blitAt(panel, 456, 6, background);

	startB   = new CButton(Point(475, 536), "CBBEGIB.DEF", CButton::tooltip(), std::bind(&CBonusSelection::startMap, this), SDLK_RETURN);
	restartB = new CButton(Point(475, 536), "CBRESTB.DEF", CButton::tooltip(), std::bind(&CBonusSelection::restartMap, this), SDLK_RETURN);
	backB    = new CButton(Point(624, 536), "CBCANCB.DEF", CButton::tooltip(), std::bind(&CBonusSelection::goBack, this), SDLK_ESCAPE);

	//campaign name
	if (ourCampaign->camp->header.name.length())
		graphics->fonts[FONT_BIG]->renderTextLeft(background, ourCampaign->camp->header.name, Colors::YELLOW, Point(481, 28));
	else
		graphics->fonts[FONT_BIG]->renderTextLeft(background, CGI->generaltexth->allTexts[508], Colors::YELLOW, Point(481, 28));

	//map size icon
	sizes = CDefHandler::giveDef("SCNRMPSZ.DEF");

	//campaign description
	graphics->fonts[FONT_SMALL]->renderTextLeft(background, CGI->generaltexth->allTexts[38], Colors::YELLOW, Point(481, 63));

	campaignDescription = new CTextBox(ourCampaign->camp->header.description, Rect(480, 86, 286, 117), 1);
	//campaignDescription->showAll(background);

	//map description
	mapDescription = new CTextBox("", Rect(480, 280, 286, 117), 1);

	//bonus choosing
	graphics->fonts[FONT_MEDIUM]->renderTextLeft(background, CGI->generaltexth->allTexts[71], Colors::WHITE, Point(511, 432));
	bonuses = new CToggleGroup(std::bind(&CBonusSelection::selectBonus, this, _1));

	//set left part of window
	bool isCurrentMapConquerable = ourCampaign->currentMap && ourCampaign->camp->conquerable(*ourCampaign->currentMap);
	for(int g = 0; g < ourCampaign->camp->scenarios.size(); ++g)
	{
		if(ourCampaign->camp->conquerable(g))
		{
			regions.push_back(new CRegion(this, true, true, g));
			regions[regions.size()-1]->rclickText = ourCampaign->camp->scenarios[g].regionText;
			if(highlightedRegion == nullptr)
			{
				if(!isCurrentMapConquerable || (isCurrentMapConquerable && g == *ourCampaign->currentMap))
				{
					highlightedRegion = regions.back();
					selectMap(g, true);
				}
			}
		}
		else if (ourCampaign->camp->scenarios[g].conquered) //display as striped
		{
			regions.push_back(new CRegion(this, false, false, g));
			regions[regions.size()-1]->rclickText = ourCampaign->camp->scenarios[g].regionText;
		}
	}

	//allies / enemies
	graphics->fonts[FONT_SMALL]->renderTextLeft(background, CGI->generaltexth->allTexts[390] + ":", Colors::WHITE, Point(486, 407));
	graphics->fonts[FONT_SMALL]->renderTextLeft(background, CGI->generaltexth->allTexts[391] + ":", Colors::WHITE, Point(619, 407));

	SDL_FreeSurface(panel);

	//difficulty
	std::vector<std::string> difficulty;
	boost::split(difficulty, CGI->generaltexth->allTexts[492], boost::is_any_of(" "));
	graphics->fonts[FONT_MEDIUM]->renderTextLeft(background, difficulty.back(), Colors::WHITE, Point(689, 432));

	//difficulty pics
	for (int b=0; b<ARRAY_COUNT(diffPics); ++b)
	{
		CDefEssential * cde = CDefHandler::giveDefEss("GSPBUT" + boost::lexical_cast<std::string>(b+3) + ".DEF");
		SDL_Surface * surfToDuplicate = cde->ourImages[0].bitmap;
		diffPics[b] = SDL_ConvertSurface(surfToDuplicate, surfToDuplicate->format,
			surfToDuplicate->flags);

		delete cde;
	}

	//difficulty selection buttons
	if (ourCampaign->camp->header.difficultyChoosenByPlayer)
	{
		diffLb = new CButton(Point(694, 508), "SCNRBLF.DEF", CButton::tooltip(), std::bind(&CBonusSelection::decreaseDifficulty, this));
		diffRb = new CButton(Point(738, 508), "SCNRBRT.DEF", CButton::tooltip(), std::bind(&CBonusSelection::increaseDifficulty, this));
	}

	//load miniflags
	sFlags = CDefHandler::giveDef("ITGFLAGS.DEF");
}

CBonusSelection::CBonusSelection(std::shared_ptr<CCampaignState> _ourCampaign) : ourCampaign(_ourCampaign)
{
	init();
}

CBonusSelection::CBonusSelection(const std::string & campaignFName)
{
	ourCampaign = std::make_shared<CCampaignState>(CCampaignHandler::getCampaign(campaignFName));
	init();
}

CBonusSelection::~CBonusSelection()
{
	SDL_FreeSurface(background);
	delete sizes;
	delete ourHeader;
	delete sFlags;
	for (auto & elem : diffPics)
	{
		SDL_FreeSurface(elem);
	}
}

void CBonusSelection::goBack()
{
	GH.popIntTotally(this);
}

void CBonusSelection::showAll(SDL_Surface * to)
{
	blitAt(background, pos.x, pos.y, to);
	CIntObject::showAll(to);

	show(to);
	if (pos.h != to->h || pos.w != to->w)
		CMessage::drawBorder(PlayerColor(1), to, pos.w+28, pos.h+30, pos.x-14, pos.y-15);
}

void CBonusSelection::loadPositionsOfGraphics()
{
	const JsonNode config(ResourceID("config/campaign_regions.json"));
	int idx = 0;

	for(const JsonNode &campaign : config["campaign_regions"].Vector())
	{
		SCampPositions sc;

		sc.campPrefix = campaign["prefix"].String();
		sc.colorSuffixLength = campaign["color_suffix_length"].Float();

		for(const JsonNode &desc :  campaign["desc"].Vector())
		{
			SCampPositions::SRegionDesc rd;

			rd.infix = desc["infix"].String();
			rd.xpos = desc["x"].Float();
			rd.ypos = desc["y"].Float();
			sc.regions.push_back(rd);
		}

		campDescriptions.push_back(sc);

		idx++;
	}

	assert(idx == CGI->generaltexth->campaignMapNames.size());
}

void CBonusSelection::selectMap(int mapNr, bool initialSelect)
{
	if(initialSelect || selectedMap != mapNr)
	{
		// initialize restart / start button
		if(!ourCampaign->currentMap || *ourCampaign->currentMap != mapNr)
		{
			// draw start button
			restartB->disable();
			startB->enable();
			if (!ourCampaign->mapsConquered.empty())
				backB->block(true);
			else
				backB->block(false);
		}
		else
		{
			// draw restart button
			startB->disable();
			restartB->enable();
			backB->block(false);
		}

		startInfo.difficulty = ourCampaign->camp->scenarios[mapNr].difficulty;
		selectedMap = mapNr;
		selectedBonus = boost::none;

		std::string scenarioName = ourCampaign->camp->header.filename.substr(0, ourCampaign->camp->header.filename.find('.'));
		boost::to_lower(scenarioName);
		scenarioName += ':' + boost::lexical_cast<std::string>(selectedMap);

		//get header
		delete ourHeader;
		std::string & headerStr = ourCampaign->camp->mapPieces.find(mapNr)->second;
		auto buffer = reinterpret_cast<const ui8 *>(headerStr.data());
		ourHeader = CMapService::loadMapHeader(buffer, headerStr.size(), scenarioName).release();

		std::map<ui8, std::string> names;
		names[1] = settings["general"]["playerName"].String();
		updateStartInfo(ourCampaign->camp->header.filename, startInfo, ourHeader, names);

		mapDescription->setText(ourHeader->description);

		updateBonusSelection();

		GH.totalRedraw();
	}
}

void CBonusSelection::show(SDL_Surface * to)
{
	//blitAt(background, pos.x, pos.y, to);

	//map name
	std::string mapName = ourHeader->name;

	if (mapName.length())
		printAtLoc(mapName, 481, 219, FONT_BIG, Colors::YELLOW, to);
	else
		printAtLoc("Unnamed", 481, 219, FONT_BIG, Colors::YELLOW, to);

	//map description
	printAtLoc(CGI->generaltexth->allTexts[496], 481, 253, FONT_SMALL, Colors::YELLOW, to);

	mapDescription->showAll(to); //showAll because CTextBox has no show()

	//map size icon
	int temp;
	switch (ourHeader->width)
	{
	case 36:
		temp=0;
		break;
	case 72:
		temp=1;
		break;
	case 108:
		temp=2;
		break;
	case 144:
		temp=3;
		break;
	default:
		temp=4;
		break;
	}
	blitAtLoc(sizes->ourImages[temp].bitmap, 735, 26, to);

	//flags
	int fx = 496  + graphics->fonts[FONT_SMALL]->getStringWidth(CGI->generaltexth->allTexts[390]);
	int ex = 629 + graphics->fonts[FONT_SMALL]->getStringWidth(CGI->generaltexth->allTexts[391]);
	TeamID myT;
	myT = ourHeader->players[playerColor.getNum()].team;
	for (auto i = startInfo.playerInfos.cbegin(); i != startInfo.playerInfos.cend(); i++)
	{
		int *myx = ((i->first == playerColor  ||  ourHeader->players[i->first.getNum()].team == myT) ? &fx : &ex);
		blitAtLoc(sFlags->ourImages[i->first.getNum()].bitmap, *myx, 405, to);
		*myx += sFlags->ourImages[i->first.getNum()].bitmap->w;
	}

	//difficulty
	blitAtLoc(diffPics[startInfo.difficulty], 709, 455, to);

	CIntObject::show(to);
}

void CBonusSelection::updateBonusSelection()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	//graphics:
	//spell - SPELLBON.DEF
	//monster - TWCRPORT.DEF
	//building - BO*.BMP graphics
	//artifact - ARTIFBON.DEF
	//spell scroll - SPELLBON.DEF
	//prim skill - PSKILBON.DEF
	//sec skill - SSKILBON.DEF
	//resource - BORES.DEF
	//player - CREST58.DEF
	//hero - PORTRAITSLARGE (HPL###.BMPs)
	const CCampaignScenario &scenario = ourCampaign->camp->scenarios[selectedMap];
	const std::vector<CScenarioTravel::STravelBonus> & bonDescs = scenario.travelOptions.bonusesToChoose;

	updateStartButtonState(-1);

	delete bonuses;
	bonuses = new CToggleGroup(std::bind(&CBonusSelection::selectBonus, this, _1));

	static const char *bonusPics[] = {"SPELLBON.DEF", "TWCRPORT.DEF", "", "ARTIFBON.DEF", "SPELLBON.DEF",
		"PSKILBON.DEF", "SSKILBON.DEF", "BORES.DEF", "PORTRAITSLARGE", "PORTRAITSLARGE"};

	for(int i = 0; i < bonDescs.size(); i++)
	{
		std::string picName=bonusPics[bonDescs[i].type];
		size_t picNumber=bonDescs[i].info2;

		std::string desc;
		switch(bonDescs[i].type)
		{
		case CScenarioTravel::STravelBonus::SPELL:
			desc = CGI->generaltexth->allTexts[715];
			boost::algorithm::replace_first(desc, "%s", CGI->spellh->objects[bonDescs[i].info2]->name);
			break;
		case CScenarioTravel::STravelBonus::MONSTER:
			picNumber = bonDescs[i].info2 + 2;
			desc = CGI->generaltexth->allTexts[717];
			boost::algorithm::replace_first(desc, "%d", boost::lexical_cast<std::string>(bonDescs[i].info3));
			boost::algorithm::replace_first(desc, "%s", CGI->creh->creatures[bonDescs[i].info2]->namePl);
			break;
		case CScenarioTravel::STravelBonus::BUILDING:
			{
				int faction = -1;
				for(auto & elem : startInfo.playerInfos)
				{
					if (elem.second.playerID)
					{
						faction = elem.second.castle;
						break;
					}

				}
				assert(faction != -1);

				BuildingID buildID = CBuildingHandler::campToERMU(bonDescs[i].info1, faction, std::set<BuildingID>());
				picName = graphics->ERMUtoPicture[faction][buildID];
				picNumber = -1;

				if (vstd::contains(CGI->townh->factions[faction]->town->buildings, buildID))
					desc = CGI->townh->factions[faction]->town->buildings.find(buildID)->second->Name();
			}
			break;
		case CScenarioTravel::STravelBonus::ARTIFACT:
			desc = CGI->generaltexth->allTexts[715];
			boost::algorithm::replace_first(desc, "%s", CGI->arth->artifacts[bonDescs[i].info2]->Name());
			break;
		case CScenarioTravel::STravelBonus::SPELL_SCROLL:
			desc = CGI->generaltexth->allTexts[716];
			boost::algorithm::replace_first(desc, "%s", CGI->spellh->objects[bonDescs[i].info2]->name);
			break;
		case CScenarioTravel::STravelBonus::PRIMARY_SKILL:
			{
				int leadingSkill = -1;
				std::vector<std::pair<int, int> > toPrint; //primary skills to be listed <num, val>
				const ui8* ptr = reinterpret_cast<const ui8*>(&bonDescs[i].info2);
				for (int g=0; g<GameConstants::PRIMARY_SKILLS; ++g)
				{
					if (leadingSkill == -1 || ptr[g] > ptr[leadingSkill])
					{
						leadingSkill = g;
					}
					if (ptr[g] != 0)
					{
						toPrint.push_back(std::make_pair(g, ptr[g]));
					}
				}
				picNumber = leadingSkill;
				desc = CGI->generaltexth->allTexts[715];

				std::string substitute; //text to be printed instead of %s
				for (int v=0; v<toPrint.size(); ++v)
				{
					substitute += boost::lexical_cast<std::string>(toPrint[v].second);
					substitute += " " + CGI->generaltexth->primarySkillNames[toPrint[v].first];
					if(v != toPrint.size() - 1)
					{
						substitute += ", ";
					}
				}

				boost::algorithm::replace_first(desc, "%s", substitute);
				break;
			}
		case CScenarioTravel::STravelBonus::SECONDARY_SKILL:
			desc = CGI->generaltexth->allTexts[718];

			boost::algorithm::replace_first(desc, "%s", CGI->generaltexth->levels[bonDescs[i].info3 - 1]); //skill level
			boost::algorithm::replace_first(desc, "%s", CGI->generaltexth->skillName[bonDescs[i].info2]); //skill name
			picNumber = bonDescs[i].info2 * 3 + bonDescs[i].info3 - 1;

			break;
		case CScenarioTravel::STravelBonus::RESOURCE:
			{
				int serialResID = 0;
				switch(bonDescs[i].info1)
				{
				case 0: case 1: case 2: case 3: case 4: case 5: case 6:
					serialResID = bonDescs[i].info1;
					break;
				case 0xFD: //wood + ore
					serialResID = 7;
					break;
				case 0xFE: //rare resources
					serialResID = 8;
					break;
				}
				picNumber = serialResID;

				desc = CGI->generaltexth->allTexts[717];
				boost::algorithm::replace_first(desc, "%d", boost::lexical_cast<std::string>(bonDescs[i].info2));
				std::string replacement;
				if (serialResID <= 6)
				{
					replacement = CGI->generaltexth->restypes[serialResID];
				}
				else
				{
					replacement = CGI->generaltexth->allTexts[714 + serialResID];
				}
				boost::algorithm::replace_first(desc, "%s", replacement);
			}
			break;
		case CScenarioTravel::STravelBonus::HEROES_FROM_PREVIOUS_SCENARIO:
			{
				auto superhero = ourCampaign->camp->scenarios[bonDescs[i].info2].strongestHero(PlayerColor(bonDescs[i].info1));
				if (!superhero) logGlobal->warnStream() << "No superhero! How could it be transferred?";
				picNumber = superhero ? superhero->portrait : 0;
				desc = CGI->generaltexth->allTexts[719];

				boost::algorithm::replace_first(desc, "%s", ourCampaign->camp->scenarios[bonDescs[i].info2].scenarioName); //scenario
			}

			break;
		case CScenarioTravel::STravelBonus::HERO:

			desc = CGI->generaltexth->allTexts[718];
			boost::algorithm::replace_first(desc, "%s", CGI->generaltexth->capColors[bonDescs[i].info1]); //hero's color

			if (bonDescs[i].info2 == 0xFFFF)
			{
				boost::algorithm::replace_first(desc, "%s", CGI->generaltexth->allTexts[101]); //hero's name
				picNumber = -1;
				picName = "CBONN1A3.BMP";
			}
			else
			{
				boost::algorithm::replace_first(desc, "%s", CGI->heroh->heroes[bonDescs[i].info2]->name); //hero's name
			}
			break;
		}

		CToggleButton *bonusButton = new CToggleButton(Point(475 + i*68, 455), "", CButton::tooltip(desc, desc));

		if (picNumber != -1)
			picName += ":" + boost::lexical_cast<std::string>(picNumber);

		auto   anim = new CAnimation();
		anim->setCustom(picName, 0);
		bonusButton->setImage(anim);
		const SDL_Color brightYellow = { 242, 226, 110, 0 };
		bonusButton->borderColor = brightYellow;
		bonuses->addToggle(i, bonusButton);
	}

	// set bonus if already chosen
	if(vstd::contains(ourCampaign->chosenCampaignBonuses, selectedMap))
	{
		bonuses->setSelected(ourCampaign->chosenCampaignBonuses[selectedMap]);
	}
}

void CBonusSelection::updateCampaignState()
{
	ourCampaign->currentMap = selectedMap;
	if (selectedBonus)
		ourCampaign->chosenCampaignBonuses[selectedMap] = *selectedBonus;
}

void CBonusSelection::startMap()
{
	auto si = new StartInfo(startInfo);
	auto showPrologVideo = [=]()
	{
		auto exitCb = [=]()
		{
			logGlobal->infoStream() << "Starting scenario " << selectedMap;
            CGP->showLoadingScreen(std::bind(&startGame, si, (CConnection *)nullptr));
		};

		const CCampaignScenario & scenario = ourCampaign->camp->scenarios[selectedMap];
		if (scenario.prolog.hasPrologEpilog)
		{
			GH.pushInt(new CPrologEpilogVideo(scenario.prolog, exitCb));
		}
		else
		{
			exitCb();
		}
	};

	if(LOCPLINT) // we're currently ingame, so ask for starting new map and end game
	{
		GH.popInt(this);
		LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[67], [=]
		{
			updateCampaignState();
			endGame();
			GH.curInt = CGPreGame::create();
			showPrologVideo();
		}, 0);
	}
	else
	{
		updateCampaignState();
		showPrologVideo();
	}
}

void CBonusSelection::restartMap()
{
	GH.popInt(this);
	LOCPLINT->showYesNoDialog(CGI->generaltexth->allTexts[67], [=]
	{
		updateCampaignState();
		auto si = new StartInfo(startInfo);

		SDL_Event event;
		event.type = SDL_USEREVENT;
		event.user.code = PREPARE_RESTART_CAMPAIGN;
		event.user.data1 = si;
		SDL_PushEvent(&event);
	}, 0);
}

void CBonusSelection::selectBonus(int id)
{
	// Total redraw is needed because the border around the bonus images
	// have to be undrawn/drawn.
	if (!selectedBonus || *selectedBonus != id)
	{
		selectedBonus = id;
		GH.totalRedraw();

		updateStartButtonState(id);
	}

	const CCampaignScenario &scenario = ourCampaign->camp->scenarios[selectedMap];
	const std::vector<CScenarioTravel::STravelBonus> & bonDescs = scenario.travelOptions.bonusesToChoose;
	if (bonDescs[id].type == CScenarioTravel::STravelBonus::HERO)
	{
		std::map<ui8, std::string> names;
		names[1] = settings["general"]["playerName"].String();
		for(auto & elem : startInfo.playerInfos)
		{
			if(elem.first == PlayerColor(bonDescs[id].info1))
				::setPlayer(elem.second, 1, names);
			else
				::setPlayer(elem.second, 0, names);
		}
	}
}

void CBonusSelection::increaseDifficulty()
{
	startInfo.difficulty = std::min(startInfo.difficulty + 1, 4);
}

void CBonusSelection::decreaseDifficulty()
{
	startInfo.difficulty = std::max(startInfo.difficulty - 1, 0);
}

void CBonusSelection::updateStartButtonState(int selected /*= -1*/)
{
	if(selected == -1)
	{
		startB->block(ourCampaign->camp->scenarios[selectedMap].travelOptions.bonusesToChoose.size());
	}
	else if(startB->isBlocked())
	{
		startB->block(false);
	}
}

CBonusSelection::CRegion::CRegion( CBonusSelection * _owner, bool _accessible, bool _selectable, int _myNumber )
: owner(_owner), accessible(_accessible), selectable(_selectable), myNumber(_myNumber)
{
	OBJ_CONSTRUCTION;
	addUsedEvents(LCLICK | RCLICK);

	static const std::string colors[2][8] = {
		{"R", "B", "N", "G", "O", "V", "T", "P"},
		{"Re", "Bl", "Br", "Gr", "Or", "Vi", "Te", "Pi"}};

	const SCampPositions & campDsc = owner->campDescriptions[owner->ourCampaign->camp->header.mapVersion];
	const SCampPositions::SRegionDesc & desc = campDsc.regions[myNumber];
	pos.x += desc.xpos;
	pos.y += desc.ypos;

	//loading of graphics

	std::string prefix = campDsc.campPrefix + desc.infix + "_";
	std::string suffix = colors[campDsc.colorSuffixLength - 1][owner->ourCampaign->camp->scenarios[myNumber].regionColor];

	static const std::string infix [] = {"En", "Se", "Co"};
	for (int g = 0; g < ARRAY_COUNT(infix); g++)
	{
		graphics[g] = BitmapHandler::loadBitmap(prefix + infix[g] + suffix + ".BMP");
	}
	pos.w = graphics[0]->w;
	pos.h = graphics[0]->h;

}

CBonusSelection::CRegion::~CRegion()
{
	for (auto & elem : graphics)
	{
		SDL_FreeSurface(elem);
	}
}

void CBonusSelection::CRegion::clickLeft( tribool down, bool previousState )
{
	//select if selectable & clicked inside our graphic
	if ( indeterminate(down) )
	{
		return;
	}
	if( !down && selectable && !CSDL_Ext::isTransparent(graphics[0], GH.current->motion.x-pos.x, GH.current->motion.y-pos.y) )
	{
		owner->selectMap(myNumber, false);
		owner->highlightedRegion = this;
		parent->showAll(screen);
	}
}

void CBonusSelection::CRegion::clickRight( tribool down, bool previousState )
{
	//show r-click text
	if( down && !CSDL_Ext::isTransparent(graphics[0], GH.current->motion.x-pos.x, GH.current->motion.y-pos.y) &&
		rclickText.size() )
	{
		CRClickPopup::createAndPush(rclickText);
	}
}

void CBonusSelection::CRegion::show(SDL_Surface * to)
{
	//const SCampPositions::SRegionDesc & desc = owner->campDescriptions[owner->ourCampaign->camp->header.mapVersion].regions[myNumber];
	if (!accessible)
	{
		//show as striped
		blitAtLoc(graphics[2], 0, 0, to);
	}
	else if (this == owner->highlightedRegion)
	{
		//show as selected
		blitAtLoc(graphics[1], 0, 0, to);
	}
	else
	{
		//show as not selected selected
		blitAtLoc(graphics[0], 0, 0, to);
	}
}

CSavingScreen::CSavingScreen(bool hotseat)
 : CSelectionScreen(CMenuScreen::saveGame, hotseat ? CMenuScreen::MULTI_HOT_SEAT : CMenuScreen::SINGLE_PLAYER)
{
	ourGame = mapInfoFromGame();
	sInfo = *LOCPLINT->cb->getStartInfo();
	setPlayersFromGame();
}

CSavingScreen::~CSavingScreen()
{

}

ISelectionScreenInfo::ISelectionScreenInfo(const std::map<ui8, std::string> *Names /*= nullptr*/)
{
	multiPlayer = CMenuScreen::SINGLE_PLAYER;
	assert(!SEL);
	SEL = this;
	current = nullptr;

	if(Names && !Names->empty()) //if have custom set of player names - use it
		playerNames = *Names;
	else
		playerNames[1] = settings["general"]["playerName"].String(); //by default we have only one player and his name is "Player" (or whatever the last used name was)
}

ISelectionScreenInfo::~ISelectionScreenInfo()
{
	assert(SEL == this);
	SEL = nullptr;
}

void ISelectionScreenInfo::updateStartInfo(std::string filename, StartInfo & sInfo, const CMapHeader * mapHeader)
{
	::updateStartInfo(filename, sInfo, mapHeader, playerNames);
}

void ISelectionScreenInfo::setPlayer(PlayerSettings &pset, ui8 player)
{
	::setPlayer(pset, player, playerNames);
}

ui8 ISelectionScreenInfo::getIdOfFirstUnallocatedPlayer()
{
	for(auto i = playerNames.cbegin(); i != playerNames.cend(); i++)
		if(!sInfo.getPlayersSettings(i->first))  //
			return i->first;

	return 0;
}

bool ISelectionScreenInfo::isGuest() const
{
	return multiPlayer == CMenuScreen::MULTI_NETWORK_GUEST;
}

bool ISelectionScreenInfo::isHost() const
{
	return multiPlayer == CMenuScreen::MULTI_NETWORK_HOST;
}

void ChatMessage::apply(CSelectionScreen *selScreen)
{
	selScreen->card->chat->addNewMessage(playerName + ": " + message);
	GH.totalRedraw();
}

void QuitMenuWithoutStarting::apply(CSelectionScreen *selScreen)
{
	if(!selScreen->ongoingClosing)
	{
		*selScreen->serv << this; //resend to confirm
		GH.popIntTotally(selScreen); //will wait with deleting us before this thread ends
	}

	vstd::clear_pointer(selScreen->serv);
}

void PlayerJoined::apply(CSelectionScreen *selScreen)
{
	//assert(SEL->playerNames.size() == connectionID); //temporary, TODO when player exits
	SEL->playerNames[connectionID] = playerName;

	//put new player in first slot with AI
	for(auto & elem : SEL->sInfo.playerInfos)
	{
		if(!elem.second.playerID && !elem.second.compOnly)
		{
			selScreen->setPlayer(elem.second, connectionID);
			selScreen->opt->entries[elem.second.color]->selectButtons();
			break;
		}
	}

	selScreen->propagateNames();
	selScreen->propagateOptions();
	selScreen->toggleTab(selScreen->curTab);

	GH.totalRedraw();
}

void SelectMap::apply(CSelectionScreen *selScreen)
{
	if(selScreen->isGuest())
	{
		free = false;
		selScreen->changeSelection(mapInfo);
	}
}

void UpdateStartOptions::apply(CSelectionScreen *selScreen)
{
	if(!selScreen->isGuest())
		return;

	selScreen->setSInfo(*options);
}

void PregameGuiAction::apply(CSelectionScreen *selScreen)
{
	if(!selScreen->isGuest())
		return;

	switch(action)
	{
	case NO_TAB:
		selScreen->toggleTab(selScreen->curTab);
		break;
	case OPEN_OPTIONS:
		selScreen->toggleTab(selScreen->opt);
		break;
	case OPEN_SCENARIO_LIST:
		selScreen->toggleTab(selScreen->sel);
		break;
	case OPEN_RANDOM_MAP_OPTIONS:
		selScreen->toggleTab(selScreen->randMapTab);
		break;
	}
}

void RequestOptionsChange::apply(CSelectionScreen *selScreen)
{
	if(!selScreen->isHost())
		return;

	PlayerColor color = selScreen->sInfo.getPlayersSettings(playerID)->color;

	switch(what)
	{
	case TOWN:
		selScreen->opt->nextCastle(color, direction);
		break;
	case HERO:
		selScreen->opt->nextHero(color, direction);
		break;
	case BONUS:
		selScreen->opt->nextBonus(color, direction);
		break;
	}
}

void PlayerLeft::apply(CSelectionScreen *selScreen)
{
	if(selScreen->isGuest())
		return;

	SEL->playerNames.erase(playerID);

	if(PlayerSettings *s = selScreen->sInfo.getPlayersSettings(playerID)) //it's possible that player was unallocated
	{
		selScreen->setPlayer(*s, 0);
		selScreen->opt->entries[s->color]->selectButtons();
	}

	selScreen->propagateNames();
	selScreen->propagateOptions();
	GH.totalRedraw();
}

void PlayersNames::apply(CSelectionScreen *selScreen)
{
	if(selScreen->isGuest())
		selScreen->playerNames = playerNames;
}

void StartWithCurrentSettings::apply(CSelectionScreen *selScreen)
{
	startingInfo.reset();
	startingInfo.serv = selScreen->serv;
	startingInfo.sInfo = new StartInfo(selScreen->sInfo);

	if(!selScreen->ongoingClosing)
	{
		*selScreen->serv << this; //resend to confirm
	}

	selScreen->serv = nullptr; //hide it so it won't be deleted
	vstd::clear_pointer(selScreen->serverHandlingThread); //detach us
	saveGameName.clear();

    CGP->showLoadingScreen(std::bind(&startGame, startingInfo.sInfo, startingInfo.serv));
	throw 666; //EVIL, EVIL, EVIL workaround to kill thread (does "goto catch" outside listening loop)
}

CCampaignScreen::CCampaignButton::CCampaignButton(const JsonNode &config )
{
	pos.x += config["x"].Float();
	pos.y += config["y"].Float();
	pos.w = 200;
	pos.h = 116;

	campFile = config["file"].String();
	video = config["video"].String();

	OBJ_CONSTRUCTION_CAPTURING_ALL;

	status = config["open"].Bool() ? CCampaignScreen::ENABLED : CCampaignScreen::DISABLED;

	CCampaignHeader header = CCampaignHandler::getHeader(campFile);
	hoverText = header.name;

	if (status != CCampaignScreen::DISABLED)
	{
		addUsedEvents(LCLICK | HOVER);
		image = new CPicture(config["image"].String());

		hoverLabel = new CLabel(pos.w / 2, pos.h + 20, FONT_MEDIUM, CENTER, Colors::YELLOW, "");
		parent->addChild(hoverLabel);
	}

	if (status == CCampaignScreen::COMPLETED)
		checkMark = new CPicture("CAMPCHK");
}

void CCampaignScreen::CCampaignButton::clickLeft(tribool down, bool previousState)
{
	if (down)
	{
		// Close running video and open the selected campaign
		CCS->videoh->close();
		GH.pushInt( new CBonusSelection(campFile) );
	}
}

void CCampaignScreen::CCampaignButton::hover(bool on)
{
	if (on)
		hoverLabel->setText(hoverText); // Shows the name of the campaign when you get into the bounds of the button
	else
		hoverLabel->setText(" ");
}

void CCampaignScreen::CCampaignButton::show(SDL_Surface * to)
{
	if (status == CCampaignScreen::DISABLED)
		return;

	CIntObject::show(to);

	// Play the campaign button video when the mouse cursor is placed over the button
	if (hovered)
	{
		if (CCS->videoh->fname != video)
			CCS->videoh->open(video);

		CCS->videoh->update(pos.x, pos.y, to, true, false); // plays sequentially frame by frame, starts at the beginning when the video is over
	}
	else if (CCS->videoh->fname == video) // When you got out of the bounds of the button then close the video
	{
		CCS->videoh->close();
		redraw();
	}
}

CButton* CCampaignScreen::createExitButton(const JsonNode& button)
{
	std::pair<std::string, std::string> help;
	if (!button["help"].isNull() && button["help"].Float() > 0)
		help = CGI->generaltexth->zelp[button["help"].Float()];

	std::function<void()> close = std::bind(&CGuiHandler::popIntTotally, &GH, this);
	return new CButton(Point(button["x"].Float(), button["y"].Float()), button["name"].String(), help, close, button["hotkey"].Float());
}


CCampaignScreen::CCampaignScreen(const JsonNode &config)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	for(const JsonNode& node : config["images"].Vector())
		images.push_back(createPicture(node));

	if (!images.empty())
	{
		images[0]->center();              // move background to center
		moveTo(images[0]->pos.topLeft()); // move everything else to center
		images[0]->moveTo(pos.topLeft()); // restore moved twice background
		pos = images[0]->pos;             // fix height\width of this window
	}

	if (!config["exitbutton"].isNull())
	{
		back = createExitButton(config["exitbutton"]);
		back->hoverable = true;
	}

	for(const JsonNode& node : config["items"].Vector())
		campButtons.push_back(new CCampaignButton(node));
}

void CCampaignScreen::showAll(SDL_Surface *to)
{
	CIntObject::showAll(to);
	if (pos.h != to->h || pos.w != to->w)
		CMessage::drawBorder(PlayerColor(1), to, pos.w+28, pos.h+30, pos.x-14, pos.y-15);
}

void CGPreGame::showLoadingScreen(std::function<void()> loader)
{
	if (GH.listInt.size() && GH.listInt.front() == CGP) //pregame active
		CGP->removeFromGui();
	GH.pushInt(new CLoadingScreen(loader));
}

std::string CLoadingScreen::getBackground()
{
	const auto & conf = CGPreGameConfig::get().getConfig()["loading"].Vector();

	if(conf.empty())
	{
		return "loadbar";
	}
	else
	{
		return RandomGeneratorUtil::nextItem(conf, CRandomGenerator::getDefault())->String();
	}
}

CLoadingScreen::CLoadingScreen(std::function<void ()> loader):
    CWindowObject(BORDERED, getBackground()),
    loadingThread(loader)
{
	CCS->musich->stopMusic(5000);
}

CLoadingScreen::~CLoadingScreen()
{
	loadingThread.join();
}

void CLoadingScreen::showAll(SDL_Surface *to)
{
	Rect rect(0,0,to->w, to->h);
	SDL_FillRect(to, &rect, 0);

	CWindowObject::showAll(to);
}

CPrologEpilogVideo::CPrologEpilogVideo( CCampaignScenario::SScenarioPrologEpilog _spe, std::function<void()> callback ):
    CWindowObject(BORDERED),
    spe(_spe),
    positionCounter(0),
    voiceSoundHandle(-1),
    exitCb(callback)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	addUsedEvents(LCLICK);
	pos = center(Rect(0,0, 800, 600));
	updateShadow();

	CCS->videoh->open(CCampaignHandler::prologVideoName(spe.prologVideo));
	CCS->musich->playMusic("Music/" + CCampaignHandler::prologMusicName(spe.prologMusic), true);
	voiceSoundHandle = CCS->soundh->playSound(CCampaignHandler::prologVoiceName(spe.prologVideo));

	text = new CMultiLineLabel(Rect(100, 500, 600, 100), EFonts::FONT_BIG, CENTER, Colors::METALLIC_GOLD, spe.prologText );
	text->scrollTextTo(-100);
}

void CPrologEpilogVideo::show( SDL_Surface * to )
{
	CSDL_Ext::fillRectBlack(to, &pos);
	//BUG: some videos are 800x600 in size while some are 800x400
	//VCMI should center them in the middle of the screen. Possible but needs modification
	//of video player API which I'd like to avoid until we'll get rid of Windows-specific player
	CCS->videoh->update(pos.x, pos.y, to, true, false);

	//move text every 5 calls/frames; seems to be good enough
	++positionCounter;
	if(positionCounter % 5 == 0)
	{
		text->scrollTextBy(1);
	}
	else
		text->showAll(to);// blit text over video, if needed

	if (text->textSize.y + 100 < positionCounter / 5)
		clickLeft(false, false);
}

void CPrologEpilogVideo::clickLeft( tribool down, bool previousState )
{
	GH.popInt(this);
	CCS->soundh->stopSound(voiceSoundHandle);
	exitCb();
}

CSimpleJoinScreen::CSimpleJoinScreen()
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bg = new CPicture("MUDIALOG.bmp"); // address background
	pos = bg->center(); //center, window has size of bg graphic (x,y = 396,278 w=232 h=212)

	Rect boxRect(20, 20, 205, 50);
	title = new CTextBox("Enter address:", boxRect, 0, FONT_BIG, CENTER, Colors::WHITE);

	address = new CTextInput(Rect(25, 68, 175, 16), *bg);
    address->cb += std::bind(&CSimpleJoinScreen::onChange, this, _1);

	port = new CTextInput(Rect(25, 115, 175, 16), *bg);
    port->cb += std::bind(&CSimpleJoinScreen::onChange, this, _1);
    port->filters += std::bind(&CTextInput::numberFilter, _1, _2, 0, 65535);

	ok     = new CButton(Point( 26, 142), "MUBCHCK.DEF", CGI->generaltexth->zelp[560], std::bind(&CSimpleJoinScreen::enterSelectionScreen, this), SDLK_RETURN);
	cancel = new CButton(Point(142, 142), "MUBCANC.DEF", CGI->generaltexth->zelp[561], std::bind(&CGuiHandler::popIntTotally, std::ref(GH), this), SDLK_ESCAPE);
	bar = new CGStatusBar(new CPicture(Rect(7, 186, 218, 18), 0));

	port->setText(boost::lexical_cast<std::string>(settings["server"]["port"].Float()), true);
	address->setText(settings["server"]["server"].String(), true);
	address->giveFocus();
}

void CSimpleJoinScreen::enterSelectionScreen()
{
	std::string textAddress = address->text;
	std::string textPort = port->text;

	GH.popIntTotally(this);
	GH.pushInt(new CSelectionScreen(CMenuScreen::newGame, CMenuScreen::MULTI_NETWORK_GUEST, nullptr, textAddress, textPort));
}

void CSimpleJoinScreen::onChange(const std::string & newText)
{
	ok->block(address->text.empty() || port->text.empty());
}

