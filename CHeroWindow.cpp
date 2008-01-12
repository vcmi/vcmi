#include "stdafx.h"
#include "CHeroWindow.h"
#include "SDL.h"
#include "SDL_Extensions.h"
#include "CAdvmapInterface.h"
#include "AdventureMapButton.h"

extern SDL_Surface * ekran;

CHeroWindow::CHeroWindow(int playerColor)
{
	background = CGI->bitmaph->loadBitmap("HEROSCR4.bmp");
	CSDL_Ext::blueToPlayersAdv(background, playerColor);
	pos.x = 65;
	pos.y = 8;
	pos.h = background->h;
	pos.w = background->w;

	quitButton = new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::quit, 674, 524, "hsbtns.def", this);
	dismissButton = new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::dismissCurrent, 519, 437, "hsbtns2.def", this);
	questlogButton = new AdventureMapButton<CHeroWindow>(std::string(), std::string(), &CHeroWindow::questlog, 379, 437, "hsbtns4.def", this);

	skillpics = CGI->spriteh->giveDef("pskil42.def");
}

CHeroWindow::~CHeroWindow()
{
	SDL_FreeSurface(background);
	delete quitButton;
	delete dismissButton;
	delete questlogButton;

	if(curBack)
		SDL_FreeSurface(curBack);

	delete skillpics;
}

void CHeroWindow::show(SDL_Surface *to)
{
	if(!to)
		to=ekran;
	blitAt(curBack,pos.x,pos.y,to);
	quitButton->show();
	dismissButton->show();
	questlogButton->show();
}

void CHeroWindow::setHero(const CGHeroInstance *hero)
{
	curHero = hero;
}

void CHeroWindow::quit()
{
	for(int i=0; i<LOCPLINT->objsToBlit.size(); ++i)
	{
		if( dynamic_cast<CHeroWindow*>( LOCPLINT->objsToBlit[i] ) )
		{
			LOCPLINT->objsToBlit.erase(LOCPLINT->objsToBlit.begin()+i);
		}
	}
	quitButton->deactivate();
	dismissButton->deactivate();
	questlogButton->deactivate();
	LOCPLINT->adventureInt->show();

	SDL_FreeSurface(curBack);
	curBack = NULL;
}

void CHeroWindow::activate()
{
	quitButton->activate();
	dismissButton->activate();
	questlogButton->activate();

	curBack = CSDL_Ext::copySurface(background);
	blitAt(skillpics->ourImages[0].bitmap, 32, 111, curBack);
	blitAt(skillpics->ourImages[1].bitmap, 102, 111, curBack);
	blitAt(skillpics->ourImages[2].bitmap, 172, 111, curBack);
	blitAt(skillpics->ourImages[5].bitmap, 242, 111, curBack);
	blitAt(skillpics->ourImages[4].bitmap, 20, 230, curBack);
	blitAt(skillpics->ourImages[3].bitmap, 162, 230, curBack);
}

void CHeroWindow::dismissCurrent()
{
}

void CHeroWindow::questlog()
{
}
