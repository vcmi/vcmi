#include "../stdafx.h"
#include "CPreGame.h"
#include "../hch/CDefHandler.h"
#include <ctime>
#include <boost/filesystem.hpp>   // includes all needed Boost.Filesystem declarations
#include <boost/algorithm/string.hpp>
#include <zlib.h>
#include "../timeHandler.h"
#include <sstream>
#include "SDL_Extensions.h"
#include "CGameInfo.h"
#include "../hch/CGeneralTextHandler.h"
#include "CCursorHandler.h"
#include "../hch/CLodHandler.h"
#include "../hch/CTownHandler.h"
#include "../hch/CHeroHandler.h"
#include <cmath>
#include "Graphics.h"
//#include <boost/thread.hpp>
#include <boost/bind.hpp>
#include <cstdlib>
#include "../lib/Connection.h"
#include "../lib/VCMIDirs.h"
#include "../hch/CMusicHandler.h"
#include "../hch/CVideoHandler.h"
#include "AdventureMapButton.h"
#include "GUIClasses.h"
#include "../hch/CCreatureHandler.h"
#include "CPlayerInterface.h"
#include "../CCallback.h"
#include <boost/lexical_cast.hpp>
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
void startGame(StartInfo * options);

CGPreGame * CGP;
static const CMapHeader *curMap;
static StartInfo *curOpts;
static int playerColor, playerSerial;

static std::string selectedName; //set when game is started/loaded

static void do_quit()
{
	SDL_Event event;
	event.quit.type = SDL_QUIT;
	SDL_PushEvent(&event);
}

CMenuScreen::CMenuScreen( EState which )
{
	OBJ_CONSTRUCTION;
	bgAd = NULL;

	switch(which)
	{
	case mainMenu:
		{
			buttons[0] = new AdventureMapButton("", CGI->generaltexth->zelp[3].second, bind(&CMenuScreen::moveTo, this, ref(CGP->scrs[newGame])), 540, 10, "ZMENUNG.DEF", SDLK_n);
			buttons[1] = new AdventureMapButton("", CGI->generaltexth->zelp[4].second, bind(&CMenuScreen::moveTo, this, ref(CGP->scrs[loadGame])), 532, 132, "ZMENULG.DEF", SDLK_l);
			buttons[2] = new AdventureMapButton("", CGI->generaltexth->zelp[5].second, 0 /*cb*/, 524, 251, "ZMENUHS.DEF", SDLK_h);
			buttons[3] = new AdventureMapButton("", CGI->generaltexth->zelp[6].second, 0 /*cb*/, 557, 359, "ZMENUCR.DEF", SDLK_c);
			boost::function<void()> confWindow = bind(CInfoWindow::showYesNoDialog, ref(CGI->generaltexth->allTexts[69]), (const std::vector<SComponent*>*)0, do_quit, 0, false, 1);
			buttons[4] = new AdventureMapButton("", CGI->generaltexth->zelp[7].second, confWindow, 586, 468, "ZMENUQT.DEF", SDLK_ESCAPE);
		}
		break;
	case newGame:
		{
			bgAd = new CPicture(BitmapHandler::loadBitmap("ZNEWGAM.bmp"), 114, 312, true);
			buttons[0] = new AdventureMapButton("", CGI->generaltexth->zelp[10].second, bind(&CGPreGame::openSel, CGP, newGame), 545, 4, "ZTSINGL.DEF", SDLK_s);
			buttons[1] = new AdventureMapButton("", CGI->generaltexth->zelp[11].second, 0 /*cb*/, 568, 120, "ZTMULTI.DEF", SDLK_m);
			buttons[2] = new AdventureMapButton("", CGI->generaltexth->zelp[12].second, 0 /*cb*/, 541, 233, "ZTCAMPN.DEF", SDLK_c);
			buttons[3] = new AdventureMapButton("", CGI->generaltexth->zelp[13].second, 0 /*cb*/, 545, 358, "ZTTUTOR.DEF", SDLK_t);
			buttons[4] = new AdventureMapButton("", CGI->generaltexth->zelp[14].second, bind(&CMenuScreen::moveTo, this, CGP->scrs[mainMenu]), 582, 464, "ZTBACK.DEF", SDLK_ESCAPE);
		}
		break;
	case loadGame:
		{
			bgAd = new CPicture(BitmapHandler::loadBitmap("ZLOADGAM.bmp"), 114, 312, true);
			buttons[0] = new AdventureMapButton("", CGI->generaltexth->zelp[10].second, bind(&CGPreGame::openSel, CGP, loadGame), 545, 4, "ZTSINGL.DEF", SDLK_s);
			buttons[1] = new AdventureMapButton("", CGI->generaltexth->zelp[11].second, 0 /*cb*/, 568, 120, "ZTMULTI.DEF", SDLK_m);
			buttons[2] = new AdventureMapButton("", CGI->generaltexth->zelp[12].second, 0 /*cb*/, 541, 233, "ZTCAMPN.DEF", SDLK_c);
			buttons[3] = new AdventureMapButton("", CGI->generaltexth->zelp[13].second, 0 /*cb*/, 545, 358, "ZTTUTOR.DEF", SDLK_t);
			buttons[4] = new AdventureMapButton("", CGI->generaltexth->zelp[14].second, bind(&CMenuScreen::moveTo, this, CGP->scrs[mainMenu]), 582, 464, "ZTBACK.DEF", SDLK_ESCAPE);
		}
		break;
	}

	for(int i = 0; i < ARRAY_COUNT(buttons); i++)
		buttons[i]->hoverable = true;
}

CMenuScreen::~CMenuScreen()
{
}

void CMenuScreen::showAll( SDL_Surface * to )
{
	blitAt(CGP->mainbg, 0, 0, to);
	CIntObject::showAll(to);
}

void CMenuScreen::show( SDL_Surface * to )
{
	CIntObject::show(to);
	CGI->videoh->update(pos.x + 8, pos.y + 105, screen, true, false);
}

void CMenuScreen::moveTo( CMenuScreen *next )
{
	GH.popInt(this);
	GH.pushInt(next);
}

CGPreGame::CGPreGame()
{
	GH.defActionsDef = 63;
	CGP = this;
	mainbg = BitmapHandler::loadBitmap("ZPIC1005.bmp");

	for(int i = 0; i < ARRAY_COUNT(scrs); i++)
		scrs[i] = new CMenuScreen((EState)i);
}

CGPreGame::~CGPreGame()
{
	SDL_FreeSurface(mainbg);

	for(int i = 0; i < ARRAY_COUNT(scrs); i++)
		delete scrs[i];
}

void CGPreGame::openSel( EState type )
{
	GH.pushInt(new CSelectionScreen(type));
}

void CGPreGame::loadGraphics()
{
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
	if (GH.listInt.size() == 0)
	{
	#ifdef _WIN32
		CGI->videoh->open("ACREDIT.SMK");
	#else
		CGI->videoh->open("ACREDIT.SMK", true, false);
	#endif
		GH.pushInt(scrs[mainMenu]);
	}

	CGI->curh->draw1();
	SDL_Flip(screen);
	CGI->curh->draw2();
	GH.topInt()->show(screen);
	GH.updateTime();
	GH.handleEvents();
}

CSelectionScreen::CSelectionScreen( EState Type )
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;
	IShowActivable::type = BLOCK_ADV_HOTKEYS;
	pos.w = 762;
	pos.h = 584;
	if(Type == saveGame)
	{
		center(pos);
	}
	else
	{
		pos.x = 3;
		pos.y = 6;
		bg = new CPicture(BitmapHandler::loadBitmap(rand()%2 ? "ZPIC1000.bmp" : "ZPIC1001.bmp"), -3, -6, true);
	}

	CGP->loadGraphics();
	type = Type;
	curOpts = &sInfo;
	sInfo.difficulty = 1;
	current = NULL;

	sInfo.mode = (Type == newGame ? 0 : 1);
	sInfo.turnTime = 0;
	curTab = NULL;

	card = new InfoCard(type);
	opt = new OptionsTab(type/*, sInfo*/);
	opt->recActions = DISPOSE;
	sel = new SelectionTab(type, bind(&CSelectionScreen::changeSelection, this, _1));
	sel->recActions = DISPOSE;

	switch(type)
	{
	case newGame:
		{
			card->difficulty->onChange = bind(&CSelectionScreen::difficultyChange, this, _1);
			card->difficulty->select(1, 0);
			AdventureMapButton *select = new AdventureMapButton(CGI->generaltexth->zelp[45], bind(&CSelectionScreen::toggleTab, this, sel), 411, 75, "GSPBUTT.DEF", SDLK_s);
			select->addTextOverlay(CGI->generaltexth->allTexts[500], FONT_SMALL);

			AdventureMapButton *opts = new AdventureMapButton(CGI->generaltexth->zelp[46], bind(&CSelectionScreen::toggleTab, this, opt), 411, 503, "GSPBUTT.DEF", SDLK_a);
			opts->addTextOverlay(CGI->generaltexth->allTexts[501], FONT_SMALL);

			AdventureMapButton *random = new AdventureMapButton(CGI->generaltexth->zelp[47], bind(&CSelectionScreen::toggleTab, this, sel), 411, 99, "GSPBUTT.DEF", SDLK_r);
			random->addTextOverlay(CGI->generaltexth->allTexts[740], FONT_SMALL);

			start  = new AdventureMapButton(CGI->generaltexth->zelp[103], bind(&CSelectionScreen::startGame, this), 411, 529, "SCNRBEG.DEF", SDLK_b);
		}
		break;
	case loadGame:
		sel->recActions = 255;
		start  = new AdventureMapButton(CGI->generaltexth->zelp[103], bind(&CSelectionScreen::startGame, this), 411, 529, "SCNRLOD.DEF", SDLK_l);
		break;
	case saveGame:
		sel->recActions = 255;
		start  = new AdventureMapButton("", CGI->generaltexth->zelp[103].second, bind(&CSelectionScreen::startGame, this), 411, 529, "SCNRSAV.DEF");
		break;
	}

	start->assignedKeys.insert(SDLK_RETURN);

	back = new AdventureMapButton("", CGI->generaltexth->zelp[105].second, bind(&CGuiHandler::popIntTotally, &GH, this), 581, 529, "SCNRBACK.DEF", SDLK_ESCAPE);
}

CSelectionScreen::~CSelectionScreen()
{
	curMap = NULL;
	curOpts = NULL;
	playerSerial = playerColor = -1;
}

void CSelectionScreen::toggleTab(CIntObject *tab)
{
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
	curMap = current = to;
	if(to && type == loadGame)
		curOpts->difficulty = to->seldiff;
	updateStartInfo(to);
	card->changeSelection(to);
	opt->changeSelection(to);
}

void CSelectionScreen::updateStartInfo( const CMapInfo * to )
{
	sInfo.playerInfos.clear();
	if(!to) 
		return;

	sInfo.playerInfos.resize(to->playerAmnt);
	sInfo.mapname = to->filename;
	playerSerial = playerColor = -1;

	int serialC=0;
	for (int i = 0; i < PLAYER_LIMIT; i++)
	{
		const PlayerInfo &pinfo = to->players[i];

		//neither computer nor human can play - no player
		if (!(pinfo.canComputerPlay || pinfo.canComputerPlay))
			continue;

		PlayerSettings &pset = sInfo.playerInfos[serialC];
		pset.color = i;
		pset.serial = serialC++;

		if (pinfo.canHumanPlay && playerColor < 0)
		{
			pset.name = CGI->generaltexth->allTexts[434]; //Player
			pset.human = true;
			playerColor = i;
			playerSerial = pset.serial;
		}
		else
		{
			pset.name = CGI->generaltexth->allTexts[468];//Computer
			pset.human = false;
		}

		for (int j = 0; j < F_NUMBER  &&  pset.castle != -1; j++) //we start with none and find matching faction. if more than one, then set to random
		{
			if((1 << j) & pinfo.allowedFactions)
			{
				if (pset.castle >= 0) //we've already assigned a castle and another one is possible -> set random and let player choose
					pset.castle = -1; //breaks

				if (pset.castle == -2) //first available castle - pick
					pset.castle = j;
			}
		}

		if ((pinfo.generateHeroAtMainTown || to->version==CMapHeader::RoE)  &&  pinfo.hasMainTown  //we will generate hero in front of main town
			|| pinfo.p8) //random hero
			pset.hero = -1;
		else
			pset.hero = -2;

		if(pinfo.mainHeroName.length())
		{
			pset.heroName = pinfo.mainHeroName;
			if((pset.heroPortrait = pinfo.mainHeroPortrait) == 255)
				pset.heroPortrait = pinfo.p9;
		}
		pset.handicap = 0;
	}
}

void CSelectionScreen::startGame()
{
	if(type != saveGame)
	{
		if(!current)
			return;

		selectedName = sInfo.mapname;
		StartInfo *si = new StartInfo(sInfo);
		GH.popInt(this);
		GH.popInt(GH.topInt());
		curMap = NULL;
		curOpts = NULL;
		::startGame(si);
	}
	else
	{
		if(!(sel && sel->txt && sel->txt->text.size()))
			return;

		selectedName = GVCMIDirs.UserPath + "/Games/" + sel->txt->text + ".vlgm1";
		LOCPLINT->cb->save(sel->txt->text);
		GH.popInt(this);
	}
}

void CSelectionScreen::difficultyChange( int to )
{
	assert(type == newGame);
	sInfo.difficulty = to;
	GH.totalRedraw();
}

// A new size filter (Small, Medium, ...) has been selected. Populate
// selMaps with the relevant data.
void SelectionTab::filter( int size, bool selectFirst )
{
	curItems.clear();

	for (size_t i=0; i<allItems.size(); i++) 
		if( allItems[i].version  &&  (!size || allItems[i].width == size))
			curItems.push_back(&allItems[i]);

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


void SelectionTab::getFiles(std::vector<FileInfo> &out, const std::string &dirname, const std::string &ext)
{
	if(!boost::filesystem::exists(dirname))
	{
		tlog1 << "Cannot find " << dirname << " directory!\n";
	}
	fs::path tie(dirname);
	fs::directory_iterator end_iter;
	for ( fs::directory_iterator file (tie); file!=end_iter; ++file )
	{
		if(fs::is_regular_file(file->status())
			&& boost::ends_with(file->path().filename(), ext))
		{
			out.resize(out.size()+1);
			out.back().date = fs::last_write_time(file->path());
			out.back().name = file->path().string();
		}
	}

	allItems.resize(out.size());
}

void SelectionTab::parseMaps(std::vector<FileInfo> &files, int start, int threads)
{
	int read=0;
	unsigned char mapBuffer[1500];

	while(start < allItems.size())
	{
		gzFile tempf = gzopen(files[start].name.c_str(),"rb");
		read = gzread(tempf, mapBuffer, 1500);
		gzclose(tempf);
		if(read < 50  ||  !mapBuffer[4])
		{
			tlog3 << "\t\tWarning: corrupted map file: " << files[start].name << std::endl; 
		}
		else //valid map
		{
			allItems[start].init(files[start].name, mapBuffer);
			//allItems[start].date = "DATEDATE";// files[start].date;
		}
		start += threads;
	}
}

void SelectionTab::parseGames(std::vector<FileInfo> &files)
{
	for(int i=0; i<files.size(); i++)
	{
		CLoadFile lf(files[i].name);
		if(!lf.sfile)
			continue;

		ui8 sign[8]; 
		lf >> sign;
		if(std::memcmp(sign,"VCMISVG",7))
		{
			tlog1 << files[i].name << " is not a correct savefile!" << std::endl;
			continue;
		}
		lf >> static_cast<CMapHeader&>(allItems[i]) >> allItems[i].seldiff;
		allItems[i].filename = files[i].name;
		allItems[i].countPlayers();
		allItems[i].date = std::asctime(std::localtime(&files[i].date));
	}
}

SelectionTab::SelectionTab(EState Type, const boost::function<void(CMapInfo *)> &OnSelect)
	:onSelect(OnSelect)
{
	OBJ_CONSTRUCTION;
	selectionPos = 0;
	used = LCLICK | WHEEL | KEYBOARD | DOUBLECLICK;
	slider = NULL;
	txt = NULL;
	type = Type;

	bg = new CPicture(BitmapHandler::loadBitmap("SCSELBCK.bmp"), 0, 0, true);
	pos.w = bg->pos.w;
	pos.h = bg->pos.h;

	std::vector<FileInfo> toParse;
	switch(type)
	{
	case newGame:
		getFiles(toParse, DATA_DIR "/Maps", "h3m");
		parseMaps(toParse);
		positions = 18;
		break;

	case loadGame:
	case saveGame:
		getFiles(toParse, GVCMIDirs.UserPath + "/Games", "vlgm1");
		parseGames(toParse);
		if(type == loadGame)
		{
			positions = 18;
		}
		else
		{
			positions = 16;
		}	
		if(type == saveGame)
			txt = new CTextInput(Rect(32, 539, 350, 20), Point(-32, -25), "GSSTRIP.bmp", 0);
		break;

	default:
		assert(0);
	}


	//size filter buttons
	{
		int sizes[] = {36, 72, 108, 144, 0};
		const char * names[] = {"SCSMBUT.DEF", "SCMDBUT.DEF", "SCLGBUT.DEF", "SCXLBUT.DEF", "SCALBUT.DEF"};
		for(int i = 0; i < 5; i++)
			new AdventureMapButton("", CGI->generaltexth->zelp[54+i].second, bind(&SelectionTab::filter, this, sizes[i], true), 158 + 47*i, 46, names[i]);
	}

	{
		int xpos[] = {23, 55, 88, 121, 306, 339};
		const char * names[] = {"SCBUTT1.DEF", "SCBUTT2.DEF", "SCBUTCP.DEF", "SCBUTT3.DEF", "SCBUTT4.DEF", "SCBUTT5.DEF"};
		for(int i = 0; i < 6; i++)
			new AdventureMapButton("", CGI->generaltexth->zelp[107+i].second, bind(&SelectionTab::sortBy, this, i), xpos[i], 86, names[i]);
	}

	slider = new CSlider(372, 86, type != saveGame ? 480 : 430, bind(&SelectionTab::sliderMove, this, _1), positions, curItems.size(), 0, false, 1);
	format =  CDefHandler::giveDef("SCSELC.DEF");

	sortingBy = _format;
	ascending = true;
	filter(0);
	//select(0);
	switch(type)
	{
	case newGame:
		selectFName(DATA_DIR "/Maps/Arrogance.h3m");
		break;
	case loadGame:
		select(0);
		break;
	case saveGame:;
		if(selectedName.size())
		{
			if(selectedName[0] == 'M')
				txt->setText("NEWGAME");
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
		std::stable_sort(curItems.begin(),curItems.end(),mapSorter(_name));
	std::stable_sort(curItems.begin(),curItems.end(),mapSorter(sortingBy));

	if(!ascending)
		std::reverse(curItems.begin(), curItems.end());

	redraw();
}

void SelectionTab::select( int position )
{
	if(!curItems.size()) return;

	// New selection. py is the index in curItems.
	int py = position + slider->value;
	amax(py, 0);
	amin(py, curItems.size()-1);

	selectionPos = py;

	if(position < 0)
		slider->moveTo(slider->value + position);
	else if(position >= positions)
		slider->moveTo(slider->value + position - positions + 1);

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


	SDL_Color nasz;
#define POS(xx, yy) pos.x + xx, pos.y + yy + line*25
	for (int line = 0; line < positions  &&  elemIdx < curItems.size(); elemIdx++, line++)
	{
		CMapInfo* curMap = curItems[elemIdx];

		if (elemIdx == selectionPos)
			nasz=tytulowy;
		else 
			nasz=zwykly;

		std::ostringstream ostr(std::ostringstream::out);
		ostr << curMap->playerAmnt << "/" << curMap->humenPlayers;
		CSDL_Ext::printAt(ostr.str(), POS(29, 120), FONT_SMALL, nasz, to);

		std::string temp2 = "C";
		switch (curMap->width)
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
		CSDL_Ext::printAtMiddle(temp2, POS(70, 128), FONT_SMALL, nasz, to);

		int temp=-1;
		switch (curMap->version)
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
			tlog2 << "Warning: " << curMap->filename << " has wrong version!\n";
			continue;
		}
		blitAt(format->ourImages[temp].bitmap, POS(88, 117), to);

		if (type == newGame) {
			if (!curMap->name.length())
				curMap->name = "Unnamed";
			CSDL_Ext::printAtMiddle(curMap->name, POS(213, 128), FONT_SMALL, nasz, to);
		} else
			CSDL_Ext::printAtMiddle(fs::basename(curMap->filename), POS(213, 128), FONT_SMALL, nasz, to);

		if (curMap->victoryCondition.condition == winStandard)
			temp = 11;
		else 
			temp = curMap->victoryCondition.condition;
		blitAt(CGP->victory->ourImages[temp].bitmap, POS(306, 117), to);

		if (curMap->lossCondition.typeOfLossCon == lossStandard)
			temp=3;
		else 
			temp=curMap->lossCondition.typeOfLossCon;
		blitAt(CGP->loss->ourImages[temp].bitmap, POS(339, 117), to);
	}
#undef POS
}

void SelectionTab::showAll( SDL_Surface * to )
{
	CIntObject::showAll(to);
	printMaps(to);

	int txtid;
	switch(type) {
	case newGame: txtid = 229; break;
	case loadGame: txtid = 230; break;
	case saveGame: txtid = 231; break;
	}

	CSDL_Ext::printAtMiddle(CGI->generaltexth->arraytxt[txtid], pos.x+205, pos.y+28, FONT_MEDIUM, tytulowy, to); //Select a Scenario to Play
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[510], pos.x+87, pos.y+62, FONT_SMALL, tytulowy, to); //Map sizes
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

void SelectionTab::wheelScrolled( bool down, bool in )
{
	slider->moveTo(slider->value + 3 * (down ? +1 : -1));
	//select(selectionPos - slider->value + (down ? +1 : -1));
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

void SelectionTab::selectFName( const std::string &fname )
{
	for(int i = curItems.size() - 1; i >= 0; i--)
	{
		if(curItems[i]->filename == fname)
		{
			slider->moveTo(i);
			selectAbs(i);
			return;
		}
	}

	selectAbs(0);
}

InfoCard::InfoCard( EState Type )
{
	OBJ_CONSTRUCTION;
	pos.x += 393;
	used = RCLICK;
	sizes = CDefHandler::giveDef("SCNRMPSZ.DEF");
	sFlags = CDefHandler::giveDef("ITGFLAGS.DEF");
	type = Type;
	bg = new CPicture(BitmapHandler::loadBitmap("GSELPOP1.bmp"), 0, 0, true);
	pos.w = bg->pos.w;
	pos.h = bg->pos.h;

	difficulty = new CHighlightableButtonsGroup(0);
	{
		static const char *difButns[] = {"GSPBUT3.DEF", "GSPBUT4.DEF", "GSPBUT5.DEF", "GSPBUT6.DEF", "GSPBUT7.DEF"};
		BLOCK_CAPTURING;

		for(int i = 0; i < 5; i++)
		{
			difficulty->addButton(new CHighlightableButton("", CGI->generaltexth->zelp[24+i].second, 0, 110 + i*32, 450, difButns[i], i));
			difficulty->buttons.back()->pos += pos.topLeft();
		}
	}

	if(type != newGame)
		difficulty->block(true);
}

InfoCard::~InfoCard()
{
	delete sizes;
	delete sFlags;
}

void InfoCard::showAll( SDL_Surface * to )
{
	CIntObject::showAll(to);

	//blit texts
	printAtLoc(CGI->generaltexth->allTexts[390] + ":", 24, 400, FONT_SMALL, zwykly, to); //Allies
	printAtLoc(CGI->generaltexth->allTexts[391] + ":", 190, 400, FONT_SMALL, zwykly, to); //Enemies
	printAtLoc(CGI->generaltexth->allTexts[494], 33, 430, FONT_SMALL, tytulowy, to);//"Map Diff:"
	printAtLoc(CGI->generaltexth->allTexts[492] + ":", 133,430, FONT_SMALL, tytulowy, to); //player difficulty
	printAtLoc(CGI->generaltexth->allTexts[218] + ":", 290,430, FONT_SMALL, tytulowy, to); //"Rating:"
	printAtLoc(CGI->generaltexth->allTexts[495], 26, 22, FONT_SMALL, tytulowy, to); //Scenario Name:
	printAtLoc(CGI->generaltexth->allTexts[496], 26, 132, FONT_SMALL, tytulowy, to); //Scenario Description:
	printAtLoc(CGI->generaltexth->allTexts[497], 26, 283, FONT_SMALL, tytulowy, to); //Victory Condition:
	printAtLoc(CGI->generaltexth->allTexts[498], 26, 339, FONT_SMALL, tytulowy, to); //Loss Condition:

	if(curMap)
	{
		if(type != newGame)
		{
			for (int i = 0; i < difficulty->buttons.size(); i++)
			{
				//if(i == curMap->difficulty)
				//	difficulty->buttons[i]->state = 3;
				//else
				//	difficulty->buttons[i]->state = 2;

				difficulty->buttons[i]->showAll(to);
			}
		}

		int temp = curMap->victoryCondition.condition+1;
		if (temp>20) temp=0;
		std::string sss = CGI->generaltexth->victoryConditions[temp];
		if (temp && curMap->victoryCondition.allowNormalVictory) sss+= "/" + CGI->generaltexth->victoryConditions[0];
		printAtLoc(sss, 60, 307, FONT_SMALL, zwykly, to);


		temp = curMap->lossCondition.typeOfLossCon+1;
		if (temp>20) temp=0;
		sss = CGI->generaltexth->lossCondtions[temp];
		printAtLoc(sss, 60, 366, FONT_SMALL, zwykly, to);

		//blit description
		std::vector<std::string> *desc = CMessage::breakText(curMap->description,52);
		for (int i=0;i<desc->size();i++)
			printAtLoc((*desc)[i], 26, 149 + i*16, FONT_SMALL, zwykly, to);
		delete desc;

		if (curMap->name.length())
			printAtLoc(curMap->name, 26, 39, FONT_BIG, tytulowy, to);
		else 
			printAtLoc("Unnamed", 26, 39, FONT_BIG, tytulowy, to);

		assert(curMap->difficulty <= 4);
		std::string &diff = CGI->generaltexth->arraytxt[142 + curMap->difficulty];
		printAtMiddleLoc(diff, 62, 472, FONT_SMALL, zwykly, to);

		switch (curMap->width)
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
		temp = curMap->victoryCondition.condition;
		if (temp>12) temp=11;
		blitAtLoc(CGP->victory->ourImages[temp].bitmap, 24, 302, to); //victory cond descr
		temp=curMap->lossCondition.typeOfLossCon;
		if (temp>12) temp=3;
		blitAtLoc(CGP->loss->ourImages[temp].bitmap, 24, 359, to); //loss cond 

		if(type == loadGame)
			printToLoc((static_cast<const CMapInfo*>(curMap))->date,308,34, FONT_SMALL, zwykly, to);

		//print flags
		int fx=64, ex=244, myT;
		//if (curMap->howManyTeams)
			myT = curMap->players[playerColor].team;
		//else 
		//	myT = -1;
		for (std::vector<PlayerSettings>::const_iterator i = curOpts->playerInfos.begin(); i != curOpts->playerInfos.end(); i++)
		{
			int *myx = ((i->color == playerColor  ||  curMap->players[i->color].team == myT) ? &fx : &ex);
			blitAtLoc(sFlags->ourImages[i->color].bitmap, *myx, 399, to);
			*myx += sFlags->ourImages[i->color].bitmap->w;
		}

		std::string tob;
		switch (curOpts->difficulty)
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
		printAtMiddleLoc(tob, 311, 472, FONT_SMALL, zwykly, to);
	}
}

void InfoCard::changeSelection( const CMapInfo *to )
{
	if(to && type != newGame)
		difficulty->select(curOpts->difficulty, 0);
	GH.totalRedraw();
}

void InfoCard::clickRight( tribool down, bool previousState )
{
	static const Rect flagArea(19, 397, 335, 23);
	if(down && curMap && isItInLoc(flagArea, GH.current->motion.x, GH.current->motion.y))
		showTeamsPopup();
}

void InfoCard::showTeamsPopup()
{
	SDL_Surface *bmp = CMessage::drawBox1(256, 90 + 50 * curMap->howManyTeams);
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[657], 128, 30, FONT_MEDIUM, tytulowy, bmp); //{Team Alignments}

	for(int i = 0; i < curMap->howManyTeams; i++)
	{
		std::vector<ui8> flags;
		std::string hlp = CGI->generaltexth->allTexts[656]; //Team %d
		hlp.replace(hlp.find("%d"), 2, boost::lexical_cast<std::string>(i+1));
		CSDL_Ext::printAtMiddle(hlp, 128, 65 + 50*i, FONT_SMALL, zwykly, bmp);

		for(int j = 0; j < PLAYER_LIMIT; j++)
			if((curMap->players[j].canHumanPlay || curMap->players[j].canComputerPlay)
				&& curMap->players[j].team == i)
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

OptionsTab::OptionsTab( EState Type)
:type(Type)
{
	OBJ_CONSTRUCTION;
	bg = new CPicture(BitmapHandler::loadBitmap("ADVOPTBK.bmp"), 0, 0, true);
	pos = bg->pos;

	if(type == newGame)
		turnDuration = new CSlider(55, 551, 194, bind(&OptionsTab::setTurnLength, this, _1), 1, 11, 11, true, 1);
}

OptionsTab::~OptionsTab()
{

}

void OptionsTab::showAll( SDL_Surface * to )
{
	CIntObject::showAll(to);
	printAtMiddleLoc(CGI->generaltexth->allTexts[515], 222, 30, FONT_BIG, tytulowy, to);
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[516], 222, 58, FONT_SMALL, 55, zwykly, to); //Select starting options, handicap, and name for each player in the game.
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[517], 107, 102, FONT_SMALL, 14, tytulowy, to); //Player Name Handicap Type
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[518], 197, 102, FONT_SMALL, 10, tytulowy, to); //Starting Town
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[519], 273, 102, FONT_SMALL, 10, tytulowy, to); //Starting Hero
	printAtMiddleWBLoc(CGI->generaltexth->allTexts[520], 349, 102, FONT_SMALL, 10, tytulowy, to); //Starting Bonus
	printAtMiddleLoc(CGI->generaltexth->allTexts[521], 222, 538, FONT_SMALL, tytulowy, to); // Player Turn Duration


	if(curOpts->turnTime)
	{
		std::ostringstream os;
		os << (int)curOpts->turnTime << " Minutes";
		printAtMiddleLoc(os.str(), 319,559, FONT_SMALL, zwykly, to);
	}
	else
		printAtMiddleLoc("Unlimited",319,559, FONT_SMALL, zwykly, to);
}

void OptionsTab::nextCastle( int player, int dir )
{
	PlayerSettings &s = curOpts->playerInfos[player];
	si32 &cur = s.castle;
	ui32 allowed = curMap->players[s.color].allowedFactions;

 	if (cur == -2) //no castle - no change
 		return;

 	if (cur == -1) //random => first/last available
 	{
 		int pom = (dir>0) ? (0) : (F_NUMBER-1); // last or first
 		for (;pom >= 0  &&  pom < F_NUMBER;  pom+=dir)
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

 			if (cur >= F_NUMBER  ||  cur<0)
 			{
 				double p1 = log((double)allowed) / log(2.0f)+0.000001f;
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
 	if(cur < 0  &&  s.bonus == bresource)
 		s.bonus = brandom;

	entries[player]->selectButtons();
	redraw();
}

void OptionsTab::nextHero( int player, int dir )
{
	PlayerSettings &s = curOpts->playerInfos[player];
	int old = s.hero;
	if (s.castle < 0  ||  !s.human  ||  s.hero == -2)
		return;

	if (s.hero == -1) //random => first/last available
	{
		int max = (s.castle*HEROES_PER_TYPE*2+15),
			min = (s.castle*HEROES_PER_TYPE*2);
		s.hero = nextAllowedHero(min,max,0,dir);
	}
	else
	{
		if(dir > 0)
			s.hero = nextAllowedHero(s.hero,(s.castle*HEROES_PER_TYPE*2+16),1,dir);
		else
			s.hero = nextAllowedHero(s.castle*HEROES_PER_TYPE*2-1,s.hero,1,dir);
	}

	if(old != s.hero)
	{
		usedHeroes.erase(old);
		usedHeroes.insert(s.hero);
		redraw();
	}
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

	return !vstd::contains(usedHeroes, ID) && curMap->allowedHeroes[ID];
}

void OptionsTab::nextBonus( int player, int dir )
{
	PlayerSettings &s = curOpts->playerInfos[player];
	si8 &ret = s.bonus += dir;

	if (s.hero==-2 && !curMap->players[s.color].heroesNames.size() && ret==bartifact) //no hero - can't be artifact
	{
		if (dir<0)
			ret=brandom;
		else ret=bgold;
	}

	if(ret > bresource)
		ret = brandom;
	if(ret < brandom)
		ret = bresource;

	if (s.castle==-1 && ret==bresource) //random castle - can't be resource
	{
		if (dir<0)
			ret=bgold;
		else ret=brandom;
	}

	redraw();
}

void OptionsTab::changeSelection( const CMapHeader *to )
{
	for(int i = 0; i < entries.size(); i++)
	{
		children -= entries[i];
		delete entries[i];
	}
	entries.clear();
	usedHeroes.clear();

	OBJ_CONSTRUCTION;
	for(int i = 0; i < curOpts->playerInfos.size(); i++)
	{
		entries.push_back(new PlayerOptionsEntry(this, curOpts->playerInfos[i]));
		const std::vector<SheroName> &heroes = curMap->players[curOpts->playerInfos[i].color].heroesNames;
		for(size_t hi=0; hi<heroes.size(); hi++)
			usedHeroes.insert(heroes[hi].heroID);
	}
}

void OptionsTab::setTurnLength( int npos )
{
	static const int times[] = {1, 2, 4, 6, 8, 10, 15, 20, 25, 30, 0};
	amin(npos, ARRAY_COUNT(times) - 1);

	curOpts->turnTime = times[npos];
	redraw();
}

void OptionsTab::flagPressed( int player )
{
	if(player == playerSerial) //that color is already selected, no action needed
		return;

	PlayerSettings &s =  curOpts->playerInfos[player],
		&old = curOpts->playerInfos[playerSerial];

	std::swap(old.human, s.human);
	std::swap(old.name, s.name);
	playerColor = s.color;

	if(!entries[playerSerial]->fixedHero)
		old.hero = -1;

	playerSerial = player;
	entries[s.serial]->selectButtons();
	entries[old.serial]->selectButtons();
	GH.totalRedraw();
}

OptionsTab::PlayerOptionsEntry::PlayerOptionsEntry( OptionsTab *owner, PlayerSettings &S)
:s(S)
{
	OBJ_CONSTRUCTION;
	defActions |= SHARE_POS;
	pos = parent->pos + Point(54, 122 + s.serial*50);

	static const char *flags[] = {"AOFLGBR.DEF", "AOFLGBB.DEF", "AOFLGBY.DEF", "AOFLGBG.DEF", 
		"AOFLGBO.DEF", "AOFLGBP.DEF", "AOFLGBT.DEF", "AOFLGBS.DEF"};
	static const char *bgs[] = {"ADOPRPNL.bmp", "ADOPBPNL.bmp", "ADOPYPNL.bmp", "ADOPGPNL.bmp", 
		"ADOPOPNL.bmp", "ADOPPPNL.bmp", "ADOPTPNL.bmp", "ADOPSPNL.bmp"};

	bg = new CPicture(BitmapHandler::loadBitmap(bgs[s.color]), 0, 0, true);
	if(owner->type == newGame)
	{
		btns[0] = new AdventureMapButton(CGI->generaltexth->zelp[132], bind(&OptionsTab::nextCastle, owner, s.serial, -1), 107, 5, "ADOPLFA.DEF");
		btns[1] = new AdventureMapButton(CGI->generaltexth->zelp[133], bind(&OptionsTab::nextCastle, owner, s.serial, +1), 168, 5, "ADOPRTA.DEF");
		btns[2] = new AdventureMapButton(CGI->generaltexth->zelp[148], bind(&OptionsTab::nextHero, owner, s.serial, -1), 183, 5, "ADOPLFA.DEF");
		btns[3] = new AdventureMapButton(CGI->generaltexth->zelp[149], bind(&OptionsTab::nextHero, owner, s.serial, +1), 244, 5, "ADOPRTA.DEF");
		btns[4] = new AdventureMapButton(CGI->generaltexth->zelp[164], bind(&OptionsTab::nextBonus, owner, s.serial, -1), 259, 5, "ADOPLFA.DEF");
		btns[5] = new AdventureMapButton(CGI->generaltexth->zelp[165], bind(&OptionsTab::nextBonus, owner, s.serial, +1), 320, 5, "ADOPRTA.DEF");
	}
	else
		for(int i = 0; i < 6; i++)
			btns[i] = NULL;

	fixedHero = s.hero != -1; //if we doesn't start with "random hero" it must be fixed or none
	selectButtons(false);


	if(owner->type != scenarioInfo  &&  curMap->players[s.color].canHumanPlay)
	{
		flag = new AdventureMapButton(CGI->generaltexth->zelp[180], bind(&OptionsTab::flagPressed, owner, s.serial), -43, 2, flags[s.color]);
		flag->hoverable = true;
	}
	else
		flag = NULL;

	defActions &= ~SHARE_POS;
	town = new SelectedBox(TOWN, s.serial);
	town->pos += pos + Point(119, 2);
	hero = new SelectedBox(HERO, s.serial);
	hero->pos += pos + Point(195, 2);
	bonus = new SelectedBox(BONUS, s.serial);
	bonus->pos += pos + Point(271, 2);
}

void OptionsTab::PlayerOptionsEntry::showAll( SDL_Surface * to )
{
	CIntObject::showAll(to);
	printAtMiddleLoc(s.name, 55, 10, FONT_SMALL, zwykly, to);
}

void OptionsTab::PlayerOptionsEntry::selectButtons(bool onlyHero)
{
	if(!btns[0])
		return;

	if(!onlyHero  &&  s.castle != -1)
	{
		btns[0]->disable();
		btns[1]->disable();
	}
	if(fixedHero  ||  !s.human  ||  s.castle < 0)
	{
		btns[2]->disable();
		btns[3]->disable();
	}
	else
	{
		btns[2]->enable(active);
		btns[3]->enable(active);
	}
}

void OptionsTab::SelectedBox::showAll( SDL_Surface * to )
{
	PlayerSettings &s = curOpts->playerInfos[player];
	SDL_Surface *toBlit = getImg();
	const std::string *toPrint = getText();
	blitAt(toBlit, pos, to);
	printAtMiddleLoc(*toPrint, 23, 39, FONT_TINY, zwykly, to);
}

OptionsTab::SelectedBox::SelectedBox( SelType Which, ui8 Player )
:which(Which), player(Player)
{
	SDL_Surface *img = getImg();
	pos.w = img->w;
	pos.h = img->h;
	used = RCLICK;
}

SDL_Surface * OptionsTab::SelectedBox::getImg() const
{
	PlayerSettings &s = curOpts->playerInfos[player];
	switch(which)
	{
	case TOWN:
		if (s.castle < F_NUMBER  &&  s.castle >= 0)
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
	PlayerSettings &s = curOpts->playerInfos[player];	
	switch(which)
	{
	case TOWN:
		if (s.castle < F_NUMBER  &&  s.castle >= 0)
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
	PlayerSettings &s = curOpts->playerInfos[player];
	SDL_Surface *bmp = NULL;
	const std::string *title = NULL, *subTitle = NULL;

	subTitle = getText();

	int val;
	switch(which)
	{
	case TOWN: 
		val = s.castle; 
		break;
	case HERO: 
		val = s.hero; 
		if(val < 0)
		{
			int p9 = curMap->players[s.color].p9;
			if(p9 != 255  &&  curOpts->playerInfos[player].heroPortrait >= 0)
				val = p9;
		}
		break;
	case BONUS: 
		val = s.bonus; 
		break;
	}

	if(val == -1  ||  which == BONUS) //random or bonus box
	{
		bmp = CMessage::drawBox1(256, 190);
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
				case brandom:
					title = &CGI->generaltexth->allTexts[86]; //{Random Bonus}
					description = &CGI->generaltexth->allTexts[94]; //Gold, wood and ore, or an artifact is randomly chosen as your starting bonus
					break;
				case bartifact:
					title = &CGI->generaltexth->allTexts[83]; //{Artifact Bonus}
					description = &CGI->generaltexth->allTexts[90]; //An artifact is randomly chosen and equipped to your starting hero
					break;
				case bgold:
					title = &CGI->generaltexth->allTexts[84]; //{Gold Bonus}
					subTitle = &CGI->generaltexth->allTexts[87]; //500-1000
					description = &CGI->generaltexth->allTexts[92]; //At the start of the game, 500-1000 gold is added to your Kingdom's resource pool
					break;
				case bresource:
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
			CSDL_Ext::printAtMiddleWB(*description, 125, 145, FONT_SMALL, 37, zwykly, bmp);
	}
	else if(val == -2)
	{
		return;
	}
	else if(which == TOWN)
	{
		bmp = CMessage::drawBox1(256, 319);
		title = &CGI->generaltexth->allTexts[80];

		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[79], 135, 137, FONT_MEDIUM, tytulowy, bmp);
		
		const CTown &t = CGI->townh->towns[val];
		//print creatures
		int x = 60, y = 159;
		for(int i = 0; i < 7; i++)
		{
			int c = t.basicCreatures[i];
			blitAt(graphics->smallImgs[c], x, y, bmp);
			CSDL_Ext::printAtMiddleWB(CGI->creh->creatures[c].nameSing, x + 16, y + 45, FONT_TINY, 10, zwykly, bmp);

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
		bmp = CMessage::drawBox1(320, 255);
		title = &CGI->generaltexth->allTexts[77];

		CSDL_Ext::printAtMiddle(*title, 167, 36, FONT_MEDIUM, tytulowy, bmp);
		CSDL_Ext::printAtMiddle(*subTitle + " - " + h->heroClass->name, 160, 99, FONT_SMALL, zwykly, bmp);

		blitAt(getImg(), 136, 56, bmp);

		//print specialty
		CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[78], 166, 132, FONT_MEDIUM, tytulowy, bmp);
		blitAt(graphics->un44->ourImages[val].bitmap, 140, 150, bmp);

		GH.pushInt(new CInfoPopup(bmp, true));
		return;
	}

	if(title)
		CSDL_Ext::printAtMiddle(*title, 135, 36, FONT_MEDIUM, tytulowy, bmp);
	if(subTitle)
		CSDL_Ext::printAtMiddle(*subTitle, 127, 103, FONT_SMALL, zwykly, bmp);

	blitAt(getImg(), 104, 60, bmp);

	GH.pushInt(new CInfoPopup(bmp, true));
}

CScenarioInfo::CScenarioInfo( const CMapHeader *mapInfo, const StartInfo *startInfo )
{
	OBJ_CONSTRUCTION_CAPTURING_ALL;

	for(size_t i = 0; i < startInfo->playerInfos.size(); i++)
	{
		if(startInfo->playerInfos[i].human)
		{
			playerColor = startInfo->playerInfos[i].color;
			playerSerial = i;
		}
	}

	pos.w = 762;
	pos.h = 584;
	center(pos);

	curMap = mapInfo;
	curOpts = (StartInfo*)startInfo;

	card = new InfoCard(scenarioInfo);
	opt = new OptionsTab(scenarioInfo);
	opt->changeSelection(0);

	card->difficulty->select(startInfo->difficulty, 0);
	back = new AdventureMapButton("", CGI->generaltexth->zelp[105].second, bind(&CGuiHandler::popIntTotally, &GH, this), 584, 535, "SCNRBACK.DEF", SDLK_ESCAPE);
}

CScenarioInfo::~CScenarioInfo()
{
}

CTextInput::CTextInput()
{
	bg = NULL;
	used = 0;
}

CTextInput::CTextInput( const Rect &Pos, const Point &bgOffset, const std::string &bgName, const CFunctionList<void(const std::string &)> &CB )
:cb(CB)
{
	pos += Pos;
	OBJ_CONSTRUCTION;
	bg = new CPicture(bgName, bgOffset.x, bgOffset.y);
	used = LCLICK | KEYBOARD;
}

CTextInput::~CTextInput()
{

}

void CTextInput::showAll( SDL_Surface * to )
{
	CIntObject::showAll(to);
	CSDL_Ext::printAt(text + "_", pos.x, pos.y, FONT_SMALL, zwykly, to);
}

void CTextInput::clickLeft( tribool down, bool previousState )
{
	//TODO

}

void CTextInput::keyPressed( const SDL_KeyboardEvent & key )
{
	if(key.state != SDL_PRESSED) return;
	switch(key.keysym.sym)
	{
	case SDLK_BACKSPACE:
		if(text.size())
			text.resize(text.size()-1);
		break;
	default:
		char c = key.keysym.unicode; //TODO 16-/>8
		static const std::string forbiddenChars = "<>:\"/\\|?*"; //if we are entering a filename, some special characters won't be allowed
		if(!vstd::contains(forbiddenChars,c) && std::isprint(c))
			text += c;
		break;
	}
	redraw();
	cb(text);
}

void CTextInput::setText( const std::string &nText, bool callCb )
{
	text = nText;
	redraw();
	if(callCb)
		cb(text);
}
