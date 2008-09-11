#include "CSpellWindow.h"
#include "client/Graphics.h"
#include "hch/CDefHandler.h"
#include "hch/CObjectHandler.h"
#include "hch/CPreGameTextHandler.h"
#include "CAdvmapInterface.h"
#include "CGameInfo.h"
#include "SDL_Extensions.h"
#include <boost/bind.hpp>
#include <sstream>

extern SDL_Surface * screen;
extern SDL_Color tytulowy, zwykly ;
extern TTF_Font *GEOR16;

SpellbookInteractiveArea::SpellbookInteractiveArea(SDL_Rect & myRect, boost::function<void()> funcL, std::string textR, boost::function<void()> funcHon, boost::function<void()> funcHoff)
{
	pos = myRect;
	onLeft = funcL;
	textOnRclick = textR;
	onHoverOn = funcHon;
	onHoverOff = funcHoff;
}

void SpellbookInteractiveArea::clickLeft(boost::logic::tribool down)
{
	if(!down)
	{
		onLeft();
	}
}

void SpellbookInteractiveArea::clickRight(boost::logic::tribool down)
{
	LOCPLINT->adventureInt->handleRightClick(textOnRclick, down, this);
}

void SpellbookInteractiveArea::hover(bool on)
{
	Hoverable::hover(on);
	if(on)
	{
		onHoverOn();
	}
	else
	{
		onHoverOff();
	}
}

void SpellbookInteractiveArea::activate()
{
	ClickableL::activate();
	ClickableR::activate();
	Hoverable::activate();
}
void SpellbookInteractiveArea::deactivate()
{
	ClickableL::deactivate();
	ClickableR::deactivate();
	Hoverable::deactivate();
}

CSpellWindow::CSpellWindow(const SDL_Rect & myRect, const CGHeroInstance * myHero): selectedTab(4)
{
	pos = myRect;
	background = BitmapHandler::loadBitmap("SpelBack.bmp");
	graphics->blueToPlayersAdv(background, myHero->tempOwner);

	std::stringstream mana;
	mana<<myHero->mana;
	CSDL_Ext::printAtMiddle(mana.str(), 434, 425, GEOR16, tytulowy, background);

	leftCorner = BitmapHandler::loadBitmap("SpelTrnL.bmp");
	rightCorner = BitmapHandler::loadBitmap("SpelTrnL.bmp");
	spells = CDefHandler::giveDef("Spells.def");
	spellTab = CDefHandler::giveDef("SpelTab.def");
	schools = CDefHandler::giveDef("Schools.def");
	schoolW = CDefHandler::giveDef("SplevW.def");
	schoolE = CDefHandler::giveDef("SplevE.def");
	schoolF = CDefHandler::giveDef("SplevF.def");
	schoolA = CDefHandler::giveDef("SplevA.def");



	statusBar = new CStatusBar(97, 571, "Spelroll.bmp");
	exitBtn = new SpellbookInteractiveArea(genRect(45, 35, 569, 407), boost::bind(&CSpellWindow::fexitb, this), CGI->preth->zelp[460].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[460].first)), boost::bind(&CStatusBar::clear, statusBar));
	battleSpells = new SpellbookInteractiveArea(genRect(45, 35, 311, 407), boost::bind(&CSpellWindow::fbattleSpellsb, this), CGI->preth->zelp[453].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[453].first)), boost::bind(&CStatusBar::clear, statusBar));
	adventureSpells = new SpellbookInteractiveArea(genRect(45, 35, 445, 407), boost::bind(&CSpellWindow::fadvSpellsb, this), CGI->preth->zelp[452].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[452].first)), boost::bind(&CStatusBar::clear, statusBar));
	manaPoints = new SpellbookInteractiveArea(genRect(45, 35, 508, 407), boost::bind(&CSpellWindow::fmanaPtsb, this), CGI->preth->zelp[459].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[459].first)), boost::bind(&CStatusBar::clear, statusBar));

	selectSpellsA = new SpellbookInteractiveArea(genRect(36, 56, 639, 96), boost::bind(&CSpellWindow::fspellsAb, this), CGI->preth->zelp[454].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[454].first)), boost::bind(&CStatusBar::clear, statusBar));
	selectSpellsE = new SpellbookInteractiveArea(genRect(36, 56, 639, 153), boost::bind(&CSpellWindow::fspellsEb, this), CGI->preth->zelp[457].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[457].first)), boost::bind(&CStatusBar::clear, statusBar));
	selectSpellsF = new SpellbookInteractiveArea(genRect(36, 56, 639, 212), boost::bind(&CSpellWindow::fspellsFb, this), CGI->preth->zelp[455].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[455].first)), boost::bind(&CStatusBar::clear, statusBar));
	selectSpellsW = new SpellbookInteractiveArea(genRect(36, 56, 639, 272), boost::bind(&CSpellWindow::fspellsWb, this), CGI->preth->zelp[456].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[456].first)), boost::bind(&CStatusBar::clear, statusBar));
	selectSpellsAll = new SpellbookInteractiveArea(genRect(36, 56, 639, 332), boost::bind(&CSpellWindow::fspellsAllb, this), CGI->preth->zelp[458].second, boost::bind(&CStatusBar::print, statusBar, (CGI->preth->zelp[458].first)), boost::bind(&CStatusBar::clear, statusBar));
}

CSpellWindow::~CSpellWindow()
{
	SDL_FreeSurface(background);
	SDL_FreeSurface(leftCorner);
	SDL_FreeSurface(rightCorner);
	delete spells;
	delete spellTab;
	delete schools;
	delete schoolW;
	delete schoolE;
	delete schoolF;
	delete schoolA;

	delete exitBtn;
	delete battleSpells;
	delete adventureSpells;
	delete manaPoints;
	delete statusBar;

	delete selectSpellsA;
	delete selectSpellsE;
	delete selectSpellsF;
	delete selectSpellsW;
	delete selectSpellsAll;
}

void CSpellWindow::fexitb()
{
	deactivate();

	for(int g=0; g<LOCPLINT->objsToBlit.size(); ++g)
	{
		if(dynamic_cast<CSpellWindow*>(LOCPLINT->objsToBlit[g]))
		{
			LOCPLINT->objsToBlit.erase(LOCPLINT->objsToBlit.begin()+g);
			break;
		}
	}

	delete this;
	LOCPLINT->curint->activate();
}

void CSpellWindow::fadvSpellsb()
{
}

void CSpellWindow::fbattleSpellsb()
{
}

void CSpellWindow::fmanaPtsb()
{
}

void CSpellWindow::fspellsAb()
{
	selectedTab = 0;
}

void CSpellWindow::fspellsEb()
{
	selectedTab = 3;
}

void CSpellWindow::fspellsFb()
{
	selectedTab = 1;
}

void CSpellWindow::fspellsWb()
{
	selectedTab = 2;
}

void CSpellWindow::fspellsAllb()
{
	selectedTab = 4;
}

void CSpellWindow::show(SDL_Surface *to)
{
	if(to == NULL)  //evaluating to
		to = screen;

	SDL_BlitSurface(background, NULL, to, &pos);
	blitAt(spellTab->ourImages[selectedTab].bitmap, 614, 96, to);
	
	statusBar->show();
}

void CSpellWindow::activate()
{
	exitBtn->activate();
	battleSpells->activate();
	adventureSpells->activate();
	manaPoints->activate();

	selectSpellsA->activate();
	selectSpellsE->activate();
	selectSpellsF->activate();
	selectSpellsW->activate();
	selectSpellsAll->activate();
}

void CSpellWindow::deactivate()
{
	exitBtn->deactivate();
	battleSpells->deactivate();
	adventureSpells->deactivate();
	manaPoints->deactivate();

	selectSpellsA->deactivate();
	selectSpellsE->deactivate();
	selectSpellsF->deactivate();
	selectSpellsW->deactivate();
	selectSpellsAll->deactivate();
}
