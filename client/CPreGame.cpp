#include "StdInc.h"
#include "CPreGame.h"

#include "../lib/Filesystem/CResourceLoader.h"
#include "../lib/Filesystem/CFileInfo.h"
#include "../lib/Filesystem/CCompressedStream.h"

#include "../lib/CStopWatch.h"
#include "UIFramework/SDL_Extensions.h"
#include "CGameInfo.h"
#include "UIFramework/CCursorHandler.h"
#include "CAnimation.h"
#include "CDefHandler.h"
#include "../lib/CDefObjInfoHandler.h"
#include "../lib/CGeneralTextHandler.h"
#include "../lib/CTownHandler.h"
#include "../lib/CHeroHandler.h"
#include "../lib/CObjectHandler.h"
#include "../lib/CCampaignHandler.h"
#include "../lib/CCreatureHandler.h"
#include "../lib/JsonNode.h"
#include "CMusicHandler.h"
#include "CVideoHandler.h"
#include "Graphics.h"
#include "../lib/Connection.h"
#include "../lib/VCMIDirs.h"
#include "../lib/map.h"
#include "GUIClasses.h"
#include "CPlayerInterface.h"
#include "../CCallback.h"
#include "CMessage.h"
#include "../lib/CSpellHandler.h" /*for campaign bonuses*/
#include "../lib/CArtHandler.h" /*for campaign bonuses*/
#include "../lib/CBuildingHandler.h" /*for campaign bonuses*/
#include "CBitmapHandler.h"
#include "Client.h"
#include "../lib/NetPacks.h"
#include "../lib/RegisterTypes.h"
#include "../lib/CThreadHelper.h"
#include "CConfigHandler.h"
#include "../lib/GameConstants.h"
#include "UIFramework/CGuiHandler.h"
#include "UIFramework/CIntObjectClasses.h"

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
using boost::bind;
using boost::ref;

#if _MSC_VER >= 1600
	//#define bind boost::bind
	//#define ref boost::ref
#endif

void startGame(StartInfo * options, CConnection *serv = NULL);

CGPreGame * CGP = nullptr;
ISelectionScreenInfo *SEL;

static int playerColor; //if more than one player - applies to the first

static std::string selectedName; //set when game is started/loaded


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
			serv = NULL;
			sInfo = NULL;
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
	CMapInfo *ret = new CMapInfo();
	CMapHeader *headerCopy = new CMapHeader(*LOCPLINT->cb->getMapHeader()); //will be deleted by CMapInfo d-tor
	ret->setHeader(headerCopy);
	return ret;
}

static void setPlayersFromGame()
{
	playerColor = LOCPLINT->playerID;
}

static void swapPlayers(PlayerSettings &a, PlayerSettings &b)
{
	std::swap(a.human, b.human);
	std::swap(a.name, b.name);

	if(a.human == 1)
		playerColor = a.color;
	else if(b.human == 1)
		playerColor = b.color;
}

void setPlayer(PlayerSettings &pset, unsigned player, const std::map<ui32, std::string> &playerNames)
{
	if(vstd::contains(playerNames, player))
		pset.name = playerNames.find(player)->second;
	else
		pset.name = CGI->generaltexth->allTexts[468];//Computer

	pset.human = player;
	if(player == playerNames.begin()->first)
		playerColor = pset.color;
}

void updateStartInfo(std::string filename, StartInfo & sInfo, const CMapHeader * mapHeader, const std::map<ui32, std::string> &playerNames)
{
	sInfo.playerInfos.clear();
	if(!filename.size())
		return;

	/*sInfo.playerInfos.resize(to->playerAmnt);*/
	sInfo.mapname = filename;
	playerColor = -1;

	std::map<ui32, std::string>::const_iterator namesIt = playerNames.begin();

	for (int i = 0; i < GameConstants::PLAYER_LIMIT; i++)
	{
		const PlayerInfo &pinfo = mapHeader->players[i];

		//neither computer nor human can play - no player
		if (!(pinfo.canComputerPlay || pinfo.canComputerPlay))
			continue;

		PlayerSettings &pset = sInfo.playerInfos[i];
		pset.color = i;
		if(pinfo.canHumanPlay && namesIt != playerNames.end())
			setPlayer(pset, namesIt++->first, playerNames);
		else
			setPlayer(pset, 0, playerNames);

		pset.castle = pinfo.defaultCastle();
		pset.hero = pinfo.defaultHero();


		if(pinfo.mainHeroName.length())
		{
			pset.heroName = pinfo.mainHeroName;
			if((pset.heroPortrait = pinfo.mainHeroPortrait) == 255)
				pset.heroPortrait = pinfo.p9;
		}
		pset.handicap = 0;
	}
}

template <typename T> class CApplyOnPG;

class CBaseForPGApply
{
public:
	virtual void applyOnPG(CSelectionScreen *selScr, void *pack) const =0;
	virtual ~CBaseForPGApply(){};
	template<typename U> static CBaseForPGApply *getApplier(const U * t=NULL)
	{
		return new CApplyOnPG<U>;
	}
};

template <typename T> class CApplyOnPG : public CBaseForPGApply
{
public:
	void applyOnPG(CSelectionScreen *selScr, void *pack) const
	{
		T *ptr = static_cast<T*>(pack);
		ptr->apply(selScr);
	}
};

static CApplier<CBaseForPGApply> *applier = NULL;

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

	BOOST_FOREACH(const JsonNode& node, config["items"].Vector())
		menuNameToEntry.push_back(node["name"].String());

	BOOST_FOREACH(const JsonNode& node, config["images"].Vector())
		images.push_back(createPicture(node));

	//Hardcoded entry
	menuNameToEntry.push_back("credits");

	tabs = new CTabbedInt(boost::bind(&CMenuScreen::createTab, this, _1), CTabbedInt::DestroyFunc());
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
		CMessage::drawBorder(1, to, pos.w+28, pos.h+30, pos.x-14, pos.y-15);

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

//funciton for std::string -> boost::function conversion for main menu
static boost::function<void()> genCommand(CMenuScreen* menu, std::vector<std::string> menuType, const std::string &string)
{
	static const std::vector<std::string> commandType  = boost::assign::list_of
		("to")("campaigns")("start")("load")("exit")("highscores");

	static const std::vector<std::string> gameType = boost::assign::list_of
		("single")("multi")("campaign")("tutorial");

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
						return boost::bind(&CMenuScreen::switchToTab, menu, index2);
				}
				break; case 1://open campaign selection window
				{
					return boost::bind(&CGPreGame::openCampaignScreen, CGP, commands.front());
				}
				break; case 2://start
				{
					switch (std::find(gameType.begin(), gameType.end(), commands.front()) - gameType.begin())
					{
						case 0: return bind(&CGPreGame::openSel, CGP, CMenuScreen::newGame, CMenuScreen::SINGLE_PLAYER);
						case 1: return &pushIntT<CMultiMode>;
						case 2: return boost::bind(&CGPreGame::openSel, CGP, CMenuScreen::campaignList, CMenuScreen::SINGLE_PLAYER);
						case 3: return boost::function<void()>();//TODO: start tutorial
					}
				}
				break; case 3://load
				{
					switch (std::find(gameType.begin(), gameType.end(), commands.front()) - gameType.begin())
					{
						case 0: return boost::bind(&CGPreGame::openSel, CGP, CMenuScreen::loadGame, CMenuScreen::SINGLE_PLAYER);
						case 1: return boost::bind(&CGPreGame::openSel, CGP, CMenuScreen::loadGame, CMenuScreen::MULTI_HOT_SEAT);
						case 2: return boost::function<void()>();//TODO: load campaign
						case 3: return boost::function<void()>();//TODO: load tutorial
					}
				}
				break; case 4://exit
				{
					return boost::bind(CInfoWindow::showYesNoDialog, boost::ref(CGI->generaltexth->allTexts[69]), (const std::vector<CComponent*>*)0, do_quit, 0, false, 1);
				}
				break; case 5://highscores
				{
					return boost::function<void()>(); //TODO: high scores &pushIntT<CHighScores>;
				}
			}
		}
	}
	tlog0<<"Failed to parse command: "<<string<<"\n";
	return boost::function<void()>();
}

CAdventureMapButton* CMenuEntry::createButton(CMenuScreen* parent, const JsonNode& button)
{
	boost::function<void()> command = genCommand(parent, parent->menuNameToEntry, button["command"].String());

	std::pair<std::string, std::string> help;
	if (!button["help"].isNull() && button["help"].Float() > 0)
		help = CGI->generaltexth->zelp[button["help"].Float()];

	int posx = button["x"].Float();
	if (posx < 0)
		posx = pos.w + posx;

	int posy = button["y"].Float();
	if (posy < 0)
		posy = pos.h + posy;

	return new CAdventureMapButton(help, command, posx, posy, button["name"].String(), button["hotkey"].Float());
}

CMenuEntry::CMenuEntry(CMenuScreen* parent, const JsonNode &config)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	type |= REDRAW_PARENT;
	pos = parent->pos;

	BOOST_FOREACH(const JsonNode& node, config["images"].Vector())
		images.push_back(createPicture(node));

	BOOST_FOREACH(const JsonNode& node, config["buttons"].Vector())
	{
		buttons.push_back(createButton(parent, node));
		buttons.back()->hoverable = true;
		buttons.back()->type |= REDRAW_PARENT;
	}
}

CreditsScreen::CreditsScreen()
{
	addUsedEvents(LCLICK | RCLICK);
	type |= REDRAW_PARENT;
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos.w = CGP->menu->pos.w;
	pos.h = CGP->menu->pos.h;
	auto textFile = CResourceHandler::get()->loadData(ResourceID("DATA/CREDITS.TXT"));
	std::string text((char*)textFile.first.get(), textFile.second);
	size_t firstQuote = text.find('\"')+1;
	text = text.substr(firstQuote, text.find('\"', firstQuote) - firstQuote );
	credits = new CTextBox(text, Rect(pos.w - 350, 600, 350, 32000), 0, FONT_CREDITS, CENTER, Colors::Cornsilk);
	credits->pos.h = credits->maxH;
}

void CreditsScreen::showAll(SDL_Surface * to)
{
	//Do not draw anything
}

void CreditsScreen::show(SDL_Surface * to)
{
	static int count = 0;
	count++;
	if (count == 2)
	{
		credits->pos.y--;
		count = 0;
	}
	Rect creditsArea = credits->pos & pos;
	SDL_SetClipRect(screenBuf, &creditsArea);
	SDL_SetClipRect(screen, &creditsArea);
	redraw();
	CIntObject::showAll(to);
	SDL_SetClipRect(screen, NULL);
	SDL_SetClipRect(screenBuf, NULL);

	//end of credits, close this screen
	if (credits->pos.y + credits->pos.h < 0)
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

CGPreGame::CGPreGame():
	pregameConfig(new JsonNode(ResourceID("config/mainmenu.json")))
{
	pos.w = screen->w;
	pos.h = screen->h;

	GH.defActionsDef = 63;
	CGP = this;
	menu = new CMenuScreen((*pregameConfig)["window"]);
	loadGraphics();
}

CGPreGame::~CGPreGame()
{
	disposeGraphics();
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
	bonuses = CDefHandler::giveDef("SCNRSTAR.DEF");
	rHero = BitmapHandler::loadBitmap("HPSRAND1.bmp");
	rTown = BitmapHandler::loadBitmap("HPSRAND0.bmp");
	nHero = BitmapHandler::loadBitmap("HPSRAND6.bmp");
	nTown = BitmapHandler::loadBitmap("HPSRAND5.bmp");
}

void CGPreGame::disposeGraphics()
{
	delete victory;
	delete loss;
	SDL_FreeSurface(rHero);
	SDL_FreeSurface(nHero);
	SDL_FreeSurface(rTown);
	SDL_FreeSurface(nTown);
}

void CGPreGame::update()
{
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

	if (GH.curInt == NULL) // no redraw, when a new game was created
		return;

	GH.topInt()->show(screen);

	if (settings["general"]["showfps"].Bool())
		GH.drawFPSCounter();

	// draw the mouse cursor and update the screen
	CCS->curh->draw1();
	CSDL_Ext::update(screen);
	CCS->curh->draw2();
}

void CGPreGame::openCampaignScreen(std::string name)
{
	BOOST_FOREACH(const JsonNode& node, (*pregameConfig)["campaignsset"].Vector())
	{
		if (node["name"].String() == name)
		{
			GH.pushInt(new CCampaignScreen(node));
			return;
		}
	}
	tlog1<<"Unknown campaign set: "<<name<<"\n";
}

CGPreGame *CGPreGame::create()
{
	if(!CGP)
		CGP = new CGPreGame();
	return CGP;
}

void CGPreGame::removeFromGui()
{
	//remove everything but main menu and background
	GH.popInts(GH.listInt.size() - 2);
	GH.popInt(GH.topInt()); //remove main menu
	GH.popInt(GH.topInt()); //remove background
}

CSelectionScreen::CSelectionScreen(CMenuScreen::EState Type, CMenuScreen::EMultiMode MultiPlayer /*= CMenuScreen::SINGLE_PLAYER*/, const std::map<ui32, std::string> *Names /*= NULL*/)
	: ISelectionScreenInfo(Names), serverHandlingThread(NULL), mx(new boost::recursive_mutex),
	  serv(NULL), ongoingClosing(false), myNameID(255)
{
	CGPreGame::create(); //we depend on its graphics
	screenType = Type;
	multiPlayer = MultiPlayer;

	OBJ_CONSTRUCTION_CAPTURING_ALL;

	bool network = (MultiPlayer == CMenuScreen::MULTI_NETWORK_GUEST || MultiPlayer == CMenuScreen::MULTI_NETWORK_HOST);

	CServerHandler *sh = NULL;
	if(multiPlayer == CMenuScreen::MULTI_NETWORK_HOST)
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
		const JsonVector & bgNames = (*CGP->pregameConfig)["game-select"].Vector();
		bg = new CPicture(bgNames[rand() % bgNames.size()].String(), 0, 0);
		pos = bg->center();
	}

	sInfo.difficulty = 1;
	current = NULL;

	sInfo.mode = (Type == CMenuScreen::newGame ? StartInfo::NEW_GAME : StartInfo::LOAD_GAME);
	sInfo.turnTime = 0;
	curTab = NULL;

	card = new InfoCard(network); //right info card
	if (screenType == CMenuScreen::campaignList)
	{
		opt = NULL;
	}
	else
	{
		opt = new OptionsTab(); //scenario options tab
		opt->recActions = DISPOSE;
	}
	sel = new SelectionTab(screenType, bind(&CSelectionScreen::changeSelection, this, _1), multiPlayer); //scenario selection tab
	sel->recActions = DISPOSE;

	switch(screenType)
	{
	case CMenuScreen::newGame:
		{
			card->difficulty->onChange = bind(&CSelectionScreen::difficultyChange, this, _1);
			card->difficulty->select(1, 0);
			CAdventureMapButton *select = new CAdventureMapButton(CGI->generaltexth->zelp[45], bind(&CSelectionScreen::toggleTab, this, sel), 411, 80, "GSPBUTT.DEF", SDLK_s);
			select->addTextOverlay(CGI->generaltexth->allTexts[500], FONT_SMALL);

			CAdventureMapButton *opts = new CAdventureMapButton(CGI->generaltexth->zelp[46], bind(&CSelectionScreen::toggleTab, this, opt), 411, 510, "GSPBUTT.DEF", SDLK_a);
			opts->addTextOverlay(CGI->generaltexth->allTexts[501], FONT_SMALL);

			CAdventureMapButton *random = new CAdventureMapButton(CGI->generaltexth->zelp[47], bind(&CSelectionScreen::toggleTab, this, sel), 411, 105, "GSPBUTT.DEF", SDLK_r);
			random->addTextOverlay(CGI->generaltexth->allTexts[740], FONT_SMALL);

			start  = new CAdventureMapButton(CGI->generaltexth->zelp[103], bind(&CSelectionScreen::startGame, this), 411, 535, "SCNRBEG.DEF", SDLK_b);

			if(network)
			{
				CAdventureMapButton *hideChat = new CAdventureMapButton(CGI->generaltexth->zelp[48], bind(&InfoCard::toggleChat, card), 619, 83, "GSPBUT2.DEF", SDLK_h);
				hideChat->addTextOverlay(CGI->generaltexth->allTexts[531], FONT_SMALL);

				if(multiPlayer == CMenuScreen::MULTI_NETWORK_GUEST)
				{
					SDL_Color orange = {232, 184, 32, 0};
					select->text->color = opts->text->color = random->text->color = orange;
					select->block(true);
					opts->block(true);
					random->block(true);
					start->block(true);
				}
			}
		}
		break;
	case CMenuScreen::loadGame:
		sel->recActions = 255;
		start  = new CAdventureMapButton(CGI->generaltexth->zelp[103], bind(&CSelectionScreen::startGame, this), 411, 535, "SCNRLOD.DEF", SDLK_l);
		break;
	case CMenuScreen::saveGame:
		sel->recActions = 255;
		start  = new CAdventureMapButton("", CGI->generaltexth->zelp[103].second, bind(&CSelectionScreen::startGame, this), 411, 535, "SCNRSAV.DEF");
		break;
	case CMenuScreen::campaignList:
		sel->recActions = 255;
		start  = new CAdventureMapButton(std::pair<std::string, std::string>(), bind(&CSelectionScreen::startCampaign, this), 411, 535, "SCNRLOD.DEF", SDLK_b);
		break;
	}

	start->assignedKeys.insert(SDLK_RETURN);

	std::string backName;
	if(Type == CMenuScreen::campaignList)
	{
		backName = "SCNRBACK.DEF";
	}
	else
	{
		backName = "SCNRBACK.DEF";
	}

	back = new CAdventureMapButton("", CGI->generaltexth->zelp[105].second, bind(&CGuiHandler::popIntTotally, &GH, this), 581, 535, backName, SDLK_ESCAPE);

	if(network)
	{
		if(multiPlayer == CMenuScreen::MULTI_NETWORK_HOST)
		{
			assert(playerNames.size() == 1  &&  vstd::contains(playerNames, 1)); //TODO hot-seat/network combo
			serv = sh->connectToServer();
			*serv << (ui8) 4;
			myNameID = 1;
		}
		else
		{
			serv = CServerHandler::justConnectToServer();
		}

		*serv << playerNames.begin()->second;

		if(multiPlayer == CMenuScreen::MULTI_NETWORK_GUEST)
		{
			const CMapInfo *map;
			*serv >> myNameID >> map;
			serv->connectionID = myNameID;
			changeSelection(map);
		}
		else //host
		{
			if(current)
			{
				SelectMap sm(*current);
				*serv << &sm;

				UpdateStartOptions uso(sInfo);
				*serv << &uso;
			}
		}

		applier = new CApplier<CBaseForPGApply>;
		registerTypes4(*applier);
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
	playerColor = -1;
	playerNames.clear();

	assert(!serv);
	vstd::clear_pointer(applier);
	delete mx;
}

void CSelectionScreen::toggleTab(CIntObject *tab)
{
	if(multiPlayer == CMenuScreen::MULTI_NETWORK_HOST && serv)
	{
		PregameGuiAction pga;
		if(tab == curTab)
			pga.action = PregameGuiAction::NO_TAB;
		else if(tab == opt)
			pga.action = PregameGuiAction::OPEN_OPTIONS;
		else if(tab == sel)
			pga.action = PregameGuiAction::OPEN_SCENARIO_LIST;

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
		curTab = NULL;
	};
	GH.totalRedraw();
}

void CSelectionScreen::changeSelection( const CMapInfo *to )
{
	if(multiPlayer == CMenuScreen::MULTI_NETWORK_GUEST)
	{
		vstd::clear_pointer(current);
	}

	current = to;

	if(to && (screenType == CMenuScreen::loadGame ||
			  screenType == CMenuScreen::saveGame))
	   SEL->sInfo.difficulty = to->scenarioOpts->difficulty;
	if(screenType != CMenuScreen::campaignList)
	{
		updateStartInfo(to ? to->fileURI : "", sInfo, to ? to->mapHeader : NULL);
	}
	card->changeSelection(to);
	if(screenType != CMenuScreen::campaignList)
	{
		opt->recreate();
	}

	if(multiPlayer == CMenuScreen::MULTI_NETWORK_HOST && serv)
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
	{
		CCampaign * ourCampaign = CCampaignHandler::getCampaign(SEL->current->fileURI);
		CCampaignState * campState = new CCampaignState();
		campState->camp = ourCampaign;
		GH.pushInt( new CBonusSelection(campState) );
	}
}

void CSelectionScreen::startGame()
{
	if(screenType == CMenuScreen::newGame)
	{
		//there must be at least one human player before game can be started
		std::map<int, PlayerSettings>::const_iterator i;
		for(i = SEL->sInfo.playerInfos.begin(); i != SEL->sInfo.playerInfos.end(); i++)
			if(i->second.human)
				break;

		if(i == SEL->sInfo.playerInfos.end())
		{
			GH.pushInt(CInfoWindow::create(CGI->generaltexth->allTexts[530])); //You must position yourself prior to starting the game.
			return;
		}
	}

	if(multiPlayer == CMenuScreen::MULTI_NETWORK_HOST)
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

		selectedName = sInfo.mapname;
		StartInfo *si = new StartInfo(sInfo);
		CGP->removeFromGui();
		//SEL->current = NULL;
		//curOpts = NULL;
		::startGame(si);
	}
	else
	{
		if(!(sel && sel->txt && sel->txt->text.size()))
			return;

		selectedName = "Saves/" + sel->txt->text;

		CFunctionList<void()> overWrite;
		overWrite += boost::bind(&CCallback::save, LOCPLINT->cb, selectedName);
		overWrite += bind(&CGuiHandler::popIntTotally, &GH, this);

		if(CResourceHandler::get()->existsResource(ResourceID(selectedName, EResType::LIB_SAVEGAME)))
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
			CPackForSelectionScreen *pack = NULL;
			*serv >> pack;
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
	} HANDLE_EXCEPTION
	catch(int i)
	{
		if(i != 666)
			throw;
	}
}

void CSelectionScreen::setSInfo(const StartInfo &si)
{
	std::map<int, PlayerSettings>::const_iterator i;
	for(i = si.playerInfos.begin(); i != si.playerInfos.end(); i++)
	{
		if(i->second.human == myNameID)
		{
			playerColor = i->first;
			break;
		}
	}

	if(i == si.playerInfos.end()) //not found
		playerColor = -1;

	sInfo = si;
	if(current)
		opt->recreate(); //will force to recreate using current sInfo

	card->difficulty->select(si.difficulty, 0);

	GH.totalRedraw();
}

void CSelectionScreen::processPacks()
{
	boost::unique_lock<boost::recursive_mutex> lll(*mx);
	while(upcomingPacks.size())
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
		CMessage::drawBorder(1, to, pos.w+28, pos.h+30, pos.x-14, pos.y-15);
}

// A new size filter (Small, Medium, ...) has been selected. Populate
// selMaps with the relevant data.
void SelectionTab::filter( int size, bool selectFirst )
{
	curItems.clear();

	if(tabType == CMenuScreen::campaignList)
	{
		for (size_t i=0; i<allItems.size(); i++)
			curItems.push_back(&allItems[i]);
	}
	else
	{
		for (size_t i=0; i<allItems.size(); i++)
			if( allItems[i].mapHeader && allItems[i].mapHeader->version  &&  (!size || allItems[i].mapHeader->width == size))
				curItems.push_back(&allItems[i]);
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
		onSelect(NULL);
	}
}

std::vector<ResourceID> SelectionTab::getFiles(std::string dirURI, int resType)
{
	std::vector<ResourceID> ret;
	boost::to_upper(dirURI);

	auto iterator = CResourceHandler::get()->getIterator([&](const ResourceID & ident)
	{
		return ident.getType() == resType
		    && boost::algorithm::starts_with(ident.getName(), dirURI);
	});

	while (iterator.hasNext())
	{
		ret.push_back(*iterator);
		++iterator;
	}

	allItems.resize(ret.size());
	return ret;
}

void SelectionTab::parseMaps(const std::vector<ResourceID> &files, int start, int threads)
{
	ui8 mapBuffer[1500];

	while(start < allItems.size())
	{
		try
		{
			CCompressedStream stream(std::move(CResourceHandler::get()->load(files[start])), true);
			int read = stream.read(mapBuffer, 1500);

			if(read < 50  ||  !mapBuffer[4])
				throw std::runtime_error("corrupted map file");

			allItems[start].mapInit(files[start].getName(), mapBuffer);
		}
		catch(std::exception &e)
		{
			tlog3 << "\t\tWarning: failed to load map " << files[start].getName() << ": " << e.what() << std::endl;
		}
		start += threads;
	}
}

void SelectionTab::parseGames(const std::vector<ResourceID> &files, bool multi)
{
	for(int i=0; i<files.size(); i++)
	{
		try
		{
			CLoadFile lf(CResourceHandler::get()->getResourceName(files[i]));

			ui8 sign[8];
			lf >> sign;
			if(std::memcmp(sign,"VCMISVG",7))
				throw std::runtime_error("not a correct savefile!");

			allItems[i].mapHeader = new CMapHeader();
			lf >> *(allItems[i].mapHeader) >> allItems[i].scenarioOpts;
			allItems[i].fileURI = files[i].getName();
			allItems[i].countPlayers();
			std::time_t time = CFileInfo(CResourceHandler::get()->getResourceName(files[i])).getDate();
			allItems[i].date = std::asctime(std::localtime(&time));

			if((allItems[i].actualHumanPlayers > 1) != multi) //if multi mode then only multi games, otherwise single
			{
				vstd::clear_pointer(allItems[i].mapHeader);
			}
		}
		catch(std::exception &e)
		{
			vstd::clear_pointer(allItems[i].mapHeader);
			tlog3 << "Failed to process " << files[i].getName() <<": " << e.what() << std::endl;
		}
	}
}

void SelectionTab::parseCampaigns(const std::vector<ResourceID> & files )
{
	for(int i=0; i<files.size(); i++)
	{
		//allItems[i].date = std::asctime(std::localtime(&files[i].date));
		allItems[i].fileURI = files[i].getName();
		allItems[i].campaignInit();
	}
}

SelectionTab::SelectionTab(CMenuScreen::EState Type, const boost::function<void(CMapInfo *)> &OnSelect, CMenuScreen::EMultiMode MultiPlayer /*= CMenuScreen::SINGLE_PLAYER*/)
	:bg(NULL), onSelect(OnSelect)
{
	OBJ_CONSTRUCTION;
	selectionPos = 0;
	addUsedEvents(LCLICK | WHEEL | KEYBOARD | DOUBLECLICK);
	slider = NULL;
	txt = NULL;
	tabType = Type;

	if (Type != CMenuScreen::campaignList)
	{
		bg = new CPicture("SCSELBCK.bmp", 0, 6);
		pos = bg->pos;
	}
	else
	{
		bg = nullptr; //use background from parent
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
		std::vector<CCampaignHeader> cpm;
		switch(tabType)
		{
		case CMenuScreen::newGame:
			parseMaps(getFiles("Maps/", EResType::MAP));
			positions = 18;
			break;

		case CMenuScreen::loadGame:
		case CMenuScreen::saveGame:
			parseGames(getFiles("Saves/", EResType::LIB_SAVEGAME), MultiPlayer);
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
				txt->filters.add(CTextInput::filenameFilter);
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



	if (tabType != CMenuScreen::campaignList)
	{
		//size filter buttons
		{
			int sizes[] = {36, 72, 108, 144, 0};
			const char * names[] = {"SCSMBUT.DEF", "SCMDBUT.DEF", "SCLGBUT.DEF", "SCXLBUT.DEF", "SCALBUT.DEF"};
			for(int i = 0; i < 5; i++)
				new CAdventureMapButton("", CGI->generaltexth->zelp[54+i].second, bind(&SelectionTab::filter, this, sizes[i], true), 158 + 47*i, 46, names[i]);
		}

		//sort buttons buttons
		{
			int xpos[] = {23, 55, 88, 121, 306, 339};
			const char * names[] = {"SCBUTT1.DEF", "SCBUTT2.DEF", "SCBUTCP.DEF", "SCBUTT3.DEF", "SCBUTT4.DEF", "SCBUTT5.DEF"};
			for(int i = 0; i < 6; i++)
				new CAdventureMapButton("", CGI->generaltexth->zelp[107+i].second, bind(&SelectionTab::sortBy, this, i), xpos[i], 86, names[i]);
		}
	}
	else
	{
		//sort by buttons
		new CAdventureMapButton("", "", bind(&SelectionTab::sortBy, this, _numOfMaps), 23, 86, "CamCusM.DEF"); //by num of maps
		new CAdventureMapButton("", "", bind(&SelectionTab::sortBy, this, _name), 55, 86, "CamCusL.DEF"); //by name
	}

	slider = new CSlider(372, 86, tabType != CMenuScreen::saveGame ? 480 : 430, bind(&SelectionTab::sliderMove, this, _1), positions, curItems.size(), 0, false, 1);
	slider->addUsedEvents(WHEEL);
	slider->slider->keepFrame = true;
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
		if(selectedName.size())
		{
			if(selectedName[2] == 'M') //name starts with ./Maps instead of ./Games => there was nothing to select
				txt->setTxt("NEWGAME");
			else
				selectFName(selectedName);
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
	if(sortingBy != _name)
		std::stable_sort(curItems.begin(), curItems.end(), mapSorter(_name));
	std::stable_sort(curItems.begin(), curItems.end(), mapSorter(sortingBy));

	if(!ascending)
		std::reverse(curItems.begin(), curItems.end());

	redraw();
}

void SelectionTab::select( int position )
{
	if(!curItems.size()) return;

	// New selection. py is the index in curItems.
	int py = position + slider->value;
	vstd::amax(py, 0);
	vstd::amin(py, curItems.size()-1);

	selectionPos = py;

	if(position < 0)
		slider->moveTo(slider->value + position);
	else if(position >= positions)
		slider->moveTo(slider->value + position - positions + 1);

	if(txt)
	{
		std::string filename = CResourceHandler::get()->getResourceName(
		                           ResourceID(curItems[py]->fileURI, EResType::MAP));
		txt->setTxt(CFileInfo(filename).getBaseName());
	}

	onSelect(curItems[py]);
}

void SelectionTab::selectAbs( int position )
{
	select(position - slider->value);
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

	int elemIdx = slider->value;

	// Display all elements if there's enough space
	//if(slider->amount < slider->capacity)
	//	elemIdx = 0;


	SDL_Color itemColor;
#define POS(xx, yy) pos.x + xx, pos.y + yy + line*25
	for (int line = 0; line < positions  &&  elemIdx < curItems.size(); elemIdx++, line++)
	{
		CMapInfo *currentItem = curItems[elemIdx];

		if (elemIdx == selectionPos)
			itemColor=Colors::Jasmine;
		else
			itemColor=Colors::Cornsilk;

		if(tabType != CMenuScreen::campaignList)
		{
			//amount of players
			std::ostringstream ostr(std::ostringstream::out);
			ostr << currentItem->playerAmnt << "/" << currentItem->humanPlayers;
			CSDL_Ext::printAt(ostr.str(), POS(29, 120), FONT_SMALL, itemColor, to);

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
			CSDL_Ext::printAtMiddle(temp2, POS(70, 128), FONT_SMALL, itemColor, to);

			int temp=-1;
			switch (currentItem->mapHeader->version)
			{
			case CMapHeader::RoE:
				temp=0;
				break;
			case CMapHeader::AB:
				temp=1;
				break;
			case CMapHeader::SoD:
				temp=2;
				break;
			case CMapHeader::WoG:
				temp=3;
				break;
			default:
				// Unknown version. Be safe and ignore that map
				tlog2 << "Warning: " << currentItem->fileURI << " has wrong version!\n";
				continue;
			}
			blitAt(format->ourImages[temp].bitmap, POS(88, 117), to);

			//victory conditions
			if (currentItem->mapHeader->victoryCondition.condition == EVictoryConditionType::WINSTANDARD)
				temp = 11;
			else
				temp = currentItem->mapHeader->victoryCondition.condition;
			blitAt(CGP->victory->ourImages[temp].bitmap, POS(306, 117), to);

			//loss conditions
			if (currentItem->mapHeader->lossCondition.typeOfLossCon == ELossConditionType::LOSSSTANDARD)
				temp=3;
			else
				temp=currentItem->mapHeader->lossCondition.typeOfLossCon;
			blitAt(CGP->loss->ourImages[temp].bitmap, POS(339, 117), to);
		}
		else //if campaign
		{
			//number of maps in campaign
			std::ostringstream ostr(std::ostringstream::out);
			ostr << CGI->generaltexth->campaignRegionNames[ currentItem->campaignHeader->mapVersion ].size();
			CSDL_Ext::printAt(ostr.str(), POS(29, 120), FONT_SMALL, itemColor, to);
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
			name = CFileInfo(CResourceHandler::get()->getResourceName(
			                     ResourceID(currentItem->fileURI, EResType::LIB_SAVEGAME))).getBaseName();
		}

		//print name
		CSDL_Ext::printAtMiddle(CSDL_Ext::trimToFit(name, 185, FONT_SMALL), POS(213, 128), FONT_SMALL, itemColor, to);

	}
#undef POS
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
		title = "Select a Campaign"; //TODO: find where is the title
		break;
	}

	CSDL_Ext::printAtMiddle(title, pos.x+205, pos.y+28, FONT_MEDIUM, Colors::Jasmine, to); //Select a Scenario to Play
	if(tabType != CMenuScreen::campaignList)
	{
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[510], pos.x+87, pos.y+62, FONT_SMALL, Colors::Jasmine, to); //Map sizes
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
		select(-slider->value);
		return;
	case SDLK_END:
		select(curItems.size() - slider->value);
		return;
	default:
		return;
	}
	select(selectionPos - slider->value + moveBy);
}

void SelectionTab::onDoubleClick()
{
	if(getLine() != -1) //double clicked scenarios list
	{
		//act as if start button was pressed
		(static_cast<CSelectionScreen*>(parent))->start->callback();
	}
}

int SelectionTab::getLine()
{
	int line = -1;
	Point clickPos(GH.current->button.x, GH.current->button.y);
	clickPos = clickPos - pos.topLeft();

	if (clickPos.y > 115  &&  clickPos.y < 564  &&  clickPos.x > 22  &&  clickPos.x < 371)
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



CChatBox::CChatBox(const Rect &rect)
{
	OBJ_CONSTRUCTION;
	pos += rect;
	addUsedEvents(KEYBOARD);
	captureAllKeys = true;

	const int height = graphics->fonts[FONT_SMALL]->height;
	inputBox = new CTextInput(Rect(0, rect.h - height, rect.w, height));
	inputBox->removeUsedEvents(KEYBOARD);
	chatHistory = new CTextBox("", Rect(0, 0, rect.w, rect.h - height), 1);

	SDL_Color green = {0,252,0, SDL_ALPHA_OPAQUE};
	chatHistory->color = green;
}

void CChatBox::keyPressed(const SDL_KeyboardEvent & key)
{
	if(key.keysym.sym == SDLK_RETURN  &&  key.state == SDL_PRESSED  &&  inputBox->text.size())
	{
		SEL->postChatMessage(inputBox->text);
		inputBox->setTxt("");
	}
	else
		inputBox->keyPressed(key);
}

void CChatBox::addNewMessage(const std::string &text)
{
	chatHistory->setTxt(chatHistory->text + text + "\n");
	if(chatHistory->slider)
		chatHistory->slider->moveToMax();
}

InfoCard::InfoCard( bool Network )
  : bg(NULL), network(Network), chatOn(false), chat(NULL), playerListBg(NULL),
	difficulty(NULL), sizes(NULL), sFlags(NULL)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	pos.x += 393;
	pos.y += 6;
	addUsedEvents(RCLICK);
	mapDescription = NULL;

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
		difficulty = new CHighlightableButtonsGroup(0);
		{
			static const char *difButns[] = {"GSPBUT3.DEF", "GSPBUT4.DEF", "GSPBUT5.DEF", "GSPBUT6.DEF", "GSPBUT7.DEF"};

			for(int i = 0; i < 5; i++)
			{
				difficulty->addButton(new CHighlightableButton("", CGI->generaltexth->zelp[24+i].second, 0, 110 + i*32, 450, difButns[i], i));
			}
		}

		if(SEL->screenType != CMenuScreen::newGame)
			difficulty->block(true);

		//description needs bg
		mapDescription->addChild(new CPicture(*bg, descriptionRect), true); //move subpicture bg to our description control (by default it's our (Infocard) child)

		if(network)
		{
			playerListBg = new CPicture("CHATPLUG.bmp", 16, 276);
			chat = new CChatBox(descriptionRect);
			chat->chatHistory->addChild(new CPicture(*bg, chat->chatHistory->pos - pos), true); //move subpicture bg to our description control (by default it's our (Infocard) child)

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
		printAtLoc(CGI->generaltexth->allTexts[390] + ":", 24, 400, FONT_SMALL, Colors::Cornsilk, to); //Allies
		printAtLoc(CGI->generaltexth->allTexts[391] + ":", 190, 400, FONT_SMALL, Colors::Cornsilk, to); //Enemies
		printAtLoc(CGI->generaltexth->allTexts[494], 33, 430, FONT_SMALL, Colors::Jasmine, to);//"Map Diff:"
		printAtLoc(CGI->generaltexth->allTexts[492] + ":", 133,430, FONT_SMALL, Colors::Jasmine, to); //player difficulty
		printAtLoc(CGI->generaltexth->allTexts[218] + ":", 290,430, FONT_SMALL, Colors::Jasmine, to); //"Rating:"
		printAtLoc(CGI->generaltexth->allTexts[495], 26, 22, FONT_SMALL, Colors::Jasmine, to); //Scenario Name:
		if(!chatOn)
		{
			printAtLoc(CGI->generaltexth->allTexts[496], 26, 132, FONT_SMALL, Colors::Jasmine, to); //Scenario Description:
			printAtLoc(CGI->generaltexth->allTexts[497], 26, 283, FONT_SMALL, Colors::Jasmine, to); //Victory Condition:
			printAtLoc(CGI->generaltexth->allTexts[498], 26, 339, FONT_SMALL, Colors::Jasmine, to); //Loss Condition:
		}
		else //players list
		{
			std::map<ui32, std::string> playerNames = SEL->playerNames;
			int playerSoFar = 0;
			for (std::map<int, PlayerSettings>::const_iterator i = SEL->sInfo.playerInfos.begin(); i != SEL->sInfo.playerInfos.end(); i++)
			{
				if(i->second.human)
				{
					printAtLoc(i->second.name, 24, 285 + playerSoFar++ * graphics->fonts[FONT_SMALL]->height, FONT_SMALL, Colors::Cornsilk, to);
					playerNames.erase(i->second.human);
				}
			}

			playerSoFar = 0;
			for (std::map<ui32, std::string>::const_iterator i = playerNames.begin(); i != playerNames.end(); i++)
			{
				printAtLoc(i->second, 193, 285 + playerSoFar++ * graphics->fonts[FONT_SMALL]->height, FONT_SMALL, Colors::Cornsilk, to);
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
				//victory conditions
				temp = SEL->current->mapHeader->victoryCondition.condition+1;
				if (temp>20) temp=0;
				std::string sss = CGI->generaltexth->victoryConditions[temp];
				if (temp && SEL->current->mapHeader->victoryCondition.allowNormalVictory) sss+= "/" + CGI->generaltexth->victoryConditions[0];
				printAtLoc(sss, 60, 307, FONT_SMALL, Colors::Cornsilk, to);

				temp = SEL->current->mapHeader->victoryCondition.condition;
				if (temp>12) temp=11;
				blitAtLoc(CGP->victory->ourImages[temp].bitmap, 24, 302, to); //victory cond descr

				//loss conditoins
				temp = SEL->current->mapHeader->lossCondition.typeOfLossCon+1;
				if (temp>20) temp=0;
				sss = CGI->generaltexth->lossCondtions[temp];
				printAtLoc(sss, 60, 366, FONT_SMALL, Colors::Cornsilk, to);

				temp=SEL->current->mapHeader->lossCondition.typeOfLossCon;
				if (temp>12) temp=3;
				blitAtLoc(CGP->loss->ourImages[temp].bitmap, 24, 359, to); //loss cond
			}

			//difficulty
			assert(SEL->current->mapHeader->difficulty <= 4);
			std::string &diff = CGI->generaltexth->arraytxt[142 + SEL->current->mapHeader->difficulty];
			printAtMiddleLoc(diff, 62, 472, FONT_SMALL, Colors::Cornsilk, to);

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
				printToLoc((static_cast<const CMapInfo*>(SEL->current))->date,308,34, FONT_SMALL, Colors::Cornsilk, to);

			//print flags
			int fx = 34  + graphics->fonts[FONT_SMALL]->getWidth(CGI->generaltexth->allTexts[390].c_str());
			int ex = 200 + graphics->fonts[FONT_SMALL]->getWidth(CGI->generaltexth->allTexts[391].c_str());

			int myT;

			if(playerColor >= 0)
				myT = SEL->current->mapHeader->players[playerColor].team;
			else
				myT = -1;

			for (std::map<int, PlayerSettings>::const_iterator i = SEL->sInfo.playerInfos.begin(); i != SEL->sInfo.playerInfos.end(); i++)
			{
				int *myx = ((i->first == playerColor  ||  SEL->current->mapHeader->players[i->first].team == myT) ? &fx : &ex);
				blitAtLoc(sFlags->ourImages[i->first].bitmap, *myx, 399, to);
				*myx += sFlags->ourImages[i->first].bitmap->w;
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
			printAtMiddleLoc(tob, 311, 472, FONT_SMALL, Colors::Cornsilk, to);
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
			printAtLoc(CSDL_Ext::trimToFit(name, 300, FONT_BIG), 26, 39, FONT_BIG, Colors::Jasmine, to);
		else
			printAtLoc("Unnamed", 26, 39, FONT_BIG, Colors::Jasmine, to);
	}
}

void InfoCard::changeSelection( const CMapInfo *to )
{
	if(to && mapDescription)
	{
		if (SEL->screenType == CMenuScreen::campaignList)
			mapDescription->setTxt(to->campaignHeader->description);
		else
			mapDescription->setTxt(to->mapHeader->description);

		if(SEL->screenType != CMenuScreen::newGame && SEL->screenType != CMenuScreen::campaignList) {
			difficulty->block(true);
			difficulty->select(SEL->sInfo.difficulty, 0);
		}
	}
	GH.totalRedraw();
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
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[657], 128, 30, FONT_MEDIUM, Colors::Jasmine, bmp); //{Team Alignments}

	for(int i = 0; i < SEL->current->mapHeader->howManyTeams; i++)
	{
		std::vector<ui8> flags;
		std::string hlp = CGI->generaltexth->allTexts[656]; //Team %d
		hlp.replace(hlp.find("%d"), 2, boost::lexical_cast<std::string>(i+1));
		CSDL_Ext::printAtMiddle(hlp, 128, 65 + 50*i, FONT_SMALL, Colors::Cornsilk, bmp);

		for(int j = 0; j < GameConstants::PLAYER_LIMIT; j++)
			if((SEL->current->mapHeader->players[j].canHumanPlay || SEL->current->mapHeader->players[j].canComputerPlay)
				&& SEL->current->mapHeader->players[j].team == i)
				flags.push_back(j);

		int curx = 128 - 9*flags.size();
		for(int j = 0; j < flags.size(); j++)
		{
			blitAt(sFlags->ourImages[flags[j]].bitmap, curx, 75 + 50*i, bmp);
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
	turnDuration(NULL)
{
	OBJ_CONSTRUCTION;
	bg = new CPicture("ADVOPTBK", 0, 6);
	pos = bg->pos;

	if(SEL->screenType == CMenuScreen::newGame)
		turnDuration = new CSlider(55, 551, 194, bind(&OptionsTab::setTurnLength, this, _1), 1, 11, 11, true, 1);
}

OptionsTab::~OptionsTab()
{

}

void OptionsTab::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	printAtMiddleLoc(CGI->generaltexth->allTexts[515], 222, 30, FONT_BIG, Colors::Jasmine, to);
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[516], 222, 58, FONT_SMALL, 55, Colors::Cornsilk, to); //Select starting options, handicap, and name for each player in the game.
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[517], 107, 102, FONT_SMALL, 14, Colors::Jasmine, to); //Player Name Handicap Type
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[518], 197, 102, FONT_SMALL, 10, Colors::Jasmine, to); //Starting Town
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[519], 273, 102, FONT_SMALL, 10, Colors::Jasmine, to); //Starting Hero
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[520], 349, 102, FONT_SMALL, 10, Colors::Jasmine, to); //Starting Bonus
	printAtMiddleLoc(CGI->generaltexth->allTexts[521], 222, 538, FONT_SMALL, Colors::Jasmine, to); // Player Turn Duration
	if (turnDuration)
		printAtMiddleLoc(CGI->generaltexth->turnDurations[turnDuration->value], 319,559, FONT_SMALL, Colors::Cornsilk, to);//Turn duration value
}

void OptionsTab::nextCastle( int player, int dir )
{
	if(SEL->isGuest())
	{
		SEL->postRequest(RequestOptionsChange::TOWN, dir);
		return;
	}

	PlayerSettings &s = SEL->sInfo.playerInfos[player];
	si32 &cur = s.castle;
	ui32 allowed = SEL->current->mapHeader->players[s.color].allowedFactions;

	if (cur == -2) //no castle - no change
		return;

	if (cur == -1) //random => first/last available
	{
		int pom = (dir>0) ? (0) : (GameConstants::F_NUMBER-1); // last or first
		for (;pom >= 0  &&  pom < GameConstants::F_NUMBER;  pom+=dir)
		{
			if((1 << pom) & allowed)
			{
				cur=pom;
				break;
			}
		}
	}
	else // next/previous available
	{
		for (;;)
		{
			cur+=dir;
			if ((1 << cur) & allowed)
				break;

			if (cur >= GameConstants::F_NUMBER  ||  cur<0)
			{
				double p1 = log((double)allowed) / log(2.0)+0.000001;
				double check = p1 - ((int)p1);
				if (check < 0.001)
					cur = (int)p1;
				else
					cur = -1;
				break;
			}
		}
	}

	if(s.hero >= 0)
		s.hero = -1;
	if(cur < 0  &&  s.bonus == PlayerSettings::bresource)
		s.bonus = PlayerSettings::brandom;

	entries[player]->selectButtons();

	SEL->propagateOptions();
	redraw();
}

void OptionsTab::nextHero( int player, int dir )
{
	if(SEL->isGuest())
	{
		SEL->postRequest(RequestOptionsChange::HERO, dir);
		return;
	}

	PlayerSettings &s = SEL->sInfo.playerInfos[player];
	int old = s.hero;
	if (s.castle < 0  ||  !s.human  ||  s.hero == -2)
		return;

	if (s.hero == -1) //random => first/last available
	{
		int max = (s.castle*GameConstants::HEROES_PER_TYPE*2+15),
			min = (s.castle*GameConstants::HEROES_PER_TYPE*2);
		s.hero = nextAllowedHero(min,max,0,dir);
	}
	else
	{
		if(dir > 0)
			s.hero = nextAllowedHero(s.hero,(s.castle*GameConstants::HEROES_PER_TYPE*2+16),1,dir);
		else
			s.hero = nextAllowedHero(s.castle*GameConstants::HEROES_PER_TYPE*2-1,s.hero,1,dir);
	}

	if(old != s.hero)
	{
		usedHeroes.erase(old);
		usedHeroes.insert(s.hero);
		redraw();
	}

	SEL->propagateOptions();
}

int OptionsTab::nextAllowedHero( int min, int max, int incl, int dir )
{
	if(dir>0)
	{
		for(int i=min+incl; i<=max-incl; i++)
			if(canUseThisHero(i))
				return i;
	}
	else
	{
		for(int i=max-incl; i>=min+incl; i--)
			if(canUseThisHero(i))
				return i;
	}
	return -1;
}

bool OptionsTab::canUseThisHero( int ID )
{
	//for(int i=0;i<CPG->ret.playerInfos.size();i++)
	//	if(CPG->ret.playerInfos[i].hero==ID) //hero is already taken
	//		return false;

	return !vstd::contains(usedHeroes, ID) && SEL->current->mapHeader->allowedHeroes[ID];
}

void OptionsTab::nextBonus( int player, int dir )
{
	if(SEL->isGuest())
	{
		SEL->postRequest(RequestOptionsChange::BONUS, dir);
		return;
	}

	PlayerSettings &s = SEL->sInfo.playerInfos[player];
	si8 &ret = s.bonus += dir;

	if (s.hero==-2 && !SEL->current->mapHeader->players[s.color].heroesNames.size() && ret==PlayerSettings::bartifact) //no hero - can't be artifact
	{
		if (dir<0)
			ret=PlayerSettings::brandom;
		else ret=PlayerSettings::bgold;
	}

	if(ret > PlayerSettings::bresource)
		ret = PlayerSettings::brandom;
	if(ret < PlayerSettings::brandom)
		ret = PlayerSettings::bresource;

	if (s.castle==-1 && ret==PlayerSettings::bresource) //random castle - can't be resource
	{
		if (dir<0)
			ret=PlayerSettings::bgold;
		else ret=PlayerSettings::brandom;
	}

	SEL->propagateOptions();
	redraw();
}

void OptionsTab::recreate()
{
	for(std::map<int, PlayerOptionsEntry*>::iterator it = entries.begin(); it != entries.end(); ++it)
	{
		delete it->second;
	}
	entries.clear();
	usedHeroes.clear();

	OBJ_CONSTRUCTION_CAPTURING_ALL;
	for(std::map<int, PlayerSettings>::iterator it = SEL->sInfo.playerInfos.begin();
		it != SEL->sInfo.playerInfos.end(); ++it)
	{
		entries.insert(std::make_pair(it->first, new PlayerOptionsEntry(this, it->second)));
		const std::vector<SheroName> &heroes = SEL->current->mapHeader->players[it->first].heroesNames;
		for(size_t hi=0; hi<heroes.size(); hi++)
			usedHeroes.insert(heroes[hi].heroID);
	}

}

void OptionsTab::setTurnLength( int npos )
{
	static const int times[] = {1, 2, 4, 6, 8, 10, 15, 20, 25, 30, 0};
	vstd::amin(npos, ARRAY_COUNT(times) - 1);

	SEL->sInfo.turnTime = times[npos];
	redraw();
}

void OptionsTab::flagPressed( int color )
{
	PlayerSettings &clicked =  SEL->sInfo.playerInfos[color];
	PlayerSettings *old = NULL;

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
		int clickedNameID = clicked.human; //human is a number of player, zero means AI

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
			std::map<ui32, std::string>::const_iterator i = SEL->playerNames.find(clickedNameID); //clicked one
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
			for(std::map<int, PlayerSettings>::iterator i = SEL->sInfo.playerInfos.begin(); i != SEL->sInfo.playerInfos.end(); i++)
			{
				int curNameID = i->second.human;
				if(i->first != color  &&  curNameID == newPlayer)
				{
					assert(i->second.human);
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
	}

	SEL->propagateOptions();
	GH.totalRedraw();
}

OptionsTab::PlayerOptionsEntry::PlayerOptionsEntry( OptionsTab *owner, PlayerSettings &S)
 : pi(SEL->current->mapHeader->players[S.color]), s(S)
{
	OBJ_CONSTRUCTION;
	defActions |= SHARE_POS;

	int serial = 0;
	for(int g=0; g < s.color; ++g)
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

	bg = new CPicture(BitmapHandler::loadBitmap(bgs[s.color]), 0, 0, true);
	if(SEL->screenType == CMenuScreen::newGame)
	{
		btns[0] = new CAdventureMapButton(CGI->generaltexth->zelp[132], bind(&OptionsTab::nextCastle, owner, s.color, -1), 107, 5, "ADOPLFA.DEF");
		btns[1] = new CAdventureMapButton(CGI->generaltexth->zelp[133], bind(&OptionsTab::nextCastle, owner, s.color, +1), 168, 5, "ADOPRTA.DEF");
		btns[2] = new CAdventureMapButton(CGI->generaltexth->zelp[148], bind(&OptionsTab::nextHero, owner, s.color, -1), 183, 5, "ADOPLFA.DEF");
		btns[3] = new CAdventureMapButton(CGI->generaltexth->zelp[149], bind(&OptionsTab::nextHero, owner, s.color, +1), 244, 5, "ADOPRTA.DEF");
		btns[4] = new CAdventureMapButton(CGI->generaltexth->zelp[164], bind(&OptionsTab::nextBonus, owner, s.color, -1), 259, 5, "ADOPLFA.DEF");
		btns[5] = new CAdventureMapButton(CGI->generaltexth->zelp[165], bind(&OptionsTab::nextBonus, owner, s.color, +1), 320, 5, "ADOPRTA.DEF");
	}
	else
		for(int i = 0; i < 6; i++)
			btns[i] = NULL;

	selectButtons();

	assert(SEL->current && SEL->current->mapHeader);
	const PlayerInfo &p = SEL->current->mapHeader->players[s.color];
	assert(p.canComputerPlay || p.canHumanPlay); //someone must be able to control this player
	if(p.canHumanPlay && p.canComputerPlay)
		whoCanPlay = HUMAN_OR_CPU;
	else if(p.canComputerPlay)
		whoCanPlay = CPU;
	else
		whoCanPlay = HUMAN;

	if(SEL->screenType != CMenuScreen::scenarioInfo
		&&  SEL->current->mapHeader->players[s.color].canHumanPlay
		&&  SEL->multiPlayer != CMenuScreen::MULTI_NETWORK_GUEST)
	{
		flag = new CAdventureMapButton(CGI->generaltexth->zelp[180], bind(&OptionsTab::flagPressed, owner, s.color), -43, 2, flags[s.color]);
		flag->hoverable = true;
	}
	else
		flag = NULL;

	town = new SelectedBox(TOWN, s.color);
	town->pos.x += 119;
	town->pos.y += 2;
	hero = new SelectedBox(HERO, s.color);
	hero->pos.x += 195;
	hero->pos.y += 2;
	bonus = new SelectedBox(BONUS, s.color);
	bonus->pos.x += 271;
	bonus->pos.y += 2;
}

void OptionsTab::PlayerOptionsEntry::showAll(SDL_Surface * to)
{
	CIntObject::showAll(to);
	printAtMiddleLoc(s.name, 55, 10, FONT_SMALL, Colors::Cornsilk, to);
	printAtMiddleWBLoc(CGI->generaltexth->arraytxt[206+whoCanPlay], 28, 34, FONT_TINY, 8, Colors::Cornsilk, to);
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

	if( (pi.defaultHero() != -1  ||  !s.human  ||  s.castle < 0) //fixed hero
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

void OptionsTab::SelectedBox::showAll(SDL_Surface * to)
{
	//PlayerSettings &s = SEL->sInfo.playerInfos[player];
	SDL_Surface *toBlit = getImg();
	const std::string *toPrint = getText();
	blitAt(toBlit, pos, to);
	printAtMiddleLoc(*toPrint, 23, 39, FONT_TINY, Colors::Cornsilk, to);
}

OptionsTab::SelectedBox::SelectedBox( SelType Which, ui8 Player )
:which(Which), player(Player)
{
	SDL_Surface *img = getImg();
	pos.w = img->w;
	pos.h = img->h;
	addUsedEvents(RCLICK);
}

SDL_Surface * OptionsTab::SelectedBox::getImg() const
{
	const PlayerSettings &s = SEL->sInfo.playerInfos[player];
	switch(which)
	{
	case TOWN:
		if (s.castle < GameConstants::F_NUMBER  &&  s.castle >= 0)
			return graphics->getPic(s.castle, true, false);
		else if (s.castle == -1)
			return CGP->rTown;
		else if (s.castle == -2)
			return CGP->nTown;
	case HERO:
		if (s.hero == -1)
		{
			return CGP->rHero;
		}
		else if (s.hero == -2)
		{
			if(s.heroPortrait >= 0)
				return graphics->portraitSmall[s.heroPortrait];
			else
				return CGP->nHero;
		}
		else
		{
			return graphics->portraitSmall[s.hero];
		}
		break;
	case BONUS:
		{
			int pom;
			switch (s.bonus)
			{
			case -1:
				pom=10;
				break;
			case 0:
				pom=9;
				break;
			case 1:
				pom=8;
				break;
			case 2:
				pom=CGI->townh->towns[s.castle].bonus;
				break;
			default:
				assert(0);
			}
			return CGP->bonuses->ourImages[pom].bitmap;
		}
	default:
		return NULL;
	}
}

const std::string * OptionsTab::SelectedBox::getText() const
{
	const PlayerSettings &s = SEL->sInfo.playerInfos[player];
	switch(which)
	{
	case TOWN:
		if (s.castle < GameConstants::F_NUMBER  &&  s.castle >= 0)
			return &CGI->townh->towns[s.castle].Name();
		else if (s.castle == -1)
			return &CGI->generaltexth->allTexts[522];
		else if (s.castle == -2)
			return &CGI->generaltexth->allTexts[523];
	case HERO:
		if (s.hero == -1)
			return &CGI->generaltexth->allTexts[522];
		else if (s.hero == -2)
		{
			if(s.heroPortrait >= 0)
			{
				if(s.heroName.length())
					return &s.heroName;
				else
					return &CGI->heroh->heroes[s.heroPortrait]->name;
			}
			else
				return &CGI->generaltexth->allTexts[523];
		}
		else
		{
			//if(s.heroName.length())
			//	return &s.heroName;
			//else
				return &CGI->heroh->heroes[s.hero]->name;
		}
	case BONUS:
		switch (s.bonus)
		{
		case -1:
			return &CGI->generaltexth->allTexts[522];
		default:
			return &CGI->generaltexth->arraytxt[214 + s.bonus];
		}
	default:
		return NULL;
	}
}

void OptionsTab::SelectedBox::clickRight( tribool down, bool previousState )
{
	if(indeterminate(down) || !down) return;
	const PlayerSettings &s = SEL->sInfo.playerInfos[player];
	SDL_Surface *bmp = NULL;
	const std::string *title = NULL, *subTitle = NULL;

	subTitle = getText();

	int val=-1;
	switch(which)
	{
	case TOWN:
		val = s.castle;
		break;
	case HERO:
		val = s.hero;
		if(val == -2) //none => we may have some preset info
		{
			int p9 = SEL->current->mapHeader->players[s.color].p9;
			if(p9 != 255  &&  SEL->sInfo.playerInfos[player].heroPortrait >= 0)
				val = p9;
		}
		break;
	case BONUS:
		val = s.bonus;
		break;
	}

	if(val == -1  ||  which == BONUS) //random or bonus box
	{
		bmp = CMessage::drawDialogBox(256, 190);
		std::string *description = NULL;

		switch(which)
		{
		case TOWN:
			title = &CGI->generaltexth->allTexts[103];
			description = &CGI->generaltexth->allTexts[104];
			break;
		case HERO:
			title = &CGI->generaltexth->allTexts[101];
			description = &CGI->generaltexth->allTexts[102];
			break;
		case BONUS:
			{
				switch(val)
				{
				case PlayerSettings::brandom:
					title = &CGI->generaltexth->allTexts[86]; //{Random Bonus}
					description = &CGI->generaltexth->allTexts[94]; //Gold, wood and ore, or an artifact is randomly chosen as your starting bonus
					break;
				case PlayerSettings::bartifact:
					title = &CGI->generaltexth->allTexts[83]; //{Artifact Bonus}
					description = &CGI->generaltexth->allTexts[90]; //An artifact is randomly chosen and equipped to your starting hero
					break;
				case PlayerSettings::bgold:
					title = &CGI->generaltexth->allTexts[84]; //{Gold Bonus}
					subTitle = &CGI->generaltexth->allTexts[87]; //500-1000
					description = &CGI->generaltexth->allTexts[92]; //At the start of the game, 500-1000 gold is added to your Kingdom's resource pool
					break;
				case PlayerSettings::bresource:
					{
						title = &CGI->generaltexth->allTexts[85]; //{Resource Bonus}
						switch(CGI->townh->towns[s.castle].primaryRes)
						{
						case 1:
							subTitle = &CGI->generaltexth->allTexts[694];
							description = &CGI->generaltexth->allTexts[690];
							break;
						case 3:
							subTitle = &CGI->generaltexth->allTexts[695];
							description = &CGI->generaltexth->allTexts[691];
							break;
						case 4:
							subTitle = &CGI->generaltexth->allTexts[692];
							description = &CGI->generaltexth->allTexts[688];
							break;
						case 5:
							subTitle = &CGI->generaltexth->allTexts[693];
							description = &CGI->generaltexth->allTexts[689];
							break;
						case 127:
							subTitle = &CGI->generaltexth->allTexts[89]; //5-10 wood / 5-10 ore
							description = &CGI->generaltexth->allTexts[93]; //At the start of the game, 5-10 wood and 5-10 ore are added to your Kingdom's resource pool
							break;
						}
					}
					break;
				}
			}
			break;
		}

		if(description)
			CSDL_Ext::printAtMiddleWB(*description, 125, 145, FONT_SMALL, 37, Colors::Cornsilk, bmp);
	}
	else if(val == -2)
	{
		return;
	}
	else if(which == TOWN)
	{
		bmp = CMessage::drawDialogBox(256, 319);
		title = &CGI->generaltexth->allTexts[80];

		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[79], 135, 137, FONT_MEDIUM, Colors::Jasmine, bmp);

		const CTown &t = CGI->townh->towns[val];
		//print creatures
		int x = 60, y = 159;
		for(int i = 0; i < 7; i++)
		{
			int c = t.creatures[i][0];
			blitAt(graphics->smallImgs[c], x, y, bmp);
			CSDL_Ext::printAtMiddleWB(CGI->creh->creatures[c]->nameSing, x + 16, y + 45, FONT_TINY, 10, Colors::Cornsilk, bmp);

			if(i == 2)
			{
				x = 40;
				y += 76;
			}
			else
			{
				x += 52;
			}
		}

	}
	else if(val >= 0)
	{
		const CHero *h = CGI->heroh->heroes[val];
		bmp = CMessage::drawDialogBox(320, 255);
		title = &CGI->generaltexth->allTexts[77];

		CSDL_Ext::printAtMiddle(*title, 167, 36, FONT_MEDIUM, Colors::Jasmine, bmp);
		CSDL_Ext::printAtMiddle(*subTitle + " - " + h->heroClass->name, 160, 99, FONT_SMALL, Colors::Cornsilk, bmp);

		blitAt(getImg(), 136, 56, bmp);

		//print specialty
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[78], 166, 132, FONT_MEDIUM, Colors::Jasmine, bmp);
		blitAt(graphics->un44->ourImages[val].bitmap, 140, 150, bmp);
		CSDL_Ext::printAtMiddle(CGI->generaltexth->hTxts[val].bonusName, 166, 203, FONT_SMALL, Colors::Cornsilk, bmp);

		GH.pushInt(new CInfoPopup(bmp, true));
		return;
	}

	if(title)
		CSDL_Ext::printAtMiddle(*title, 135, 36, FONT_MEDIUM, Colors::Jasmine, bmp);
	if(subTitle)
		CSDL_Ext::printAtMiddle(*subTitle, 127, 103, FONT_SMALL, Colors::Cornsilk, bmp);

	blitAt(getImg(), 104, 60, bmp);

	GH.pushInt(new CInfoPopup(bmp, true));
}

CScenarioInfo::CScenarioInfo(const CMapHeader *mapHeader, const StartInfo *startInfo)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	for(std::map<int, PlayerSettings>::const_iterator it = startInfo->playerInfos.begin();
		it != startInfo->playerInfos.end(); ++it)
	{
		if(it->second.human)
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

	card->difficulty->select(startInfo->difficulty, 0);
	back = new CAdventureMapButton("", CGI->generaltexth->zelp[105].second, bind(&CGuiHandler::popIntTotally, &GH, this), 584, 535, "SCNRBACK.DEF", SDLK_ESCAPE);
}

CScenarioInfo::~CScenarioInfo()
{
	delete current;
}

bool mapSorter::operator()(const CMapInfo *aaa, const CMapInfo *bbb)
{
	const CMapHeader * a = aaa->mapHeader,
		* b = bbb->mapHeader;
	if(a && b) //if we are sorting scenarios
	{
		switch (sortBy)
		{
		case _format: //by map format (RoE, WoG, etc)
			return (a->version<b->version);
			break;
		case _loscon: //by loss conditions
			return (a->lossCondition.typeOfLossCon<b->lossCondition.typeOfLossCon);
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
			return (a->victoryCondition.condition < b->victoryCondition.condition);
			break;
		case _name: //by name
			return boost::ilexicographical_compare(a->name, b->name);
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
	txt->setTxt(settings["general"]["playerName"].String()); //Player

	btns[0] = new CAdventureMapButton(CGI->generaltexth->zelp[266], bind(&CMultiMode::openHotseat, this), 373, 78, "MUBHOT.DEF");
	btns[1] = new CAdventureMapButton("Host TCP/IP game", "", bind(&CMultiMode::hostTCP, this), 373, 78 + 57*1, "MUBHOST.DEF");
	btns[2] = new CAdventureMapButton("Join TCP/IP game", "", bind(&CMultiMode::joinTCP, this), 373, 78 + 57*2, "MUBJOIN.DEF");
	btns[6] = new CAdventureMapButton(CGI->generaltexth->zelp[288], bind(&CGuiHandler::popIntTotally, ref(GH), this), 373, 424, "MUBCANC.DEF", SDLK_ESCAPE);
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
	GH.popIntTotally(this);
	GH.pushInt(new CSelectionScreen(CMenuScreen::newGame, CMenuScreen::MULTI_NETWORK_GUEST));
}

CHotSeatPlayers::CHotSeatPlayers(const std::string &firstPlayer)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	bg = new CPicture("MUHOTSEA.bmp");
	pos = bg->center(); //center, window has size of bg graphic

	std::string text = CGI->generaltexth->allTexts[446];
	boost::replace_all(text, "\t","\n");
	Rect boxRect(25, 20, 315, 50);
	title = new CTextBox(text, boxRect, 0, FONT_BIG, CENTER, Colors::Cornsilk);//HOTSEAT	Please enter names

	for(int i = 0; i < ARRAY_COUNT(txt); i++)
	{
		txt[i] = new CTextInput(Rect(60, 85 + i*30, 280, 16), *bg);
		txt[i]->cb += boost::bind(&CHotSeatPlayers::onChange, this, _1);
	}

	ok = new CAdventureMapButton(CGI->generaltexth->zelp[560], bind(&CHotSeatPlayers::enterSelectionScreen, this), 95, 338, "MUBCHCK.DEF", SDLK_RETURN);
	cancel = new CAdventureMapButton(CGI->generaltexth->zelp[561], bind(&CGuiHandler::popIntTotally, ref(GH), this), 205, 338, "MUBCANC.DEF", SDLK_ESCAPE);
	bar = new CGStatusBar(new CPicture(Rect(7, 381, 348, 18), 0));//226, 472

	txt[0]->setTxt(firstPlayer, true);
	txt[0]->giveFocus();
}

void CHotSeatPlayers::onChange(std::string newText)
{
	size_t namesCount = 0;

	for(int i = 0; i < ARRAY_COUNT(txt); i++)
		if(!txt[i]->text.empty())
			namesCount++;

	ok->block(namesCount < 2);
}

void CHotSeatPlayers::enterSelectionScreen()
{
	std::map<ui32, std::string> names;
	for(int i = 0, j = 1; i < ARRAY_COUNT(txt); i++)
		if(txt[i]->text.length())
			names[j++] = txt[i]->text;

	Settings name = settings.write["general"]["playerName"];
	name->String() = names.begin()->second;

	GH.popInts(2); //pop MP mode window and this
	GH.pushInt(new CSelectionScreen(CMenuScreen::newGame, CMenuScreen::MULTI_HOT_SEAT, &names));
}

CBonusSelection::CBonusSelection( CCampaignState * _ourCampaign )
: highlightedRegion(NULL), ourCampaign(_ourCampaign), ourHeader(NULL),
	 diffLb(NULL), diffRb(NULL), bonuses(NULL)
{
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

	startB = new CAdventureMapButton("", "", bind(&CBonusSelection::startMap, this), 475, 536, "CBBEGIB.DEF", SDLK_RETURN);
	backB = new CAdventureMapButton("", "", bind(&CBonusSelection::goBack, this), 624, 536, "CBCANCB.DEF", SDLK_ESCAPE);
	startB->setState(CButtonBase::BLOCKED);

	//campaign name
	if (ourCampaign->camp->header.name.length())
		CSDL_Ext::printAt(ourCampaign->camp->header.name, 481, 28, FONT_BIG, Colors::Jasmine, background);
	else
		CSDL_Ext::printAt("Unnamed", 481, 28, FONT_BIG, Colors::Jasmine, background);

	//map size icon
	sizes = CDefHandler::giveDef("SCNRMPSZ.DEF");


	//campaign description
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[38], 481, 63, FONT_SMALL, Colors::Jasmine, background);

	cmpgDesc = new CTextBox(ourCampaign->camp->header.description, Rect(480, 86, 286, 117), 1);
	//cmpgDesc->showAll(background);

	//map description
	mapDesc = new CTextBox("", Rect(480, 280, 286, 117), 1);

	//bonus choosing
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[71], 511, 432, FONT_MEDIUM, Colors::Cornsilk, background); //Choose a bonus:
	bonuses = new CHighlightableButtonsGroup(bind(&CBonusSelection::selectBonus, this, _1));

	//set left part of window
	for (int g=0; g<ourCampaign->camp->scenarios.size(); ++g)
	{
		if(ourCampaign->camp->conquerable(g))
		{
			regions.push_back(new CRegion(this, true, true, g));
			regions[regions.size()-1]->rclickText = ourCampaign->camp->scenarios[g].regionText;
			if (highlightedRegion == NULL)
			{
				highlightedRegion = regions.back();
				selectMap(g);
			}
		}
		else if (ourCampaign->camp->scenarios[g].conquered) //display as striped
		{
			regions.push_back(new CRegion(this, false, false, g));
			regions[regions.size()-1]->rclickText = ourCampaign->camp->scenarios[g].regionText;
		}
	}

	//init campaign state if necessary
	if (ourCampaign->campaignName.size() == 0)
	{
		ourCampaign->initNewCampaign(sInfo);
	}

	//allies / enemies
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[390] + ":", 486, 407, FONT_SMALL, Colors::Cornsilk, background); //Allies
	CSDL_Ext::printAt(CGI->generaltexth->allTexts[391] + ":", 619, 407, FONT_SMALL, Colors::Cornsilk, background); //Enemies

	SDL_FreeSurface(panel);

	//difficulty
	std::vector<std::string> difficulty;
	boost::split(difficulty, CGI->generaltexth->allTexts[492], boost::is_any_of(" "));
	CSDL_Ext::printAt(difficulty.back(), 689, 432, FONT_MEDIUM, Colors::Cornsilk, background); //Difficulty

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
		diffLb = new CAdventureMapButton("", "", bind(&CBonusSelection::changeDiff, this, false), 694, 508, "SCNRBLF.DEF");
		diffRb = new CAdventureMapButton("", "", bind(&CBonusSelection::changeDiff, this, true), 738, 508, "SCNRBRT.DEF");
	}

	//load miniflags
	sFlags = CDefHandler::giveDef("ITGFLAGS.DEF");

}

CBonusSelection::~CBonusSelection()
{
	SDL_FreeSurface(background);
	delete sizes;
	delete ourHeader;
	delete sFlags;
	for (int b=0; b<ARRAY_COUNT(diffPics); ++b)
	{
		SDL_FreeSurface(diffPics[b]);
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
		CMessage::drawBorder(1, to, pos.w+28, pos.h+30, pos.x-14, pos.y-15);
}

void CBonusSelection::loadPositionsOfGraphics()
{
	const JsonNode config(ResourceID("config/campaign_regions.json"));
	int idx = 0;

	BOOST_FOREACH(const JsonNode &campaign, config["campaign_regions"].Vector())
	{
		SCampPositions sc;

		sc.campPrefix = campaign["prefix"].String();
		sc.colorSuffixLength = campaign["color_suffix_length"].Float();

		BOOST_FOREACH(const JsonNode &desc,  campaign["desc"].Vector())
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

void CBonusSelection::selectMap( int whichOne )
{
	sInfo.difficulty = ourCampaign->camp->scenarios[whichOne].difficulty;
	sInfo.mapname = ourCampaign->camp->header.filename;
	sInfo.mode = StartInfo::CAMPAIGN;

	//get header
	int i = 0;
	delete ourHeader;
	ourHeader = new CMapHeader();
	ourHeader->initFromMemory((const unsigned char*)ourCampaign->camp->mapPieces.find(whichOne)->second.data(), i);

	std::map<ui32, std::string> names;
	names[1] = settings["general"]["playerName"].String();
	updateStartInfo(ourCampaign->camp->header.filename, sInfo, ourHeader, names);
	sInfo.turnTime = 0;
	sInfo.whichMapInCampaign = whichOne;
	sInfo.difficulty = ourCampaign->camp->scenarios[whichOne].difficulty;

	ourCampaign->currentMap = whichOne;

	mapDesc->setTxt(ourHeader->description);

	updateBonusSelection();
}

void CBonusSelection::show(SDL_Surface * to)
{
	//blitAt(background, pos.x, pos.y, to);

	//map name
	std::string mapName = ourHeader->name;

	if (mapName.length())
		printAtLoc(mapName, 481, 219, FONT_BIG, Colors::Jasmine, to);
	else
		printAtLoc("Unnamed", 481, 219, FONT_BIG, Colors::Jasmine, to);

	//map description
	printAtLoc(CGI->generaltexth->allTexts[496], 481, 253, FONT_SMALL, Colors::Jasmine, to);

	mapDesc->showAll(to); //showAll because CTextBox has no show()

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
	int fx = 496  + graphics->fonts[FONT_SMALL]->getWidth(CGI->generaltexth->allTexts[390].c_str());
	int ex = 629 + graphics->fonts[FONT_SMALL]->getWidth(CGI->generaltexth->allTexts[391].c_str());
	int myT;
	myT = ourHeader->players[playerColor].team;
	for (std::map<int, PlayerSettings>::const_iterator i = sInfo.playerInfos.begin(); i != sInfo.playerInfos.end(); i++)
	{
		int *myx = ((i->first == playerColor  ||  ourHeader->players[i->first].team == myT) ? &fx : &ex);
		blitAtLoc(sFlags->ourImages[i->first].bitmap, pos.x + *myx, pos.y + 405, to);
		*myx += sFlags->ourImages[i->first].bitmap->w;
	}

	//difficulty
	blitAtLoc(diffPics[sInfo.difficulty], 709, 455, to);

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
	const CCampaignScenario &scenario = ourCampaign->camp->scenarios[sInfo.whichMapInCampaign];
	const std::vector<CScenarioTravel::STravelBonus> & bonDescs = scenario.travelOptions.bonusesToChoose;

	for (size_t i=0; i<bonuses->buttons.size(); i++)
	{
		if (bonuses->buttons[i]->active)
			bonuses->buttons[i]->deactivate();
		delete bonuses->buttons[i];
	}
	bonuses->buttons.clear();

		static const char *bonusPics[] = {"SPELLBON.DEF", "TWCRPORT.DEF", "", "ARTIFBON.DEF", "SPELLBON.DEF",
			"PSKILBON.DEF", "SSKILBON.DEF", "BORES.DEF", "CREST58.DEF", "PORTRAITSLARGE"};

		for(int i = 0; i < bonDescs.size(); i++)
		{
			std::string picName=bonusPics[bonDescs[i].type];
			size_t picNumber=bonDescs[i].info2;

			std::string desc;
			switch(bonDescs[i].type)
			{
			case 0: //spell
				desc = CGI->generaltexth->allTexts[715];
				boost::algorithm::replace_first(desc, "%s", CGI->spellh->spells[bonDescs[i].info2]->name);
				break;
			case 1: //monster
				picNumber = bonDescs[i].info2 + 2;
				desc = CGI->generaltexth->allTexts[717];
				boost::algorithm::replace_first(desc, "%d", boost::lexical_cast<std::string>(bonDescs[i].info3));
				boost::algorithm::replace_first(desc, "%s", CGI->creh->creatures[bonDescs[i].info2]->namePl);
				break;
			case 2: //building
				{
					int faction = -1;
					for(std::map<int, PlayerSettings>::iterator it = sInfo.playerInfos.begin();
						it != sInfo.playerInfos.end(); ++it)
					{
						if (it->second.human)
						{
							faction = it->second.castle;
							break;
						}

					}
					assert(faction != -1);

					int buildID = CBuildingHandler::campToERMU(bonDescs[i].info1, faction, std::set<si32>());
					picName = graphics->ERMUtoPicture[faction][buildID];
					picNumber = -1;

					tlog1<<CGI->buildh->buildings.size()<<"\t"<<faction<<"\t"<<CGI->buildh->buildings[faction].size()<<"\t"<<buildID<<"\n";
					if (vstd::contains(CGI->buildh->buildings[faction], buildID))
						desc = CGI->buildh->buildings[faction].find(buildID)->second->Description();
				}
				break;
			case 3: //artifact
				desc = CGI->generaltexth->allTexts[715];
				boost::algorithm::replace_first(desc, "%s", CGI->arth->artifacts[bonDescs[i].info2]->Name());
				break;
			case 4: //spell scroll
				desc = CGI->generaltexth->allTexts[716];
				boost::algorithm::replace_first(desc, "%s", CGI->spellh->spells[bonDescs[i].info2]->name);
				break;
			case 5: //primary skill
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
			case 6: //secondary skill
				desc = CGI->generaltexth->allTexts[718];

				boost::algorithm::replace_first(desc, "%s", CGI->generaltexth->levels[bonDescs[i].info3]); //skill level
				boost::algorithm::replace_first(desc, "%s", CGI->generaltexth->skillName[bonDescs[i].info2]); //skill name

				break;
			case 7: //resource
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
			case 8: //player aka hero crossover
				picNumber = bonDescs[i].info1;
				desc = CGI->generaltexth->allTexts[718];

				boost::algorithm::replace_first(desc, "%s", CGI->generaltexth->capColors[bonDescs[i].info1]); //player color
				boost::algorithm::replace_first(desc, "%s", ourCampaign->camp->scenarios[bonDescs[i].info2].mapName); //scenario

				break;
			case 9: //hero

				desc = CGI->generaltexth->allTexts[718];
				boost::algorithm::replace_first(desc, "%s", CGI->generaltexth->capColors[bonDescs[i].info1]); //hero's color

				if (bonDescs[i].info2 == 0xFFFF)
				{
					boost::algorithm::replace_first(desc, "%s", CGI->heroh->heroes[0]->name); //hero's name
					picNumber = 0;
				}
				else
				{
					boost::algorithm::replace_first(desc, "%s", CGI->heroh->heroes[bonDescs[i].info2]->name); //hero's name
				}
				break;
			}

			CHighlightableButton *bonusButton = new CHighlightableButton(desc, desc, 0, 475 + i*68, 455, "", i);

			if (picNumber != -1)
				picName += ":" + boost::lexical_cast<std::string>(picNumber);

			CAnimation * anim = new CAnimation();
			anim->setCustom(picName, 0);
			bonusButton->setImage(anim);
			bonusButton->borderColor = Colors::Maize; // yellow border
			bonuses->addButton(bonusButton);
		}
}

void CBonusSelection::startMap()
{
	StartInfo *si = new StartInfo(sInfo);
	if (ourCampaign->mapsConquered.size())
	{
		GH.popInts(1);
	}
	else
	{
		CGP->removeFromGui();
	}
	::startGame(si);
}

void CBonusSelection::selectBonus( int id )
{
	// Total redraw is needed because the border around the bonus images
	// have to be undrawn/drawn.
	if (id != sInfo.choosenCampaignBonus)
	{
		sInfo.choosenCampaignBonus = id;
		GH.totalRedraw();

		if (startB->getState() == CButtonBase::BLOCKED)
			startB->setState(CButtonBase::NORMAL);
	}


	const CCampaignScenario &scenario = ourCampaign->camp->scenarios[sInfo.whichMapInCampaign];
	const std::vector<CScenarioTravel::STravelBonus> & bonDescs = scenario.travelOptions.bonusesToChoose;
	if (bonDescs[id].type == 8) //hero crossover
	{
		SEL->setPlayer(sInfo.playerInfos[0], bonDescs[id].info1); //TODO: substitute with appropriate color
	}
}

void CBonusSelection::changeDiff( bool increase )
{
	if (increase)
	{
		sInfo.difficulty = std::min(sInfo.difficulty + 1, 4);
	}
	else
	{
		sInfo.difficulty = std::max(sInfo.difficulty - 1, 0);
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
	for (int g=0; g<ARRAY_COUNT(graphics); ++g)
	{
		SDL_FreeSurface(graphics[g]);
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
		owner->selectMap(myNumber);
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

ISelectionScreenInfo::ISelectionScreenInfo(const std::map<ui32, std::string> *Names /*= NULL*/)
{
	multiPlayer = CMenuScreen::SINGLE_PLAYER;
	assert(!SEL);
	SEL = this;
	current = NULL;

	if(Names && Names->size()) //if have custom set of player names - use it
		playerNames = *Names;
	else
		playerNames[1] = settings["general"]["playerName"].String(); //by default we have only one player and his name is "Player" (or whatever the last used name was)
}

ISelectionScreenInfo::~ISelectionScreenInfo()
{
	assert(SEL == this);
	SEL = NULL;
}

void ISelectionScreenInfo::updateStartInfo(std::string filename, StartInfo & sInfo, const CMapHeader * mapHeader)
{
	::updateStartInfo(filename, sInfo, mapHeader, playerNames);
}

void ISelectionScreenInfo::setPlayer(PlayerSettings &pset, unsigned player)
{
	::setPlayer(pset, player, playerNames);
}

int ISelectionScreenInfo::getIdOfFirstUnallocatedPlayer()
{
	for(std::map<ui32, std::string>::const_iterator i = playerNames.begin(); i != playerNames.end(); i++)
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
	for(std::map<int, PlayerSettings>::iterator i = SEL->sInfo.playerInfos.begin(); i != SEL->sInfo.playerInfos.end(); i++)
	{
		if(!i->second.human)
		{
			selScreen->setPlayer(i->second, connectionID);
			selScreen->opt->entries[i->second.color]->selectButtons();
			break;
		}
	}

	selScreen->propagateNames();
	selScreen->propagateOptions();

	GH.totalRedraw();
}

void SelectMap::apply(CSelectionScreen *selScreen)
{
	if(selScreen->multiPlayer == CMenuScreen::MULTI_NETWORK_GUEST)
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
	}
}

void RequestOptionsChange::apply(CSelectionScreen *selScreen)
{
	if(!selScreen->isHost())
		return;

	ui8 color = selScreen->sInfo.getPlayersSettings(playerID)->color;

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

	selScreen->serv = NULL; //hide it so it won't be deleted
	vstd::clear_pointer(selScreen->serverHandlingThread); //detach us
	selectedName = selScreen->sInfo.mapname;

	CGP->removeFromGui();

	::startGame(startingInfo.sInfo, startingInfo.serv);
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

//On linux we can only play *.mjpg videos from LOKI release
#ifdef _WIN32
	std::transform(video.begin(), video.end(), video.begin(), toupper);
	video += ".BIK";
#else
	std::transform(video.begin(), video.end(), video.begin(), tolower);
	video += ".mjpg";
#endif

	OBJ_CONSTRUCTION_CAPTURING_ALL;

	status = config["open"].Bool() ? CCampaignScreen::ENABLED : CCampaignScreen::DISABLED;

	CCampaignHeader header = CCampaignHandler::getHeader(campFile);
	hoverText = header.name;

	if (status != CCampaignScreen::DISABLED)
	{
		addUsedEvents(LCLICK | HOVER);
		image = new CPicture(config["image"].String());

		hoverLabel = new CLabel(pos.w / 2, pos.h + 20, FONT_MEDIUM, CENTER, Colors::Jasmine, "");
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

		CCampaign * ourCampaign = CCampaignHandler::getCampaign(campFile);
		CCampaignState * campState = new CCampaignState();
		campState->camp = ourCampaign;
		GH.pushInt( new CBonusSelection(campState) );
	}
}

void CCampaignScreen::CCampaignButton::hover(bool on)
{
	if (on)
		hoverLabel->setTxt(hoverText); // Shows the name of the campaign when you get into the bounds of the button
	else
		hoverLabel->setTxt(" ");
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

CAdventureMapButton* CCampaignScreen::createExitButton(const JsonNode& button)
{
	std::pair<std::string, std::string> help;
	if (!button["help"].isNull() && button["help"].Float() > 0)
		help = CGI->generaltexth->zelp[button["help"].Float()];

	boost::function<void()> close = boost::bind(&CGuiHandler::popIntTotally, &GH, this);
	return new CAdventureMapButton(help, close, button["x"].Float(), button["y"].Float(), button["name"].String(), button["hotkey"].Float());
}


CCampaignScreen::CCampaignScreen(const JsonNode &config)
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	BOOST_FOREACH(const JsonNode& node, config["images"].Vector())
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
	
	BOOST_FOREACH(const JsonNode& node, config["items"].Vector())
		campButtons.push_back(new CCampaignButton(node));
}

void CCampaignScreen::showAll(SDL_Surface *to)
{
	CIntObject::showAll(to);
	if (pos.h != to->h || pos.w != to->w)
		CMessage::drawBorder(1, to, pos.w+28, pos.h+30, pos.x-14, pos.y-15);
}
