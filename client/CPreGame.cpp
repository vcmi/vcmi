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
#include "../hch/CMusicHandler.h"
#include "../hch/CVideoHandler.h"
#include "AdventureMapButton.h"
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
void startGame(StartInfo * options);

CGPreGame * CGP;
static const CMapInfo *curMap = NULL;
static StartInfo *curOpts = NULL;
static int playerColor;

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
			buttons[4] = new AdventureMapButton("", CGI->generaltexth->zelp[7].second, bind(std::exit, 0), 586, 468, "ZMENUQT.DEF", SDLK_ESCAPE);
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

void CGPreGame::run()
{
	GH.handleEvents();

#ifdef _WIN32
	CGI->videoh->open("ACREDIT.SMK");
#else
	CGI->videoh->open("ACREDIT.SMK", true, false);
#endif

	GH.pushInt(scrs[mainMenu]);

	while(1)
	{
		CGI->curh->draw1();
		SDL_Flip(screen);
		CGI->curh->draw2();
		SDL_Delay(20); //give time for other apps

		GH.topInt()->show(screen);
		GH.updateTime();
		GH.handleEvents();
	}
}

CGPreGame::CGPreGame()
{
	GH.defActionsDef = 31;
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

CSelectionScreen::CSelectionScreen( EState Type )
{
	OBJ_CONSTRUCTION;
	CGP->loadGraphics();
	type = Type;
	curOpts = &sInfo;
	current = NULL;

	sInfo.mode = (Type == newGame ? 0 : 1);
	sInfo.turnTime = 0;
	curTab = NULL;

	bg = new CPicture(BitmapHandler::loadBitmap(rand()%2 ? "ZPIC1000.bmp" : "ZPIC1001.bmp"), 0, 0, true);
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
			AdventureMapButton *select = new AdventureMapButton(CGI->generaltexth->zelp[45], bind(&CSelectionScreen::toggleTab, this, sel), 414, 81, "GSPBUTT.DEF", SDLK_s);
			select->addTextOverlay(CGI->generaltexth->allTexts[500], FONT_SMALL);

			AdventureMapButton *opts = new AdventureMapButton(CGI->generaltexth->zelp[46], bind(&CSelectionScreen::toggleTab, this, opt), 414, 509, "GSPBUTT.DEF", SDLK_a);
			opts->addTextOverlay(CGI->generaltexth->allTexts[501], FONT_SMALL);

			AdventureMapButton *random = new AdventureMapButton(CGI->generaltexth->zelp[47], bind(&CSelectionScreen::toggleTab, this, sel), 414, 105, "GSPBUTT.DEF", SDLK_r);
			random->addTextOverlay(CGI->generaltexth->allTexts[740], FONT_SMALL);

			start  = new AdventureMapButton(CGI->generaltexth->zelp[103], bind(&CSelectionScreen::startGame, this), 414, 535, "SCNRBEG.DEF", SDLK_b);
		}
		break;

	case loadGame:
		card->difficulty->select(current->seldiff, 0);
		sel->recActions = 255;
		start  = new AdventureMapButton(CGI->generaltexth->zelp[103], bind(&CSelectionScreen::startGame, this), 414, 535, "SCNRLOD.DEF", SDLK_b);
		break;
	}

	back = new AdventureMapButton(CGI->generaltexth->zelp[105], bind(&CGuiHandler::popIntTotally, &GH, this), 584, 535, "SCNRBACK.DEF", SDLK_ESCAPE);
}

CSelectionScreen::~CSelectionScreen()
{

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
	updateStartInfo(to);
	card->changeSelection(to);
	opt->changeSelection(to);
}

void CSelectionScreen::updateStartInfo( const CMapInfo * to )
{
	sInfo.mapname = to->filename;
	sInfo.playerInfos.clear();
	sInfo.playerInfos.resize(to->playerAmnt);
	playerColor = -1;

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
	if(!current)
		return;

	//CGP->disposeGraphics();
	StartInfo *si = new StartInfo(sInfo);
	GH.popIntTotally(this);
	GH.popIntTotally(GH.topInt());
	curMap = NULL;
	curOpts = NULL;
	::startGame(si);
	delete si; //rather won't be called...
}

void CSelectionScreen::difficultyChange( int to )
{
	assert(type == newGame);
	sInfo.difficulty = to;
	GH.totalRedraw();
}

// A new size filter (Small, Medium, ...) has been selected. Populate
// selMaps with the relevant data.
void SelectionTab::filter( int size )
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
		slider->moveTo(0);
		onSelect(curItems[0]);
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
	fs::path tie( (fs::initial_path<fs::path>())/dirname );
	fs::directory_iterator end_iter;
	for ( fs::directory_iterator file (tie); file!=end_iter; ++file )
	{
		if(fs::is_regular_file(file->status())
			&& boost::ends_with(file->path().filename(), ext))
		{
			out.resize(out.size()+1);
			out.back().date = fs::last_write_time(file->path());
			out.back().name = dirname+"/"+(file->path().leaf());
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

	used = LCLICK;
	slider = NULL;
	type = Type;
	std::vector<FileInfo> toParse;
	switch(type)
	{
	case newGame:
		getFiles(toParse, "Maps", "h3m");
		parseMaps(toParse);
		positions = 18;
		break;

	case loadGame:
	case saveGame:
		getFiles(toParse, "Games", "vlgm1");
		parseGames(toParse);
		if(type == loadGame)
		{
			positions = 18;
		}
		else
		{
			positions = 16;
		}
		break;

	default:
		assert(0);
	}

	bg = new CPicture(BitmapHandler::loadBitmap("SCSELBCK.bmp"), 3, 6, true);
	pos = bg->pos;

	//size filter buttons
	{
		int sizes[] = {36, 72, 108, 144, 0};
		const char * names[] = {"SCSMBUT.DEF", "SCMDBUT.DEF", "SCLGBUT.DEF", "SCXLBUT.DEF", "SCALBUT.DEF"};
		for(int i = 0; i < 5; i++)
			new AdventureMapButton(CGI->generaltexth->zelp[54+i], bind(&SelectionTab::filter, this, sizes[i]), 161 + 47*i, 52, names[i]);
	}

	{
		int xpos[] = {26, 58, 91, 124, 309, 342};
		const char * names[] = {"SCBUTT1.DEF", "SCBUTT2.DEF", "SCBUTCP.DEF", "SCBUTT3.DEF", "SCBUTT4.DEF", "SCBUTT5.DEF"};
		for(int i = 0; i < 6; i++)
			new AdventureMapButton(CGI->generaltexth->zelp[107+i], bind(&SelectionTab::sortBy, this, i), xpos[i], 92, names[i]);
	}

	slider = new CSlider(375, 92, 480, bind(&SelectionTab::sliderMove, this, _1), positions, curItems.size(), 0, false, 1);
	format =  CDefHandler::giveDef("SCSELC.DEF");

	sortingBy = _format;
	ascending = true;
	filter(0);
	select(0);
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
	// New selection. py is the index in curItems.
	int py = position + slider->value;
	if (py < curItems.size()) 
	{
		CGI->soundh->playSound(soundBase::button);
		selectionPos = py;

		onSelect(curItems[py]);
	}
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

		if (type == newGame)
		{
			if (!(curMap->name.length()))
				curMap->name = "Unnamed";
			CSDL_Ext::printAtMiddle(curMap->name, POS(213, 128), FONT_SMALL, nasz, to);
		}
		else
		{
			std::string &name = curMap->filename;
			CSDL_Ext::printAtMiddle(name.substr(6,name.size()-12), POS(213, 128), FONT_SMALL, nasz, to);
		}

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
	CSDL_Ext::printAtMiddle(CGI->generaltexth->arraytxt[type==newGame ? 229 : 230], 208, 34, FONT_MEDIUM, tytulowy, to); //Select a Scenario to Play
	CSDL_Ext::printAtMiddle(CGI->generaltexth->allTexts[510], 90, 68, FONT_SMALL, tytulowy, to); //Map sizes
}

void SelectionTab::clickLeft( tribool down, bool previousState )
{
	Point clickPos(GH.current->button.x, GH.current->button.y);
	clickPos -= pos.topLeft();

	if (clickPos.y > 115  &&  clickPos.y < 564  &&  clickPos.x > 52  &&  clickPos.x < 366)
	{
		int line = (clickPos.y-115) / 25; //which line
		select(line);

	}
}

InfoCard::InfoCard( EState Type )
{
	OBJ_CONSTRUCTION;
	sizes = CDefHandler::giveDef("SCNRMPSZ.DEF");
	sFlags = CDefHandler::giveDef("ITGFLAGS.DEF");
	type = Type;
	bg = new CPicture(BitmapHandler::loadBitmap("GSELPOP1.bmp"), 396, 6, true);
	pos = bg->pos;
	difficulty = new CHighlightableButtonsGroup(0);
	{
		BLOCK_CAPTURING;
		difficulty->addButton(new CHighlightableButton(CGI->generaltexth->zelp[24], 0, 506, 456, "GSPBUT3.DEF", 0));
		difficulty->addButton(new CHighlightableButton(CGI->generaltexth->zelp[25], 0, 538, 456, "GSPBUT4.DEF", 1));
		difficulty->addButton(new CHighlightableButton(CGI->generaltexth->zelp[26], 0, 570, 456, "GSPBUT5.DEF", 2));
		difficulty->addButton(new CHighlightableButton(CGI->generaltexth->zelp[27], 0, 602, 456, "GSPBUT6.DEF", 3));
		difficulty->addButton(new CHighlightableButton(CGI->generaltexth->zelp[28], 0, 634, 456, "GSPBUT7.DEF", 4));
	}
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
				if(i == curMap->difficulty)
					difficulty->buttons[i]->state = 3;
				else
					difficulty->buttons[i]->state = 2;

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
		blitAt(CGP->victory->ourImages[temp].bitmap, 420, 308, to); //vicotry cond descr
		temp=curMap->lossCondition.typeOfLossCon;
		if (temp>12) temp=3;
		blitAt(CGP->loss->ourImages[temp].bitmap, 420, 365, to); //loss cond 

		if(type == loadGame)
			printToLoc(curMap->date,308,34, FONT_SMALL, zwykly, to);

		//print flags
		int fx=64, ex=244, myT;
		if (curMap->howManyTeams)
			myT = curMap->players[playerColor].team;
		else 
			myT = -1;
		for (std::vector<PlayerSettings>::const_iterator i = curOpts->playerInfos.begin(); i != curOpts->playerInfos.end(); i++)
		{
			int *myx = ((i->color == playerColor  ||  curMap->players[i->color].team == myT) ? &fx : &ex);
			blitAtLoc(sFlags->ourImages[i->color].bitmap, *myx, 399, to);
			*myx += sFlags->ourImages[i->color].bitmap->w;
		}

		std::string tob;
		switch (type != newGame ? curMap->seldiff : curOpts->difficulty)
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
	//current = to;
	GH.totalRedraw();
}

OptionsTab::OptionsTab( EState Type/*, StartInfo &Opts */)
	//:opts(Opts)
{
	OBJ_CONSTRUCTION;
	bg = new CPicture(BitmapHandler::loadBitmap("ADVOPTBK.bmp"), 3, 6, true);
	pos = bg->pos;

	turnDuration = new CSlider(pos.x + 55, pos.y + 551, 194, bind(&OptionsTab::setTurnLength, this, _1), 1, 11, 11, true, 1);
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
	si32 &cur = curOpts->playerInfos[player].castle;
	ui32 allowed = curMap->players[curOpts->playerInfos[player].color].allowedFactions;

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

void OptionsTab::changeSelection( const CMapInfo *to )
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
	PlayerSettings &s =  curOpts->playerInfos[player];
	std::swap(curOpts->playerInfos[playerColor].human, s.human);
	std::swap(curOpts->playerInfos[playerColor].name, s.name);
	playerColor = s.color;
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
	btns[0] = new AdventureMapButton(CGI->generaltexth->zelp[132], bind(&OptionsTab::nextCastle, owner, s.serial, -1), 107, 5, "ADOPLFA.DEF");
	btns[1] = new AdventureMapButton(CGI->generaltexth->zelp[133], bind(&OptionsTab::nextCastle, owner, s.serial, +1), 168, 5, "ADOPRTA.DEF");
	btns[2] = new AdventureMapButton(CGI->generaltexth->zelp[148], bind(&OptionsTab::nextHero, owner, s.serial, -1), 183, 5, "ADOPLFA.DEF");
	btns[3] = new AdventureMapButton(CGI->generaltexth->zelp[149], bind(&OptionsTab::nextHero, owner, s.serial, +1), 244, 5, "ADOPRTA.DEF");
	btns[4] = new AdventureMapButton(CGI->generaltexth->zelp[164], bind(&OptionsTab::nextBonus, owner, s.serial, -1), 259, 5, "ADOPLFA.DEF");
	btns[5] = new AdventureMapButton(CGI->generaltexth->zelp[165], bind(&OptionsTab::nextBonus, owner, s.serial, +1), 320, 5, "ADOPRTA.DEF");

	if(curMap->players[s.color].canHumanPlay)
	{
		flag = new AdventureMapButton(CGI->generaltexth->zelp[180], bind(&OptionsTab::flagPressed, owner, s.serial), -43, 2, flags[s.color]);
		flag->hoverable = true;
	}
	else
		flag = NULL;

	town = new SelectedBox(TOWN, s.serial);
	town->pos = pos + Point(119, 2);
	hero = new SelectedBox(HERO, s.serial);
	hero->pos = pos + Point(195, 2);
	bonus = new SelectedBox(BONUS, s.serial);
	bonus->pos = pos + Point(271, 2);
}

void OptionsTab::PlayerOptionsEntry::showAll( SDL_Surface * to )
{
	CIntObject::showAll(to);
	printAtMiddleLoc(s.name, 55, 10, FONT_SMALL, zwykly, to);
}

void OptionsTab::SelectedBox::showAll( SDL_Surface * to )
{
	PlayerSettings &s = curOpts->playerInfos[player];

	switch(which)
	{
	case TOWN:
		{
			if (s.castle < F_NUMBER  &&  s.castle >= 0)
				blitAt(graphics->getPic(s.castle, true, false), pos, to);
			else if (s.castle == -1)
				blitAt(CGP->rTown, pos, to);
			else if (s.castle == -2)
				blitAt(CGP->nTown, pos, to);
		}
		break;
	case HERO:
		{
			if (s.hero == -1)
			{
				blitAt(CGP->rHero, pos, to);
			}
			else if (s.hero == -2)
			{
				if(s.heroPortrait >= 0)
				{
					blitAt(graphics->portraitSmall[s.heroPortrait], pos, to);
				}
				else
				{
					blitAt(CGP->nHero, pos, to);
				}
			}
			else
			{
				blitAt(graphics->portraitSmall[s.hero], pos, to);
			}
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
			}
			blitAt(CGP->bonuses->ourImages[pom].bitmap, pos, to);
		}
		break;
	}
}

OptionsTab::SelectedBox::SelectedBox( SelType Which, ui8 Player )
:which(Which), player(Player)
{
}