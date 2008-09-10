#include "CSpellWindow.h"
#include "client/Graphics.h"
#include "hch/CDefHandler.h"
#include "hch/CObjectHandler.h"
#include "SDL_Extensions.h"
#include <boost/bind.hpp>
#include <sstream>

extern SDL_Surface * screen;
extern SDL_Color tytulowy, zwykly ;
extern TTF_Font *GEOR16;

ClickableArea::ClickableArea(SDL_Rect &myRect, boost::function<void()> func)
{
	pos = myRect;
	myFunc = func;
}

void ClickableArea::clickLeft(boost::logic::tribool down)
{
	if(!down)
	{
		myFunc();
	}
}

void ClickableArea::activate()
{
	ClickableL::activate();
}
void ClickableArea::deactivate()
{
	ClickableL::deactivate();
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

	SDL_Rect temp_rect = genRect(45, 35, 569, 407);
	exitBtn = new ClickableArea(temp_rect, boost::bind(&CSpellWindow::fexitb, this));
	statusBar = new CStatusBar(97, 571, "Spelroll.bmp");
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
	delete statusBar;
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
}

void CSpellWindow::deactivate()
{
	exitBtn->deactivate();
}
